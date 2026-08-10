#include <uf/engine/graph/graph.h>
#include <uf/ext/gltf/gltf.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/math/hash.h>
#include <uf/utils/mesh/grid.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/string/base64.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/memory/map.h>
#include <uf/utils/memory/unordered_set.h>
#include <uf/ext/xatlas/xatlas.h>
#include <uf/ext/ffx/fsr.h>
#include <uf/utils/math/physics/broadphase/bvh.h>

#include <uf/engine/ext.h>

#if UF_ENV_DREAMCAST
	#define UF_DEBUG_TIMER_MULTITRACE_START(...) UF_TIMER_MULTITRACE_START(__VA_ARGS__)
	#define UF_DEBUG_TIMER_MULTITRACE(...) UF_TIMER_MULTITRACE(__VA_ARGS__)
	#define UF_DEBUG_TIMER_MULTITRACE_END(...) UF_TIMER_MULTITRACE_END(__VA_ARGS__)
#else
	#define UF_DEBUG_TIMER_MULTITRACE_START(...)
	#define UF_DEBUG_TIMER_MULTITRACE(...)
	#define UF_DEBUG_TIMER_MULTITRACE_END(...)
#endif


#define UF_GRAPH_EXTENDED 1
// to-do: fix LOD1+ breaking

namespace {
	struct PendingTexture {
		uf::stl::vector<uint8_t> buffer;
	};

	struct PendingMesh {
		uf::stl::unordered_map<size_t, uf::stl::vector<uint8_t>> buffers;
	};

	struct TextureDescriptor {
		bool srgb = false;
		size_t layers = 1;
		size_t references = 0;
	};

	void convertLightmap( uf::Image& image ) {
		auto* pixels = (pod::Vector4ub*) image.getPixels().data();
		auto& size = image.getDimensions();
		for ( auto p = 0; p < size.x * size.y; ++p ) {
			auto& pixel = pixels[p];
			if ( pixel.w == 0 ) {
				pixel = {0,0,0,255};
				continue;
			}

			// decode
			float exp = (float) pixel.w - 128.0f;
			float mult = std::exp2(exp);

			const float gamma = 1.0f / 2.2f;
			auto linear = pod::Vector3f{ pixel.x, pixel.y, pixel.z } * mult / 255.0f;
			// tone-map
			FOR_EACH( 3, {
				linear[i] = linear[i] / ( 1 + linear[i] );
			});
			// gamma correct
			linear = uf::vector::pow( uf::vector::clamp( linear, 0.0f, 1.0f ), gamma );
			// 0-1 => 0-255
			linear *= 255.0f;
			pixel = { (uint8_t)(linear.x), (uint8_t)(linear.y), (uint8_t)(linear.z), 255 };
		}
	}

	uf::stl::string keyedID( size_t id ) {
		return FMT_FORMAT("{}", id);
	}
	size_t allocateObjectID( pod::Graph::Storage& storage ) {
		return storage.entities.keys.size();
	}
	size_t allocateInstanceID( pod::Graph::Storage& storage, const uf::stl::string& name ) {
		size_t instanceID = 0;
		for ( auto& key : storage.primitives.keys ) {
			if ( key == name ) break;
			instanceID += storage.primitives.map[key].size();
		}
		return instanceID;
	}
	size_t allocateJointID( pod::Graph::Storage& storage, const uf::stl::string& name ) {
		size_t jointID = 0;
		for ( auto& key : storage.joints.keys ) {
			if ( key == name ) break;
			jointID += storage.joints.map[key].size();
		}
		return jointID;
	}

	// removes non-uniform aliased buffers
	void resetBuffers( uf::renderer::Shader& shader ) {
		shader.metadata.aliases.buffers.clear();
	}

	bool bindTextures( pod::Graph& graph, uf::renderer::Graphic& graphic ) {
		auto& storage = uf::graph::getStorage( graph );
		bool changed = false;

		size_t activeCount = 0;
		for ( auto& key : storage.images.keys ) {
			auto& texture = storage.images.map[key].handle;
			if ( texture.viewType != uf::renderer::enums::Image::VIEW_TYPE_2D ) continue;
			activeCount++;
		}

		if ( graphic.material.textures.size() != activeCount ) {
			changed = true;
		} else {
			size_t idx = 0;
			for ( auto& key : storage.images.keys ) {
				auto& texture = storage.images.map[key].handle;
				if ( texture.viewType != uf::renderer::enums::Image::VIEW_TYPE_2D ) continue;

				if ( graphic.material.textures[idx].image != texture.image ) {
					changed = true;
					break;
				}
				idx++;
			}
		}

		if ( changed ) {
			graphic.material.textures.clear();
			for ( auto& key : storage.images.keys ) {
				auto& texture = storage.images.map[key].handle;
				if ( texture.viewType != uf::renderer::enums::Image::VIEW_TYPE_2D ) continue;
				graphic.material.textures.emplace_back().aliasTexture( texture );
			}
		}

		return changed;
	}

	void bindShaders( pod::Graph& graph, uf::Object& entity, uf::Mesh& mesh, uf::stl::vector<pod::Primitive>& primitives ) {
		auto& scene = uf::scene::getCurrentScene();
		auto& sceneTextures = scene.getComponent<pod::SceneTextures>();
		auto& sceneMetadataJson = scene.getComponent<uf::Serializer>();
		auto& storage = uf::graph::getStorage( graph );

		auto& graphic = entity.getComponent<uf::renderer::Graphic>();
		auto& graphMetadataJson = graph.metadata;

		uf::stl::string root = uf::io::directory( graph.name );
		size_t texture2Ds = 0;
		size_t texture3Ds = 0;
		size_t textureCubes = 0;
		for ( auto& texture : graphic.material.textures ) {
			if ( texture.width > 1 && texture.height > 1 && texture.depth == 1 && texture.layers == 1 ) ++texture2Ds;
			else if ( texture.width > 1 && texture.height > 1 && texture.depth == 1 && texture.layers == 6 ) ++textureCubes;
			else if ( texture.width > 1 && texture.height > 1 && texture.depth > 1 && texture.layers == 1 ) ++texture3Ds;
		}

		// standard pipeline
		{
			uf::stl::string dir = "/graph/base/";
			uf::stl::string vertexShaderFilename = graphMetadataJson["shaders"]["vertex"].as<uf::stl::string>(FMT_FORMAT("{}/{}", dir, "vert.spv" )); {
				std::pair<bool, uf::stl::string> settings[] = {
					{ graphMetadataJson["renderer"]["skinned"].as<bool>(), "skinned.vert" },
					{ !graphMetadataJson["renderer"]["separate"].as<bool>(), "instanced.vert" },
				};
				FOR_ARRAY(settings) if ( settings[i].first ) vertexShaderFilename = uf::string::replace( vertexShaderFilename, "vert", settings[i].second );
				vertexShaderFilename = entity.resolveURI( vertexShaderFilename, root );
			}
			uf::stl::string geometryShaderFilename = graphMetadataJson["shaders"]["geometry"].as<uf::stl::string>(""); if ( geometryShaderFilename != "" ) {
				geometryShaderFilename = entity.resolveURI( geometryShaderFilename, root );
			}
			
			if ( graphic.descriptor.renderTarget == 1 ) {
				dir = "/base/graph/";
			}
			uf::stl::string fragmentShaderFilename = graphMetadataJson["shaders"]["fragment"].as<uf::stl::string>(FMT_FORMAT("{}/{}", dir, "frag.spv")); {
				fragmentShaderFilename = entity.resolveURI( fragmentShaderFilename, root );
			}

			graphic.material.metadata.autoInitializeUniformBuffers = false;
			graphic.material.attachShader(vertexShaderFilename, uf::renderer::enums::Shader::VERTEX);
			graphic.material.attachShader(fragmentShaderFilename, uf::renderer::enums::Shader::FRAGMENT);
		#if UF_USE_VULKAN
			if ( geometryShaderFilename != "" && uf::renderer::device.enabledFeatures.geometryShader ) { // to-do: should cram in the attachShader itself
				graphic.material.attachShader(geometryShaderFilename, uf::renderer::enums::Shader::GEOMETRY);
			}
		#endif
			graphic.material.metadata.autoInitializeUniformBuffers = true;
			// vertex shader
			{
				auto& shader = graphic.material.getShader("vertex");
			#if UF_USE_VULKAN
				uint32_t maxPasses = 6;
				shader.setSpecializationConstants({
					{ "PASSES", maxPasses }
				});
			#endif
			}
			// fragment shader
			{
				auto& shader = graphic.material.getShader("fragment");
			#if UF_USE_VULKAN
				size_t maxTextures = storage.textures.map.size();
				size_t maxCubemaps = uf::config["engine"]["scenes"]["textures"]["max"]["cube"].as<size_t>(128);
				size_t maxTextures3D = uf::config["engine"]["scenes"]["textures"]["max"]["3D"].as<size_t>(128);
				uint32_t maxCascades = sceneTextures.voxels.id.size();

				shader.setSpecializationConstants({
					{ "TEXTURES", maxTextures },
					{ "CUBEMAPS", maxCubemaps },
					{ "CASCADES", maxCascades },
				});
				shader.setDescriptorCounts({
					{ "samplerTextures", maxTextures },
					{ "samplerCubemaps", maxCubemaps },
					{ "voxelOutput", maxCascades },
				});
			#endif
			}
		}

		#if UF_USE_VULKAN
		// depth only pipeline
		{
			uf::stl::string fragmentShaderFilename = graphMetadataJson["shaders"]["depth"]["fragmment"].as<uf::stl::string>("/graph/depth/frag.spv");
			fragmentShaderFilename = entity.resolveURI( fragmentShaderFilename, root );
			
			graphic.material.metadata.autoInitializeUniformBuffers = false;
			graphic.material.attachShader(fragmentShaderFilename, uf::renderer::enums::Shader::FRAGMENT, "depth");
			graphic.material.metadata.autoInitializeUniformBuffers = true;

			// fragment shader
			auto& shader = graphic.material.getShader("fragment", "depth");

			uint32_t maxTextures = storage.textures.map.size();
			shader.setSpecializationConstants({
				{ "TEXTURES", maxTextures },
			});
			shader.setDescriptorCounts({
				{ "samplerTextures", maxTextures },
			});
		}
		// culling pipeline
		if ( uf::renderer::settings::pipelines::culling && graphic.descriptor.inputs.indirect.count ) {
			uf::stl::string compShaderFilename = graphMetadataJson["shaders"][uf::renderer::settings::pipelines::names::culling]["compute"].as<uf::stl::string>("/graph/cull/comp.spv");
			compShaderFilename = entity.resolveURI( compShaderFilename, root );

			graphic.material.metadata.autoInitializeUniformBuffers = false;
			graphic.material.attachShader(compShaderFilename, uf::renderer::enums::Shader::COMPUTE, uf::renderer::settings::pipelines::names::culling);
			graphic.material.metadata.autoInitializeUniformBuffers = true;

			graphic.descriptor.bind.width = graphic.descriptor.inputs.indirect.count;
			graphic.descriptor.bind.height = 1;
			graphic.descriptor.bind.depth = 1;
			graphic.descriptor.bind.point = VK_PIPELINE_BIND_POINT_COMPUTE;

			// compute shader
			auto& shader = graphic.material.getShader("compute", uf::renderer::settings::pipelines::names::culling);
		}
		// vxgi pipeline
		if ( uf::renderer::settings::pipelines::vxgi ) {
			uf::stl::string geometryShaderFilename = graphMetadataJson["shaders"][uf::renderer::settings::pipelines::names::vxgi]["geometry"].as<uf::stl::string>("/graph/voxelize/geom.spv"); {
				geometryShaderFilename = entity.resolveURI( geometryShaderFilename, root );
			}
			uf::stl::string fragmentShaderFilename = graphMetadataJson["shaders"][uf::renderer::settings::pipelines::names::vxgi]["fragment"].as<uf::stl::string>("/graph/voxelize/frag.spv"); if ( geometryShaderFilename != "" ) {
				fragmentShaderFilename = entity.resolveURI( fragmentShaderFilename, root );
			}

			graphic.material.metadata.autoInitializeUniformBuffers = false;
			graphic.material.attachShader(fragmentShaderFilename, uf::renderer::enums::Shader::FRAGMENT, uf::renderer::settings::pipelines::names::vxgi);
			graphic.material.metadata.autoInitializeUniformBuffers = true;
			if ( geometryShaderFilename != "" && uf::renderer::device.enabledFeatures.geometryShader ) { // to-do: should cram in the attachShader itself
				graphic.material.attachShader(geometryShaderFilename, uf::renderer::enums::Shader::GEOMETRY, uf::renderer::settings::pipelines::names::vxgi);
			}

			uint32_t maxTextures = texture2Ds;
			uint32_t maxCascades = sceneTextures.voxels.id.size();

			// fragment shader
			{
				auto& shader = graphic.material.getShader("fragment", uf::renderer::settings::pipelines::names::vxgi);
				shader.setSpecializationConstants({
					{ "TEXTURES", maxTextures },
					{ "CASCADES", maxCascades },
				});
				shader.setDescriptorCounts({
					{ "samplerTextures", maxTextures },
					{ "voxelId", maxCascades },
					{ "voxelNormal", maxCascades },
					{ "voxelRadiance", maxCascades },
					{ "voxelOutput", maxCascades },
				});

				for ( auto& t : sceneTextures.voxels.id ) shader.textures.emplace_back().aliasTexture(t);
				for ( auto& t : sceneTextures.voxels.normal ) shader.textures.emplace_back().aliasTexture(t);
				for ( auto& t : sceneTextures.voxels.radiance ) shader.textures.emplace_back().aliasTexture(t);
				for ( auto& t : sceneTextures.voxels.output ) shader.textures.emplace_back().aliasTexture(t);
			}
		}
		// baking pipeline
		if ( graphMetadataJson["baking"]["enabled"].as<bool>() ) {
			uf::stl::string vertexShaderFilename = uf::io::resolveURI("/graph/baking/vert.spv");
			uf::stl::string fragmentShaderFilename = uf::io::resolveURI("/graph/baking/frag.spv");
			std::pair<bool, uf::stl::string> settings[] = {
				{ uf::renderer::settings::pipelines::rt, "rt.frag" },
			};
			FOR_ARRAY(settings) if ( settings[i].first ) fragmentShaderFilename = uf::string::replace( fragmentShaderFilename, "frag", settings[i].second );

			graphic.material.metadata.autoInitializeUniformBuffers = false;
			graphic.material.attachShader(vertexShaderFilename, uf::renderer::enums::Shader::VERTEX, "baking");
			graphic.material.attachShader(fragmentShaderFilename, uf::renderer::enums::Shader::FRAGMENT, "baking");
			graphic.material.metadata.autoInitializeUniformBuffers = true;

			// vertex shader
			{
				uint32_t maxPasses = 6;

				auto& shader = graphic.material.getShader("vertex", "baking");
				shader.setSpecializationConstants({
					{ "PASSES", maxPasses },
				});
			}

			// fragment shader
			{
				size_t maxTextures = uf::config["engine"]["scenes"]["textures"]["max"]["2D"].as<size_t>(512);
				size_t maxCubemaps = uf::config["engine"]["scenes"]["textures"]["max"]["cube"].as<size_t>(128);
				size_t maxTextures3D = uf::config["engine"]["scenes"]["textures"]["max"]["3D"].as<size_t>(128);

				auto& shader = graphic.material.getShader("fragment", "baking");
				shader.setSpecializationConstants({
					{ "TEXTURES", maxTextures },
					{ "CUBEMAPS", maxCubemaps },
				});
				shader.setDescriptorCounts({
					{ "samplerTextures", maxTextures },
					{ "samplerCubemaps", maxCubemaps },
				});
			}
		}

		// rt pipeline
		// to-do: segregate out buffer updating code
		if ( uf::renderer::settings::pipelines::rt && mesh.vertex.count ) {
			if ( graphMetadataJson["renderer"]["skinned"].as<bool>() ) {
				uf::stl::string compShaderFilename = graphMetadataJson["shaders"]["skinning"]["compute"].as<uf::stl::string>("/graph/skinning/skinning.comp.spv"); {
					compShaderFilename = entity.resolveURI( compShaderFilename, root );
				}
			
			//	graphic.material.metadata.autoInitializeUniformBuffers = false;
				graphic.material.attachShader(compShaderFilename, uf::renderer::enums::Shader::COMPUTE, "skinning");
			//	graphic.material.metadata.autoInitializeUniformBuffers = true;
			
				graphic.descriptor.bind.width = mesh.vertex.count;
				graphic.descriptor.bind.height = 1;
				graphic.descriptor.bind.depth = 1;

				uf::Mesh::Attribute vertexPos;
				uf::Mesh::Attribute vertexJoints;
				uf::Mesh::Attribute vertexWeights;

				size_t vertexPosIndex = 0;
				size_t vertexJointsIndex = 0;
				size_t vertexWeightsIndex = 0;

				for ( size_t i = 0; i < graphic.descriptor.inputs.vertex.attributes.size(); ++i ) {
					auto& attribute = graphic.descriptor.inputs.vertex.attributes[i];

					if ( attribute.buffer < 0 ) continue;
					if ( attribute.descriptor.name == "position" ) {
						vertexPos = attribute; 
						vertexPosIndex = graphic.metadata.buffers["vertex[position]"];
					}
					else if ( attribute.descriptor.name == "joints" ) {
						vertexJoints = attribute;
						vertexJointsIndex = graphic.metadata.buffers["vertex[joints]"];
					}
					else if ( attribute.descriptor.name == "weights" ) {
						vertexWeights = attribute;
						vertexWeightsIndex = graphic.metadata.buffers["vertex[weights]"];
					}
				}

				auto& vertexSourceData = mesh.buffers[vertexPos.buffer];
				size_t vertexSourceDataIndex = graphic.initializeBuffer( (const void*) vertexSourceData.data(), vertexSourceData.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR );

				auto& vertexPositionBuffer = graphic.buffers.at(vertexPosIndex);
				auto& vertexJointsBuffer = graphic.buffers.at(vertexJointsIndex);
				auto& vertexWeightsBuffer = graphic.buffers.at(vertexWeightsIndex);

				auto& vertexOutPosition = graphic.buffers.at(vertexSourceDataIndex);
				graphic.metadata.buffers["vertexSkinned"] = vertexSourceDataIndex;

				auto& shader = graphic.material.getShader("compute", "skinning");

				struct PushConstant {
					uint32_t jointID;
					uint32_t vertexCount;
				};

				auto& pushConstant = shader.pushConstants.front().get<PushConstant>();
				pushConstant = {
					.jointID = (uint32_t) primitives.front().instance.jointID,
					.vertexCount = (uint32_t) mesh.vertex.count,
				};

				// bind buffers
				::resetBuffers( shader );
				shader.aliasBuffer( storage.buffers.joint );
				shader.aliasBuffer( vertexPositionBuffer );
				shader.aliasBuffer( vertexJointsBuffer );
				shader.aliasBuffer( vertexWeightsBuffer );
				shader.aliasBuffer( vertexOutPosition );
			}

			graphic.generateBottomAccelerationStructures();
		}
		#endif
	}

	void bindBuffers( pod::Graph& graph, uf::renderer::Graphic& graphic, uf::Mesh& mesh ) {
		auto& scene = uf::scene::getCurrentScene();
		auto& sceneTextures = scene.getComponent<pod::SceneTextures>();
		auto& sceneMetadataJson = scene.getComponent<uf::Serializer>();
		auto& storage = uf::graph::getStorage( graph );

		// draw command buffer for binding
		uf::renderer::Buffer* indirect = NULL;
		for ( auto& buffer : graphic.buffers ) if ( !indirect && buffer.usage & uf::renderer::enums::Buffer::INDIRECT ) indirect = &buffer;

		// standard pipeline
		{
			// vertex shader
			{
				auto& shader = graphic.material.getShader("vertex");

				// bind buffers
				::resetBuffers( shader );
				shader.aliasBuffer( "camera", storage.buffers.camera );

			#if UF_USE_VULKAN
				shader.aliasBuffer( "indirect", *indirect );
			#endif
				shader.aliasBuffer( "instance", storage.buffers.instance );
				shader.aliasBuffer( "object", storage.buffers.object );
				shader.aliasBuffer( "joint", storage.buffers.joint );
			}
			// fragment shader
			{
				auto& shader = graphic.material.getShader("fragment");

				// bind buffers
				::resetBuffers( shader );
				shader.aliasBuffer( "drawCommands", storage.buffers.drawCommands );
				shader.aliasBuffer( "instance", storage.buffers.instance );
				shader.aliasBuffer( "addresses", storage.buffers.addresses );
				shader.aliasBuffer( "material", storage.buffers.material );
				shader.aliasBuffer( "texture", storage.buffers.texture );
				shader.aliasBuffer( "light", storage.buffers.light );
				shader.aliasBuffer( "object", storage.buffers.object );
				shader.aliasBuffer( "camera", storage.buffers.camera );
			}
		}


		#if UF_USE_VULKAN
		// depth only pipeline
		{
			// fragment shader
			auto& shader = graphic.material.getShader("fragment", "depth");

			// bind buffers
			::resetBuffers( shader );
			shader.aliasBuffer( "drawCommands", storage.buffers.drawCommands );
			shader.aliasBuffer( "instance", storage.buffers.instance );
			shader.aliasBuffer( "addresses", storage.buffers.addresses );
			shader.aliasBuffer( "material", storage.buffers.material );
			shader.aliasBuffer( "texture", storage.buffers.texture );
			shader.aliasBuffer( "light", storage.buffers.light );
		}
		// culling pipeline
		if ( uf::renderer::settings::pipelines::culling && indirect ) {
			// compute shader
			auto& shader = graphic.material.getShader("compute", uf::renderer::settings::pipelines::names::culling);

			// bind buffers
			::resetBuffers( shader );
			shader.aliasBuffer( "camera", storage.buffers.camera );
			shader.aliasBuffer( "indirect", *indirect );
			shader.aliasBuffer( "instance", storage.buffers.instance );
			shader.aliasBuffer( "lodMetadata", storage.buffers.lodMetadata );
			shader.aliasBuffer( "object", storage.buffers.object );

			shader.textures.clear();
			shader.textures.emplace_back().aliasTexture( storage.buffers.depthPyramid );
		}

		// vxgi pipeline
		if ( uf::renderer::settings::pipelines::vxgi ) {	
			// fragment shader
			{
				auto& shader = graphic.material.getShader("fragment", uf::renderer::settings::pipelines::names::vxgi);
				
				// bind buffers
				::resetBuffers( shader );
				shader.aliasBuffer( "drawCommands", storage.buffers.drawCommands );
				shader.aliasBuffer( "instance", storage.buffers.instance );
				shader.aliasBuffer( "addresses", storage.buffers.addresses );
				shader.aliasBuffer( "material", storage.buffers.material );
				shader.aliasBuffer( "texture", storage.buffers.texture );
				shader.aliasBuffer( "light", storage.buffers.light );
			}
		}
		// baking pipeline
		if ( graphic.material.hasShader("vertex", "baking") ) {
			// vertex shader
			{
				auto& shader = graphic.material.getShader("vertex", "baking");

				// bind buffers
				::resetBuffers( shader );
				shader.aliasBuffer( "camera", storage.buffers.camera );
				shader.aliasBuffer( "indirect", *indirect );
				shader.aliasBuffer( "instance", storage.buffers.instance );
				shader.aliasBuffer( "object", storage.buffers.object );
				shader.aliasBuffer( "joint", storage.buffers.joint );
			}

			// fragment shader
			{
				auto& shader = graphic.material.getShader("fragment", "baking");

				// bind buffers
				::resetBuffers( shader );
				shader.aliasBuffer( "drawCommands", storage.buffers.drawCommands );
				shader.aliasBuffer( "instance", storage.buffers.instance );
				shader.aliasBuffer( "addresses", storage.buffers.addresses );
				shader.aliasBuffer( "material", storage.buffers.material );
				shader.aliasBuffer( "texture", storage.buffers.texture );
				shader.aliasBuffer( "light", storage.buffers.light );
			}
		}
		#endif
	}

	void bindAddresses( pod::Graph& graph, uf::renderer::Graphic& graphic, uf::Mesh& mesh, uf::stl::vector<pod::Primitive>& primitives ) {
	#if UF_USE_VULKAN
		if ( !uf::renderer::settings::invariant::deviceAddressing || !mesh.indirect.count ) return;

		pod::Instance::Addresses addresses;
		if ( mesh.vertex.count ) {
			for ( auto& attribute : graphic.descriptor.inputs.vertex.attributes ) {
				if ( attribute.buffer < 0 ) continue;
				if ( attribute.descriptor.name == "position" ) 		addresses.position = graphic.buffers.at(attribute.buffer).getAddress();
				else if ( attribute.descriptor.name == "uv" ) 		addresses.uv = graphic.buffers.at(attribute.buffer).getAddress();
				else if ( attribute.descriptor.name == "color" ) 	addresses.color = graphic.buffers.at(attribute.buffer).getAddress();
				else if ( attribute.descriptor.name == "st" ) 		addresses.st = graphic.buffers.at(attribute.buffer).getAddress();
				else if ( attribute.descriptor.name == "normal" ) 	addresses.normal = graphic.buffers.at(attribute.buffer).getAddress();
				else if ( attribute.descriptor.name == "tangent" ) 	addresses.tangent = graphic.buffers.at(attribute.buffer).getAddress();
				else if ( attribute.descriptor.name == "joints" ) 	addresses.joints = graphic.buffers.at(attribute.buffer).getAddress();
				else if ( attribute.descriptor.name == "weights" ) 	addresses.weights = graphic.buffers.at(attribute.buffer).getAddress();
			}
		}
		if ( mesh.index.count ) {
			addresses.index = graphic.buffers.at(graphic.descriptor.inputs.index.attributes.front().buffer).getAddress();
		}

		if ( mesh.indirect.count ) {
			addresses.indirect = graphic.buffers.at(graphic.descriptor.inputs.indirect.attributes.front().buffer).getAddress();
		}

		for ( size_t drawID = 0; drawID < mesh.indirect.count; ++drawID ) {
			// copy address
			primitives[drawID].addresses = addresses;
			// bind draw ID (necessary for deferred pass where we store <instanceID + primitiveID>, to fetch the drawID, to fetch the triangleID) (or something)
			primitives[drawID].addresses.drawID = drawID;
		}
	#endif
	}
}

size_t uf::graph::initialBufferElements = 1024;

uint32_t uf::graph::storageMode = pod::Graph::Storage::SCENE;
pod::Graph::Storage uf::graph::globalStorage;

UF_VERTEX_DESCRIPTOR(uf::graph::mesh::Base,
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base, R32G32_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base, R8G8B8A8_UNORM, color)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base, R32G32_SFLOAT, st)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base, R32G32B32_SFLOAT, normal)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base, R32G32B32A32_SFLOAT, tangent)
);
// it'd be super sugoi if I could somehow macro this annoyance
UF_VERTEX_INTERPOLATE(uf::graph::mesh::Base, {
	return {
		uf::vector::lerp( p1.position, p2.position, t ),
		uf::vector::lerp( p1.uv, p2.uv, t ),
		t < 0.5 ? p1.color : p2.color,
		uf::vector::lerp( p1.st, p2.st, t ),
		uf::vector::lerp( p1.normal, p2.normal, t ),
		uf::vector::lerp( p1.tangent, p2.tangent, t ),
	};
})

UF_VERTEX_DESCRIPTOR(uf::graph::mesh::Skinned,
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R32G32_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R8G8B8A8_UNORM, color)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R32G32_SFLOAT, st)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R32G32B32_SFLOAT, normal)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R32G32B32A32_SFLOAT, tangent)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R16G16B16A16_UINT, joints)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R32G32B32A32_SFLOAT, weights)
);
UF_VERTEX_INTERPOLATE(uf::graph::mesh::Skinned, {
	return {
		uf::vector::lerp( p1.position, p2.position, t ),
		uf::vector::lerp( p1.uv, p2.uv, t ),
		t < 0.5 ? p1.color : p2.color,
		uf::vector::lerp( p1.st, p2.st, t ),
		uf::vector::lerp( p1.normal, p2.normal, t ),
		uf::vector::lerp( p1.tangent, p2.tangent, t ),
		t < 0.5 ? p1.joints : p2.joints,
		uf::vector::lerp( p1.weights, p2.weights, t ),
	};
})

#if UF_USE_FLOAT16
UF_VERTEX_DESCRIPTOR(uf::graph::mesh::Base_16f,
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base_16f, R16G16B16_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base_16f, R16G16_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base_16f, R8G8B8A8_UNORM, color)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base_16f, R16G16_SFLOAT, st)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base_16f, R16G16B16_SFLOAT, normal)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base_16f, R16G16B16A16_SFLOAT, tangent)
);
UF_VERTEX_INTERPOLATE(uf::graph::mesh::Base_16f, {
	return t < 0.5 ? p1 : p2;
})

UF_VERTEX_DESCRIPTOR(uf::graph::mesh::Skinned_16f,
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_16f, R16G16B16_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_16f, R16G16_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_16f, R8G8B8A8_UNORM, color)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_16f, R16G16_SFLOAT, st)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_16f, R16G16B16_SFLOAT, normal)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_16f, R16G16B16A16_SFLOAT, tangent)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_16f, R16G16B16A16_UINT, joints)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_16f, R16G16B16A16_SFLOAT, weights)
);
UF_VERTEX_INTERPOLATE(uf::graph::mesh::Skinned_16f, {
	return t < 0.5 ? p1 : p2;
})
#endif

UF_VERTEX_DESCRIPTOR(uf::graph::mesh::Base_u16q,
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base_u16q, R16G16B16_UINT, position)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base_u16q, R16G16_UINT, uv)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base_u16q, R8G8B8A8_UNORM, color)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base_u16q, R16G16_UINT, st)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base_u16q, R16G16B16_UINT, normal)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base_u16q, R16G16B16A16_UINT, tangent)
);
UF_VERTEX_INTERPOLATE(uf::graph::mesh::Base_u16q, {
	return t < 0.5 ? p1 : p2;
})

UF_VERTEX_DESCRIPTOR(uf::graph::mesh::Skinned_u16q,
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_u16q, R16G16B16_UINT, position)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_u16q, R16G16_UINT, uv)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_u16q, R8G8B8A8_UNORM, color)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_u16q, R16G16_UINT, st)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_u16q, R16G16B16_UINT, normal)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_u16q, R16G16B16A16_UINT, tangent)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_u16q, R16G16B16A16_UINT, joints)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_u16q, R16G16B16A16_UINT, weights)
);

UF_VERTEX_INTERPOLATE(uf::graph::mesh::Skinned_u16q, {
	return t < 0.5 ? p1 : p2;
})

pod::Graph::Storage& uf::graph::getStorage( pod::Graph& graph ) {
	if ( graph.storage ) return *graph.storage; // just fetch it if it already exists

	switch ( uf::graph::storageMode ) {
		case pod::Graph::Storage::OBJECT: {
			if ( !graph.root.entity ) {
				UF_EXCEPTION("Graph root entity is null in OBJECT storage mode.");
			}

			uf::Object* entity = graph.root.entity;
			while ( entity ) {
				if ( entity->hasComponent<pod::Graph::Storage>() ) {
					return entity->getComponent<pod::Graph::Storage>();
				}
				entity = entity->hasParent() ? &entity->getParent() : nullptr;
			}
			UF_EXCEPTION("Failed to find pod::Graph::Storage in entity hierarchy.");
		}
		case pod::Graph::Storage::GRAPH: {
			if ( !graph.storage ) graph.storage = new pod::Graph::Storage();
			return *graph.storage;
		}
		case pod::Graph::Storage::SCENE: {
			return uf::scene::getCurrentScene().getComponent<pod::Graph::Storage>();
		}
		case pod::Graph::Storage::GLOBAL:
		default: {
			return uf::graph::globalStorage;
		}
	}
}

pod::Graph::Storage& uf::graph::getStorage( uf::Object& object ) {
	switch ( uf::graph::storageMode ) {
		case pod::Graph::Storage::OBJECT: {
			// Assume the object itself or one of its parents holds the storage
			uf::Object* current = &object;
			while ( current ) {
				if ( current->hasComponent<pod::Graph::Storage>() ) {
					return current->getComponent<pod::Graph::Storage>();
				}
				current = current->hasParent() ? &current->getParent() : nullptr;
			}
			// fall back to scene, since it's more than likely trying to grab the scene anyways
			if ( uf::scene::getCurrentScene().as<uf::Object>() == object.as<uf::Object>() ) {
				return object.getComponent<pod::Graph::Storage>();
			}
			UF_EXCEPTION("No storage component found on object or its parents.");
		}
		case pod::Graph::Storage::GRAPH: {
			// Safely fetch graph and its storage
			auto& graph = object.getComponent<pod::Graph>();
			if ( !graph.storage ) graph.storage = new pod::Graph::Storage();
			return *graph.storage;
		}
		case pod::Graph::Storage::SCENE: {
			return uf::scene::getCurrentScene().getComponent<pod::Graph::Storage>();
		}
		case pod::Graph::Storage::GLOBAL:
		default: {
			return uf::graph::globalStorage;
		}
	}
}

// yucky
const pod::Graph::Storage& uf::graph::getStorage( const uf::Object& object ) {
	auto& o = const_cast<uf::Object&>( object );
	return uf::graph::getStorage( o );
}
const pod::Graph::Storage& uf::graph::getStorage( const pod::Graph& graph ) {
	auto& g = const_cast<pod::Graph&>( graph );
	return uf::graph::getStorage( g );
}

void uf::graph::initializeGraphics( pod::Graph& graph, uf::Object& entity, uf::Mesh& mesh, uf::stl::vector<pod::Primitive>& primitives ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& sceneTextures = scene.getComponent<pod::SceneTextures>();
	auto& sceneMetadataJson = scene.getComponent<uf::Serializer>();
	auto& storage = uf::graph::getStorage( graph );
	
	auto& graphMetadataJson = graph.metadata;

	auto& graphic = entity.getComponent<uf::renderer::Graphic>();
	graphic.initialize();
	graphic.initializeMesh( mesh );

	graphic.device = &uf::renderer::device;
	graphic.material.device = &uf::renderer::device;
	graphic.descriptor.frontFace = graphMetadataJson["renderer"]["invert"].as<bool>(true) ? uf::renderer::enums::Face::CW : uf::renderer::enums::Face::CCW;
	graphic.descriptor.cullMode = uf::renderer::enums::CullMode::BACK;
	
	auto& entityName = entity.getName();
	auto& metadataJson = entity.getComponent<uf::Serializer>();
	auto& metadataValve = metadataJson["valve"];
	if ( entityName == "skybox" || metadataValve["skyboxed"].as<bool>(false) ) {
		graphic.descriptor.aux = 1; // to-do: some global enums for this shit
	}

	auto tag = metadataJson["graph"];
	if ( !ext::json::isObject( tag ) ) {
		tag["renderer"] = graphMetadataJson["renderer"];
	}

	if ( tag["renderer"]["front face"].is<uf::stl::string>() ) {
		const auto mode = uf::string::lowercase( tag["renderer"]["front face"].as<uf::stl::string>() );
		if ( mode == "cw" ) graphic.descriptor.frontFace = uf::renderer::enums::Face::CW;
		else if ( mode == "ccw" ) graphic.descriptor.frontFace = uf::renderer::enums::Face::CCW;
		else if ( mode == "auto" ) {
			graphic.descriptor.frontFace = graphMetadataJson["renderer"]["invert"].as<bool>(true) ? uf::renderer::enums::Face::CW : uf::renderer::enums::Face::CCW;
		}
		else UF_MSG_WARNING("Invalid Face enum string specified: {}", mode);
	}
	
	// query materials if culling needs to be disabled
	if ( entityName != "worldspawn" ) {
		for ( auto& primitive : primitives ) {
			auto materialID = primitive.instance.materialID;
			if ( 0 <= materialID && materialID <= graph.materials.size() ) {
				auto& materialName = graph.materials[materialID];
				auto& material = storage.materials[materialName];
				if ( material.modeCull == pod::Material::CullMode::NONE ) {
					tag["renderer"]["cull mode"] = "none";
				}
				if ( material.modeAlpha == pod::Material::AlphaMode::BLEND ) {
					graphic.descriptor.renderTarget = 1;
				}
			}
		}
	}

	if ( tag["renderer"]["cull mode"].is<uf::stl::string>() ) {
		const auto mode = uf::string::lowercase( tag["renderer"]["cull mode"].as<uf::stl::string>() );
		if ( mode == "back" ) graphic.descriptor.cullMode = uf::renderer::enums::CullMode::BACK;
		else if ( mode == "front" ) graphic.descriptor.cullMode = uf::renderer::enums::CullMode::FRONT;
		else if ( mode == "none" ) graphic.descriptor.cullMode = uf::renderer::enums::CullMode::NONE;
		else if ( mode == "both" ) graphic.descriptor.cullMode = uf::renderer::enums::CullMode::BOTH;
		else UF_MSG_WARNING("Invalid CullMode enum string specified: {}", mode);
	}

	::bindTextures( graph, graphic );
	::bindShaders( graph, entity, mesh, primitives );
	::bindBuffers( graph, graphic, mesh );
	::bindAddresses( graph, graphic, mesh, primitives );

	graphic.process = true;
	storage.stale = true;
}

void uf::graph::process( pod::Graph& graph ) {
	UF_DEBUG_TIMER_MULTITRACE_START("Processing {}", graph.name);

	// root entity should already be bound, but just in case
	if ( !graph.root.entity ) {
		graph.root.entity = new uf::Object;
	//	UF_MSG_DEBUG("binding root: {}", (void*) graph.root.entity);
	}
	
	// copy lighting settings from graph
	auto& scene = uf::scene::getCurrentScene();
	auto& sceneMetadataJson = scene.getComponent<uf::Serializer>();
	auto& graphMetadataJson = graph.metadata;
	auto& graphMetadataValve = graphMetadataJson["valve"];
	auto& graphMetadataDark = graphMetadataJson["dark"];
	auto& storage = uf::graph::getStorage( graph );

	// set simple on dreamcast for debugging purposes
#if UF_ENV_DREAMCAST
	graphMetadataJson["debug"]["simple"] = true;
#endif
	
	std::lock_guard<std::mutex> lock(*storage.mutex);
	uf::graph::initialize( storage );

	// merge light settings with global settings
	{
		const auto& globalSettings = graphMetadataJson["light"];
		ext::json::forEach( globalSettings, [&]( const uf::stl::string& key, const ext::json::Value& value ){
			if ( !ext::json::isNull( sceneMetadataJson["light"][key] ) ) return;
			sceneMetadataJson["light"][key] = value;
		} );
	}
	// merge bloom settings with global settings
	{
		const auto& globalSettings = graphMetadataJson["light"]["bloom"];
		ext::json::forEach( globalSettings, [&]( const uf::stl::string& key, const ext::json::Value& value ){
			if ( !ext::json::isNull( sceneMetadataJson["light"]["bloom"][key] ) ) return;
			sceneMetadataJson["light"]["bloom"][key] = value;
		} );
	}
	// merge shadows settings with global settings
	{
		const auto& globalSettings = graphMetadataJson["light"]["shadows"];
		ext::json::forEach( globalSettings, [&]( const uf::stl::string& key, const ext::json::Value& value ){
			if ( !ext::json::isNull( sceneMetadataJson["light"]["shadows"][key] ) ) return;
			sceneMetadataJson["light"]["shadows"][key] = value;
		} );
	}
	// merge fog settings with global settings
	{
		const auto& globalSettings = graphMetadataJson["light"]["fog"];
		ext::json::forEach( globalSettings, [&]( const uf::stl::string& key, const ext::json::Value& value ){
			if ( !ext::json::isNull( sceneMetadataJson["light"]["fog"][key] ) ) return;
			sceneMetadataJson["light"]["fog"][key] = value;
		} );
	}

	// stores temporary metadata for textures (that can be deduced at runtime)
	uf::stl::unordered_map<uf::stl::string, TextureDescriptor> textureDescriptors;

	// process lightmap
	UF_DEBUG_TIMER_MULTITRACE("Parsing lightmaps");
	// cringe hack for VBSP loader
	if ( storage.textures.map.count("lightmap_atlas") > 0 ) {
		graphMetadataJson["baking"]["enabled"] = false;
		textureDescriptors["lightmap_atlas"].srgb = false;
		if ( sceneMetadataJson["light"]["lightmaps"].as<bool>(true) ) {
		#if UF_USE_OPENGL && !UF_ENV_DREAMCAST
			::convertLightmap( storage.images["lightmap_atlas"].data );
		#endif
		} else {
			for ( auto& name : graph.primitives ) {
				auto& primitives = storage.primitives[name];
				for ( auto& primitive : primitives ) {
					primitive.instance.auxID = -1;
					primitive.instance.lightmapID = -1;
				}
			}
		}
	// cringer hack for per-cell lightmaps
	} else if ( graphMetadataJson["lightmaps"].isObject() ) {
		graphMetadataJson["baking"]["enabled"] = false;
		if ( sceneMetadataJson["light"]["lightmaps"].as<bool>(true) ) {
			ext::json::forEach( graphMetadataJson["lightmaps"], [&]( const uf::stl::string& key, const ext::json::Value& value ) {
				textureDescriptors[key].srgb = false;
			#if UF_USE_OPENGL && !UF_ENV_DREAMCAST
				::convertLightmap( storage.images[key].data );
			#endif
			});
		} else {
			for ( auto& name : graph.primitives ) {
				auto& primitives = storage.primitives[name];
				for ( auto& primitive : primitives ) {
					primitive.instance.auxID = -1;
					primitive.instance.lightmapID = -1;
				}
			}
		}
	} else {
		constexpr const char* UF_GRAPH_DEFAULT_LIGHTMAP = "./lightmap.%i.png";
		uf::stl::unordered_map<size_t, uf::stl::string> filenames;
		uf::stl::unordered_map<size_t, size_t> lightmapIDs;
		uint32_t lightmapCount = 0;

		for ( auto& name : graph.primitives ) {
			auto& primitives = storage.primitives[name];
			for ( auto& primitive : primitives ) {
				filenames[primitive.instance.auxID] = uf::string::replace(UF_GRAPH_DEFAULT_LIGHTMAP, "%i", ::keyedID(primitive.instance.auxID));

				lightmapCount = std::max( lightmapCount, primitive.instance.auxID + 1 );
			}
		}

		if ( graphMetadataJson["lights"]["lightmap"].is<bool>() && !graphMetadataJson["lights"]["lightmap"].as<bool>() ) {
			graphMetadataJson["baking"]["enabled"] = false;
		}
		if ( !sceneMetadataJson["light"]["lightmaps"].as<bool>(true) ) {
			graphMetadataJson["lights"]["lightmap"] = false;
			graphMetadataJson["baking"]["enabled"] = false;
		}

		if ( graphMetadataJson["lights"]["lightmap"].is<uf::stl::string>() && graphMetadataJson["lights"]["lightmap"].as<uf::stl::string>() == "auto" ) {
			uint32_t mtime = uf::io::mtime( graph.name );
			// lightmaps are considered stale if they're older than the graph's source
			bool stale = false;
			for ( auto& pair : filenames ) {
				uf::stl::string filename = uf::io::resolveURI( pair.second, uf::io::directory( graph.name ) );
				auto time = uf::io::mtime(filename);
				if ( !uf::io::exists( filename ) ) continue;
				if ( time < mtime ) {
					UF_MSG_INFO("Stale lightmap detected, disabling use of lightmaps: {}", filename);
					stale = true;
					break;
				}
			}
			graphMetadataJson["lights"]["lightmap"] = !stale;
		}


		graphMetadataJson["baking"]["layers"] = lightmapCount;
		
		if ( graphMetadataJson["lights"]["lightmap"].as<bool>() ) {
			for ( auto& pair : filenames ) {
				auto i = pair.first;
				auto f = uf::io::resolveURI( pair.second, uf::io::directory( graph.name ) );
				if ( !uf::io::exists( f ) ) {
					graphMetadataJson["lights"]["lightmap"] = false;
					UF_MSG_ERROR( "lightmap does not exist: {} {}, disabling lightmaps", i, f )
					break;
				}
			}
		}
		if ( graphMetadataJson["lights"]["lightmap"].as<bool>() ) {
			for ( auto& pair : filenames ) {
				auto i = pair.first;
				auto f = uf::io::resolveURI( pair.second, uf::io::directory( graph.name ) );

				auto textureID = graph.textures.size();
				auto imageID = graph.images.size();

				auto& texture = storage.textures[graph.textures.emplace_back(f)];
				auto& image = storage.images[graph.images.emplace_back(f)].data;
				if ( !graph.settings.stream.textures ) {
					image.open( f, false );
				}

				texture.index = imageID;

				lightmapIDs[i] = textureID;

				graphMetadataJson["lights"]["lightmaps"][i] = f;
				graphMetadataJson["baking"]["enabled"] = false;

				textureDescriptors[f].srgb = false;
			}
		}
				
		for ( auto& name : graph.primitives ) {
			auto& primitives = storage.primitives[name];
			for ( auto& primitive : primitives ) {
				if ( lightmapIDs.count( primitive.instance.auxID ) == 0 ) continue;
				primitive.instance.lightmapID = lightmapIDs[primitive.instance.auxID];
			}
		}
	}

	// process cubemaps
	ext::json::forEach( graph.metadata["cubemaps"], [&]( const ext::json::Value& value ) {
		auto name = value.as<uf::stl::string>();
		textureDescriptors[name].layers = 6;
	});

	// figure out what texture is what exactly
	UF_DEBUG_TIMER_MULTITRACE("Determining format of textures");
	for ( auto& key : graph.materials ) {
		auto& material = storage.materials[key];

		if ( 0 <= material.indexCubemap && material.indexCubemap < graph.textures.size() ) {
			auto texName = graph.textures[material.indexCubemap];
			textureDescriptors[texName].layers = 6;
			textureDescriptors[texName].srgb = true;
		}

		if ( (0 <= material.indexAlbedo && material.indexAlbedo < graph.textures.size() ) ) {
			auto texName = graph.textures[material.indexAlbedo];
			textureDescriptors[texName].srgb = true;
		}

		if ( (0 <= material.indexNormal && material.indexNormal < graph.textures.size() ) ) {
			auto texName = graph.textures[material.indexNormal];
			textureDescriptors[texName].srgb = false;
		}
	}

	UF_DEBUG_TIMER_MULTITRACE("Processing images...");
	for ( auto& key : graph.images ) {
		auto& image = storage.images[key].data;
		auto& texture = storage.images[key].handle;
		if ( !texture.generated() ) {
			// set as null
			if ( graph.settings.stream.textures ) {
				texture.aliasTexture(uf::renderer::Texture2D::empty);
				continue;
			}

			auto filter = uf::renderer::enums::Filter::LINEAR;
			auto& tag = (graph.metadata["tags"][key] = ext::json::find( key, graphMetadataJson["tags"] ));
			if ( !ext::json::isObject( tag ) ) {
				tag["renderer"] = graphMetadataJson["renderer"];
			}
			if ( tag["renderer"]["filter"].is<uf::stl::string>() ) {
				const auto mode = uf::string::lowercase( tag["renderer"]["filter"].as<uf::stl::string>("linear") );
				if ( mode == "linear" ) filter = uf::renderer::enums::Filter::LINEAR;
				else if ( mode == "nearest" ) filter = uf::renderer::enums::Filter::NEAREST;
				else UF_MSG_WARNING("Invalid Filter enum string specified: {}", mode);
			}

			texture.sampler.descriptor.filter.min = filter;
			texture.sampler.descriptor.filter.mag = filter;
			texture.layers = textureDescriptors[key].layers;
			texture.srgb = textureDescriptors[key].srgb;

			image.size.y /= texture.layers;

			texture.loadFromImage( image );
		#if UF_ENV_DREAMCAST
			image.clear();
		#endif
		}
	}
	
	// process nodes
	UF_DEBUG_TIMER_MULTITRACE("Processing nodes");
	for ( auto index : graph.root.children ) {
		process( graph, index, *graph.root.entity );

		auto& node = graph.nodes[index];
	//	if ( node.entity ) UF_DEBUG_TIMER_MULTITRACE("Processed node: {}", node.name);
	}

	// spawn player if not already spawned
	if ( auto* player = scene.findByName("Player"); !player ) {
		int32_t spawnID = -1;
		uf::stl::vector<int32_t> spawns;
		for ( auto nodeID = 0; nodeID < graph.nodes.size(); ++nodeID ) {
			auto& node = graph.nodes[nodeID];
			if ( node.name == "info_player_start" ) {
				spawnID = nodeID;
			} else if ( node.name.starts_with("info_player_") ) {
				spawns.emplace_back(nodeID);
			}
		}
		if ( spawnID == -1 && !spawns.empty() ) spawnID = uf::stl::random( spawns );
		
		auto& node = spawnID == -1 ? graph.root : graph.nodes[spawnID];
		auto& child = /*graph.root.entity->*/node.entity->loadChild( "./player.json", false ); // to-do: do not hardcode this
		auto& childTransform = child.getComponent<pod::Transform<>>();

		auto flatten = uf::transform::flatten( node.transform );
		flatten.position.y += 1;
		childTransform = flatten;

		graph.settings.stream.player = spawnID;
	}

	// patch materials/textures
	UF_DEBUG_TIMER_MULTITRACE("Patching textures/materials");
	for ( auto& name : graph.materials ) {
		auto& material = storage.materials[name];

		// 
		if ( ext::json::isObject( graphMetadataValve ) ) {
			// nodraw
			if ( name.starts_with("tools/") ) {
				material.colorBase.w = 0.0f;
				material.modeAlpha = pod::Material::AlphaMode::MASK;
				material.factorAlphaCutoff = 1.0f;
				material.indexAlbedo = -1;
			}
		}

		auto& tag = (graph.metadata["tags"][name] = ext::json::find( name, graphMetadataJson["tags"] ));
		if ( ext::json::isObject( tag ) ) {
			material.colorBase = uf::vector::decode( tag["material"]["base"], material.colorBase);
			material.colorEmissive = uf::vector::decode( tag["material"]["emissive"], material.colorEmissive);
			material.factorMetallic = tag["material"]["fMetallic"].as(material.factorMetallic);
			material.factorRoughness = tag["material"]["fRoughness"].as(material.factorRoughness);
			material.factorOcclusion = tag["material"]["fOcclusion"].as(material.factorOcclusion);
			material.factorAlphaCutoff = tag["material"]["fAlphaCutoff"].as(material.factorAlphaCutoff);
			if ( tag["material"]["iAlbedo"].is<uf::stl::string>() ) {
				auto keyName = tag["material"]["iAlbedo"].as<uf::stl::string>();
				if ( storage.textures.map.count(keyName) > 0 ) {
					auto& texture = storage.textures[keyName];
					material.indexAlbedo = tag["material"]["iAlbedo"].as(texture.index);
				}
			} else {
				material.indexAlbedo = tag["material"]["iAlbedo"].as(material.indexAlbedo);
			}
			if ( tag["material"]["iNormal"].is<uf::stl::string>() ) {
				auto keyName = tag["material"]["iNormal"].as<uf::stl::string>();
				if ( storage.textures.map.count(keyName) > 0 ) {
					auto& texture = storage.textures[keyName];
					material.indexNormal = tag["material"]["iNormal"].as(texture.index);
				}
			} else {
				material.indexNormal = tag["material"]["iNormal"].as(material.indexNormal);
			}
			if ( tag["material"]["iEmissive"].is<uf::stl::string>() ) {
				auto keyName = tag["material"]["iEmissive"].as<uf::stl::string>();
				if ( storage.textures.map.count(keyName) > 0 ) {
					auto& texture = storage.textures[keyName];
					material.indexEmissive = tag["material"]["iEmissive"].as(texture.index);
				}
			} else {
				material.indexEmissive = tag["material"]["iEmissive"].as(material.indexEmissive);
			}
			if ( tag["material"]["iOcclusion"].is<uf::stl::string>() ) {
				auto keyName = tag["material"]["iOcclusion"].as<uf::stl::string>();
				if ( storage.textures.map.count(keyName) > 0 ) {
					auto& texture = storage.textures[keyName];
					material.indexOcclusion = tag["material"]["iOcclusion"].as(texture.index);
				}
			} else {
				material.indexOcclusion = tag["material"]["iOcclusion"].as(material.indexOcclusion);
			}
			if ( tag["material"]["iMetallicRoughness"].is<uf::stl::string>() ) {
				auto keyName = tag["material"]["iMetallicRoughness"].as<uf::stl::string>();
				if ( storage.textures.map.count(keyName) > 0 ) {
					auto& texture = storage.textures[keyName];
					material.indexMetallicRoughness = tag["material"]["iMetallicRoughness"].as(texture.index);
				}
			} else {
				material.indexMetallicRoughness = tag["material"]["iMetallicRoughness"].as(material.indexMetallicRoughness);
			}
			
			if ( tag["material"]["modeAlpha"].is<uf::stl::string>() ) {
				const auto mode = uf::string::lowercase( tag["material"]["modeAlpha"].as<uf::stl::string>() );
				if ( mode == "opaque" ) material.modeAlpha = pod::Material::AlphaMode::OPAQUE;
				else if ( mode == "blend" ) material.modeAlpha = pod::Material::AlphaMode::BLEND;
				else if ( mode == "mask" ) material.modeAlpha = pod::Material::AlphaMode::MASK;
				else if ( mode == "emissive" ) material.modeAlpha = pod::Material::AlphaMode::EMISSIVE;
				else UF_MSG_WARNING("Invalid AlphaMode enum string specified: {}", mode);
			} else {
				material.modeAlpha = tag["material"]["modeAlpha"].as(material.modeAlpha);
			}
		}
	}

	// remap textures->images IDs
	UF_DEBUG_TIMER_MULTITRACE("Remapping texture -> image IDs");

	// separate texture2Ds and textureCubes from images
	uf::stl::vector<int32_t> gpuIndex2D(storage.images.keys.size(), -1);
	uf::stl::vector<int32_t> gpuIndexCube(storage.images.keys.size(), -1);

	int32_t count2D = 0;
	int32_t countCube = 0;
	for ( size_t i = 0; i < storage.images.keys.size(); ++i ) {
		auto& key = storage.images.keys[i];
		auto& image = storage.images.map[key].handle;

		if ( image.viewType == uf::renderer::enums::Image::VIEW_TYPE_CUBE ) {
			gpuIndexCube[i] = countCube++;
		} else if ( image.viewType == uf::renderer::enums::Image::VIEW_TYPE_2D ) {
			gpuIndex2D[i] = count2D++;
		} else {
			UF_MSG_DEBUG("Invalid view type 0x{:x} for: {}", image.viewType, key );
		}
	}

	for ( auto& name : graph.textures ) {
		auto& texture = storage.textures[name];
		auto& keys = storage.images.keys;
		auto& indices = storage.images.indices;

		if ( !(0 <= texture.index && texture.index < graph.images.size()) ) continue;

		auto& needle = graph.images[texture.index];
		int32_t storageImageIndex = indices[needle];

		auto& image = storage.images.map[keys[storageImageIndex]].handle;
		if ( image.viewType == uf::renderer::enums::Image::VIEW_TYPE_CUBE ) {
			texture.index = gpuIndexCube[storageImageIndex];
		} else {
			texture.index = gpuIndex2D[storageImageIndex];
		}
	}

	// remap materials->texture IDs
	UF_DEBUG_TIMER_MULTITRACE("Remapping material -> texture IDs");
	for ( auto& name : graph.materials ) {
		auto& material = storage.materials[name];
		auto& keys = storage.textures.keys;
		auto& indices = storage.textures.indices;
		int32_t* IDs[] = { &material.indexAlbedo, &material.indexNormal, &material.indexEmissive, &material.indexOcclusion, &material.indexMetallicRoughness, &material.indexCubemap };
		for ( auto* pointer : IDs ) {
			auto& ID = *pointer;
			if ( !(0 <= ID && ID < graph.textures.size()) ) continue;
			auto& needle = graph.textures[ID];
			ID = indices[needle];
		}
	}

	// remap instance variables
	UF_DEBUG_TIMER_MULTITRACE("Remapping instance -> material IDs");
	for ( auto& name : graph.primitives ) {
		for ( auto& primitive : storage.primitives[name] ) {
			auto& instance = primitive.instance;
			
			if ( 0 <= instance.materialID && instance.materialID < graph.materials.size() ) {
				auto& keys = storage.materials.keys;
				auto& indices = storage.materials.indices;
				
				if ( !(0 <= instance.materialID && instance.materialID < graph.materials.size()) ) continue;

				auto& needle = graph.materials[instance.materialID];
				instance.materialID = indices[needle];
			}
			if ( 0 <= instance.lightmapID && instance.lightmapID < graph.textures.size() ) {
				auto& keys = storage.textures.keys;
				auto& indices = storage.textures.indices;

				if ( !(0 <= instance.lightmapID && instance.lightmapID < graph.textures.size()) ) continue;

				auto& needle = graph.textures[instance.lightmapID];
				instance.lightmapID = indices[needle];
			}
			if ( 0 <= instance.cubemapID && instance.cubemapID < graph.textures.size() ) {
				auto& keys = storage.textures.keys;
				auto& indices = storage.textures.indices;

				if ( !(0 <= instance.cubemapID && instance.cubemapID < graph.textures.size()) ) continue;

				auto& needle = graph.textures[instance.cubemapID];
				instance.cubemapID = indices[needle];
			}
		}

		for ( auto& instance : storage.instances.map[name] ) {
			if ( 0 <= instance.materialID && instance.materialID < graph.materials.size() ) {
				auto& keys = storage.materials.keys;
				auto& indices = storage.materials.indices;
				auto& needle = graph.materials[instance.materialID];
				instance.materialID = indices[needle];
			}
			if ( 0 <= instance.lightmapID && instance.lightmapID < graph.textures.size() ) {
				auto& keys = storage.textures.keys;
				auto& indices = storage.textures.indices;
				auto& needle = graph.textures[instance.lightmapID];
				instance.lightmapID = indices[needle];
			}
			if ( 0 <= instance.cubemapID && instance.cubemapID < graph.textures.size() ) {
				auto& keys = storage.textures.keys;
				auto& indices = storage.textures.indices;
				auto& needle = graph.textures[instance.cubemapID];
				instance.cubemapID = indices[needle];
			}
		}
	}

	UF_DEBUG_TIMER_MULTITRACE("Updating master graph");
#if UF_GRAPH_EXTENDED
	uf::graph::reload( graph );
#endif
	uf::graph::reload();
	
	UF_DEBUG_TIMER_MULTITRACE_END("Processed graph.");
}
void uf::graph::process( pod::Graph& graph, int32_t index, uf::Object& parent ) {

	auto& scene = uf::scene::getCurrentScene();
	auto& storage = uf::graph::getStorage( graph );

	auto& graphMetadataJson = graph.metadata;
	auto& node = graph.nodes[index];
	node.index = index;

	// 
	bool ignore = false;
	// ignore pesky light_Orientation nodes
	if ( uf::string::split( node.name, "_" ).back() == "Orientation" ) ignore = true;

	auto& tag = (node.metadata["tag"] = ext::json::find( node.name, graphMetadataJson["tags"] ));
	if ( ext::json::isObject( tag ) ) {
		if ( graphMetadataJson["baking"]["enabled"].as<bool>(false) && !tag["bake"].as<bool>(true) ) ignore = true;
		if ( tag["ignore"].as<bool>() ) ignore = true;
	}

	if ( ignore ) return;
		
	// create child
	uf::Object* pointer = new uf::Object;
	parent.addChild(*pointer);

	uf::Object& entity = *pointer;
	node.entity = &entity;
	
	auto nameID = FMT_FORMAT( "{}_{}", node.name, index );
	bool setName = entity.getName() == "Entity";
	auto& metadata = entity.getComponent<uf::ObjectBehavior::Metadata>();
	auto& metadataJson = entity.getComponent<uf::Serializer>();
	metadataJson["system"]["graph"]["name"] = node.name;
	metadataJson["system"]["graph"]["index"] = index;

	uf::Serializer loadJson;
	
	// worldspawn
	if ( node.name == "worldspawn" ) {
		graph.settings.stream.world = node.index;
	} else if ( graphMetadataJson["debug"]["simple"].as<bool>() ) {
		node.metadata["valve"] = ext::json::null();
		node.metadata["dark"] = ext::json::null();
	}
	
	// convert metadata["valve"] into internal values:
	auto& metadataValve = node.metadata["valve"];
	if ( ext::json::isObject( metadataValve ) ) {

		// bind io connectivity
		if ( ext::json::isArray( metadataValve["connections"] ) || metadataValve["targetname"].is<uf::stl::string>() ) {
			node.metadata["connections"] = metadataValve["connections"];
			loadJson["assets"].emplace_back("ent://scripts/valve/io.lua");
		}

		// bind ambient
		if ( node.name.starts_with("ambient_") ) {
			loadJson["assets"].emplace_back("ent://scripts/valve/ambient_generic.lua");
			loadJson["behaviors"].emplace_back("AudioEmitterBehavior");
		// bind door script
		} else if ( ext::json::isObject( metadataValve["door"] ) ) {
			node.metadata["door"] = metadataValve["door"];
			loadJson["imports"].emplace_back("ent://door.json");
		}
		// bind prop
		else if ( ( node.name.starts_with("prop_") || node.name == "func_physbox" ) && ( 0 <= node.mesh && node.mesh < graph.meshes.size() ) ) {
			auto& meshName = graph.meshes[node.mesh];

			// get flags
			int spawnflags = metadataValve["spawnflags"].as<int>(0);
			bool motionDisabled = (spawnflags & 8) != 0;
			bool preventPickup = (spawnflags & 512) != 0;

			// get mass
			float baseMass = graph.metadata["valve"]["models"][meshName]["mass"].as<float>(1.0f);
			float massScale = metadataValve["massScale"].as<float>(1.0f);
			// flag as static
			//if ( node.name.starts_with("prop_static") || motionDisabled ) massScale = 0;
			massScale = 0;
			float mass = baseMass * massScale;

			node.metadata["physics"]["type"] = massScale ? "obb" : "mesh";
			node.metadata["physics"]["mass"] = mass;
			node.metadata["physics"]["category"] = massScale ? "dynamic" : "static";

			node.metadata["holdable"] = (mass <= 35.0f) && !motionDisabled && !preventPickup;
		}
		// assume all other funcs are to have a physics body
		else if ( node.name.starts_with("func_") ) {
			if ( ext::json::isNull( node.metadata["physics"] ) ) {
				//node.metadata["physics"]["type"] = "bounding box";
				node.metadata["physics"]["type"] = "mesh";
				node.metadata["physics"]["category"] = "trigger";
			}
		}

		// check if trigger
		if ( node.name.starts_with("trigger_") ) {
			loadJson["assets"].emplace_back("ent://scripts/valve/trigger.lua");
			// signal to assign a physics body
			if ( ext::json::isNull( node.metadata["physics"] ) ) {
				node.metadata["physics"]["type"] = "bounding box";
				node.metadata["physics"]["category"] = "trigger";
			}
		} else if ( 0 <= node.mesh && node.mesh < graph.meshes.size() ) {
			auto& primitives = storage.primitives.map[graph.primitives[node.mesh]];
			for ( auto& primitive : primitives ) {
				auto materialID = primitive.instance.materialID;
				if ( 0 <= materialID && materialID <= graph.materials.size() ) {
					auto& materialName = graph.materials[materialID];
					// attach trigger script + physics body
					if ( materialName == "tools/toolstrigger" ) {
						loadJson["assets"].emplace_back("ent://scripts/valve/trigger.lua");
						// signal to assign a physics body
						if ( ext::json::isNull( node.metadata["physics"] ) ) {
							node.metadata["physics"]["type"] = "bounding box";
							node.metadata["physics"]["category"] = "trigger";
						}
						break;
					}
				}
			}
		}
	}

	// convert metadata["dark"] into internal values:
	auto& metadataDark = node.metadata["dark"];
	if ( ext::json::isObject( metadataDark ) ) {
		bool emitsAudio = false;
	
		// bind elevator
		bool isElevator = false; // node.name.find("Elevator") != std::string::npos
		bool isTrap = false; // node.name.find("Trap") != std::string::npos
		if ( ext::json::isArray( metadataDark["scripts"] ) ) {
			ext::json::forEach( metadataDark["scripts"], [&]( ext::json::Value& value ){
				auto script = value.as<uf::stl::string>();
				if ( script.ends_with("Elevator") ) isElevator = true;
				if ( script.starts_with("Trap") ) isTrap = true;
			});
		}

		if ( metadataDark["class_tags"].is<uf::stl::string>() ) {
			emitsAudio = true;
		} else if ( ext::json::isObject( metadataDark["sound"] ) ) {
			emitsAudio = true;
		}

		// bind schema DB
		if ( ext::json::isArray( metadataDark["schema_db"] ) ) {
			UF_MSG_DEBUG("Binding schema DB: {}", node.name);
			loadJson["assets"].emplace_back("ent://scripts/dark/schema_db.lua");
		}

		// bind io connectivity
		if ( ext::json::isArray( metadataDark["connections"] ) || metadataDark["id"].is<int>() ) {
			node.metadata["connections"] = metadataDark["connections"];
			loadJson["assets"].emplace_back("ent://scripts/dark/io.lua");
			loadJson["assets"].emplace_back("ent://scripts/dark/osm.lua");
		}

		// bind door
		if ( ext::json::isObject( metadataDark["door"] ) ) {
			loadJson["assets"].emplace_back("ent://scripts/dark/door.lua");
			emitsAudio = true;
		}

		if ( isElevator ) {
			loadJson["assets"].emplace_back("ent://scripts/dark/elevator.lua");
			emitsAudio = true;
		}

		// bind songs
		if ( ext::json::isObject( metadataDark["songs"] ) ) {
			loadJson["assets"].emplace_back("ent://scripts/dark/music.lua");
			emitsAudio = true;
		}

		// bind trap
		if ( isTrap ) {
			loadJson["assets"].emplace_back("ent://scripts/dark/trap.lua");
			emitsAudio = true;
		// bind ambient
		} else if ( ext::json::isObject( metadataDark["sound"] ) ) {
			loadJson["assets"].emplace_back("ent://scripts/dark/ambient.lua");
			emitsAudio = true;
		}

		// bind trigger
		auto& physMeta = node.metadata["physics"];
		if ( ext::json::isObject( physMeta ) && physMeta["category"].as<uf::stl::string>("") == "trigger" ) {
			loadJson["assets"].emplace_back("ent://scripts/dark/trigger.lua");
		}

		if ( emitsAudio ) loadJson["behaviors"].emplace_back("AudioEmitterBehavior");
	}

	if ( ext::json::isObject( tag ) ) {
		if ( tag["action"].as<uf::stl::string>() == "load" ) {
			if ( tag["filename"].is<uf::stl::string>() ) {
				uf::stl::string filename = uf::io::resolveURI( tag["filename"].as<uf::stl::string>(), graphMetadataJson["root"].as<uf::stl::string>() );
				entity.load(filename);
			} else if ( ext::json::isObject( tag["payload"] ) ) {
				uf::Serializer json = tag["payload"];
				json["root"] = graphMetadataJson["root"];
				json.import( loadJson );
				entity.load(json);
			}
		} else if ( tag["action"].as<uf::stl::string>() == "attach" ) {
			uf::stl::string filename = uf::io::resolveURI( tag["filename"].as<uf::stl::string>(), graphMetadataJson["root"].as<uf::stl::string>() );
			auto& child = entity.loadChild( filename, false );
			auto& childTransform = child.getComponent<pod::Transform<>>();
			auto& childMetadataJson = child.getComponent<uf::Serializer>();

			auto flatten = uf::transform::flatten( node.transform );
			childTransform = flatten;
		}
		if ( tag["static"].is<bool>() ) {
			metadata.system.ignoreGraph = tag["static"].as<bool>();
		}
	} else if ( ext::json::isObject( loadJson ) ) {
		loadJson["root"] = graphMetadataJson["root"];
		entity.load( loadJson );
	}

	// import metadata
	metadataJson.import( node.metadata );

	// create as light
	{
		// attempt to resolve a light name
		auto lightName = node.name;
		if ( graph.lights.count( nameID ) > 0 ) lightName = nameID;
		if ( graph.lights.count( lightName ) > 0 ) {
			auto& l = graph.lights[lightName];
			
		#if UF_USE_OPENGL
			metadata.system.ignoreGraph = true;
		#else
			metadata.system.ignoreGraph = graphMetadataJson["debug"]["static"].as<bool>();
		#endif
			const float LIGHT_POWER_CUTOFF = 0.005f;
			float power = l.intensity * graphMetadataJson["lights"]["scale"].as<float>(1);
			float range = l.range;
			if ( range <= 0.0f ) {
				if ( power > LIGHT_POWER_CUTOFF ) {
					range = std::sqrt( (power / LIGHT_POWER_CUTOFF) - 1.0f );
				} else {
					range = 0.001f;
				}
			}
			
			uf::Serializer metadataLight;
			metadataLight["radius"][0] = range;
			metadataLight["radius"][1] = 0.001;
			metadataLight["power"] = power;

			metadataLight["color"][0] = l.color.x;
			metadataLight["color"][1] = l.color.y;
			metadataLight["color"][2] = l.color.z;

			metadataLight["shadows"] = graphMetadataJson["lights"]["shadows"].as<bool>();
			metadataLight["dynamic"] = false;

		/*
			if ( uf::string::matched( node.name, R"(/\bspot\b/)" ) ) {
				metadataLight["type"] = "spot";
			}
		*/
		/*
			if ( ext::json::isArray( graphMetadataJson["lights"]["radius"] ) ) {
				metadataLight["radius"] = graphMetadataJson["lights"]["radius"];
			}
		*/
			if ( graphMetadataJson["lights"]["bias"].is<float>() ) {
				metadataLight["bias"] = graphMetadataJson["lights"]["bias"].as<float>();
			}
			// copy from tag information
			ext::json::forEach( graphMetadataJson["tags"][node.name]["light"], [&]( const uf::stl::string& key, ext::json::Value& value ){
				if ( key == "transform" ) return;
				metadataLight[key] = value;
			});

			auto& metadataJson = entity.getComponent<uf::Serializer>();
			entity.load("/light.json");
			// copy

			ext::json::forEach( metadataLight, [&]( const uf::stl::string& key, ext::json::Value& value ) {
				metadataJson["light"][key] = value;
			});
		}
	}

	// set name
	if ( setName ) {
		entity.setName( node.name );
	}

	// reference transform to parent
	auto& transform = entity.getComponent<pod::Transform<>>();
	{
		transform = node.transform;
		if ( node.metadata["debug"]["parent node transforms"].as<bool>(true) ) {
			transform.reference = &parent.getComponent<pod::Transform<>>();
		}
		// override transform
		if ( tag["transform"]["offset"].as<bool>() ) {
			auto parsed = uf::transform::decode( tag["transform"], pod::Transform<>{} );
			transform.position += parsed.position;
			transform.orientation = uf::quaternion::multiply( parsed.orientation, transform.orientation );
		} else {
			transform = uf::transform::decode( tag["transform"], transform );
			if ( tag["transform"]["parent"].is<uf::stl::string>() ) {
				auto* parentPointer = uf::graph::find( graph, tag["transform"]["parent"].as<uf::stl::string>() );
				if ( parentPointer ) {
					auto& parentNode = *parentPointer;
					// entity already exists, bind to its transform
					if ( parentNode.entity && parentNode.entity->hasComponent<pod::Transform<>>() ) {
						auto& parentTransform = parentNode.entity->getComponent<pod::Transform<>>();
						transform = uf::transform::reference( transform, parentTransform, tag["transform"]["reorient"].as<bool>() );
						transform.position = -transform.position;
					// doesnt exist, bind to the node transform
					} else {
						transform = uf::transform::reference( transform, parentNode.transform, tag["transform"]["reorient"].as<bool>() );
					}
				}
			}
		}
	}

	// temporarily disable models for non-worldspawn
	if ( graphMetadataJson["debug"]["simple"].as<bool>() && node.name != "worldspawn" ) node.mesh = -1;

	// 
	if ( 0 <= node.mesh && node.mesh < graph.meshes.size() ) {
		auto model = uf::transform::model( transform );
		node.object = ::allocateObjectID( storage );
		auto objectKeyName = ::keyedID( node.object );

		storage.entities[objectKeyName] = &entity;	
		storage.objects[objectKeyName] = pod::Instance::Object{
			.model = model,
			.previous = model,
			.color = {1, 1, 1, 1},
		};

		if ( node.skin >= 0 && node.skin < graph.skins.size() ) {
			storage.joints[objectKeyName] = {};
		}

		auto& mesh = storage.meshes.map[graph.meshes[node.mesh]];
		auto primitiveName = graph.primitives[node.mesh];
		auto& primitives = storage.primitives.map[primitiveName];

		pod::Instance::Bounds bounds = {};

		auto& grouped = storage.instances[primitiveName];
		for ( auto drawID = 0; drawID < primitives.size(); ++drawID ) {
			pod::Instance newInstance = primitives[drawID].instance;
			newInstance.objectID = node.object;
			if ( node.skin >= 0 && node.skin < graph.skins.size() ) {
				newInstance.jointID = ::allocateJointID( storage, ::keyedID(node.object) );
			} else {
				newInstance.jointID = -1;
			}

			bounds.min = uf::vector::min( bounds.min, newInstance.bounds.min );
			bounds.max = uf::vector::max( bounds.max, newInstance.bounds.max );

			grouped.emplace_back(newInstance);
		}

		bounds.center = (bounds.max + bounds.min) * 0.5f;
		bounds.extent = uf::vector::abs(bounds.max - bounds.min) * 0.5f;

	#if !UF_GRAPH_EXTENDED
		bool isFirstInstance = ( grouped.size() == primitives.size() );
		bool isSkinned = graphMetadataJson["renderer"]["skinned"].as<bool>();
		bool shouldInitializeRender = graphMetadataJson["renderer"]["render"].as<bool>();

		if ( shouldInitializeRender && (isFirstInstance || isSkinned) ) {
			uf::graph::initializeGraphics( graph, entity, mesh, primitives );
		}
	#endif
		
		{
			auto phyziks = tag["physics"];
			if ( !ext::json::isObject( phyziks ) ) phyziks = metadataJson["physics"];
			else metadataJson["physics"] = phyziks;
			
			if ( ext::json::isObject( phyziks ) ) {
				uf::stl::string type = phyziks["type"].as<uf::stl::string>();
				bool isMesh = type == "mesh" || type == "hull";
				if ( !isMesh ) {
					if ( ext::json::isNull( metadataJson["physics"]["center"] ) ) metadataJson["physics"]["center"] = uf::vector::encode( bounds.center );
					if ( ext::json::isNull( metadataJson["physics"]["extent"] ) ) metadataJson["physics"]["extent"] = uf::vector::encode( bounds.extent );
					if ( ext::json::isNull( metadataJson["physics"]["min"] ) ) metadataJson["physics"]["min"] = uf::vector::encode( bounds.min );
					if ( ext::json::isNull( metadataJson["physics"]["max"] ) ) metadataJson["physics"]["max"] = uf::vector::encode( bounds.max );
				}
			#if !UF_GRAPH_EXTENDED
				if ( isMesh ) {
					float mass = phyziks["mass"].as(0.0f);
					auto center = uf::vector::decode( phyziks["center"], pod::Vector3f{} );

					auto& body = uf::physics::create( entity, mass, center );
					uf::physics::initialize( body, mesh, type != "mesh" );

					body.material.staticFriction = phyziks["friction"].as(body.material.staticFriction);
					body.material.restitution = phyziks["restitution"].as(body.material.restitution);
					body.inverseInertiaTensor = uf::vector::decode( phyziks["inertia"], body.inverseInertiaTensor );
					body.gravity = uf::vector::decode( phyziks["gravity"], body.gravity );
				}
			#endif
			}
		}
	}

	for ( auto childIndex : node.children ) {
		uf::graph::process( graph, childIndex, entity );
		graph.nodes[childIndex].parent = index;
	}
}

void uf::graph::destroy( pod::Graph& graph ) {
}

void uf::graph::initialize() {
	uf::graph::initialize( uf::scene::getCurrentScene() );
}
void uf::graph::initialize( uf::Object& object, size_t initialElements ) {
	auto& storage = uf::graph::getStorage( object );
	return uf::graph::initialize( storage, initialElements );
}
void uf::graph::initialize( pod::Graph::Storage& storage, size_t initialElements ) {
	if ( !storage.buffers.camera.buffer ) storage.buffers.camera.initialize( (const void*) nullptr, sizeof(pod::Camera::Viewports), uf::renderer::enums::Buffer::UNIFORM );
	// to-do: check if opengl really needs these
	if ( !storage.buffers.drawCommands.buffer ) storage.buffers.drawCommands.initialize( (const void*) nullptr, sizeof(pod::DrawCommand)  * initialElements, uf::renderer::enums::Buffer::STORAGE );
	if ( !storage.buffers.instance.buffer ) storage.buffers.instance.initialize( (const void*) nullptr, sizeof(pod::Instance) * initialElements, uf::renderer::enums::Buffer::STORAGE );
	if ( !storage.buffers.addresses.buffer ) storage.buffers.addresses.initialize( (const void*) nullptr, sizeof(pod::Instance::Addresses) * initialElements, uf::renderer::enums::Buffer::STORAGE );
	if ( !storage.buffers.lodMetadata.buffer ) storage.buffers.lodMetadata.initialize( (const void*) nullptr, sizeof(pod::LODMetadata) * initialElements, uf::renderer::enums::Buffer::STORAGE );
	if ( !storage.buffers.joint.buffer ) storage.buffers.joint.initialize( (const void*) nullptr, sizeof(pod::Matrix4f) * initialElements, uf::renderer::enums::Buffer::STORAGE );
	if ( !storage.buffers.object.buffer ) storage.buffers.object.initialize( (const void*) nullptr, sizeof(pod::Instance::Object) * initialElements, uf::renderer::enums::Buffer::STORAGE );
	if ( !storage.buffers.material.buffer ) storage.buffers.material.initialize( (const void*) nullptr, sizeof(pod::Material) * initialElements, uf::renderer::enums::Buffer::STORAGE );
	if ( !storage.buffers.texture.buffer ) storage.buffers.texture.initialize( (const void*) nullptr, sizeof(pod::Texture) * initialElements, uf::renderer::enums::Buffer::STORAGE );
	if ( !storage.buffers.light.buffer ) storage.buffers.light.initialize( (const void*) nullptr, sizeof(pod::Light) * initialElements, uf::renderer::enums::Buffer::STORAGE );
}

void uf::graph::initialize( pod::Graph& graph ) {
	if ( graph.metadata["baking"]["enabled"].as<bool>() ) {
		auto& metadataJson = graph.root.entity->getComponent<uf::Serializer>();
		metadataJson["baking"] = graph.metadata["baking"];
		metadataJson["baking"]["root"] = uf::io::directory( graph.name );
		uf::instantiator::bind( "BakingBehavior", *graph.root.entity );
	}

	if ( !graph.root.entity->isValid() ) graph.root.entity->initialize();
	graph.root.entity->process([&]( uf::Entity* entity ) {
		if ( !entity->isValid() ) entity->initialize();
	});

	auto& graphMetadataJson = graph.metadata;
	for ( auto& node : graph.nodes ) {
		if ( node.skin < 0 || node.mesh < 0 ) continue;
		auto& tag = node.metadata["tag"];
		if ( ext::json::isNull( tag ) ) tag["physics"] = graphMetadataJson["physics"];
		if ( tag["physics"]["ragdoll"].as<bool>(false) ) {
			uf::graph::rigRagdoll( graph, node );
		}
	}

	auto& scene = uf::scene::getCurrentScene();
	scene.invalidateGraph();
}

void uf::graph::tick() {
	auto& scene = uf::scene::getCurrentScene();

	// tick only one graph if scene/global
	switch ( uf::graph::storageMode ) {
		case pod::Graph::Storage::GLOBAL: {
			auto& storage = uf::graph::globalStorage;
			storage.shouldRebind = uf::graph::tick( storage );
			return;
		} break;
		case pod::Graph::Storage::SCENE:{
			return uf::graph::tick( scene );
		} break;
	}

	// tick per entity
	auto/*&*/ graph = scene.getGraph();
	for ( auto entity : graph ) {
		if ( !entity->hasComponent<pod::Graph>() ) continue;
		uf::graph::tick( *entity );
	}
}
void uf::graph::tick( uf::Object& object ) {
	auto& storage = uf::graph::getStorage( object );
	// ! NOTE ! additionally, uncommenting these out also breaks things
//	if ( !object.hasComponent<pod::Graph>() ) return;
//	auto& graph = object.getComponent<pod::Graph>();
//	if ( !graph.root.entity || !graph.root.entity->isValid() ) return;
	storage.shouldRebind = uf::graph::tick( storage );
}
bool uf::graph::tick( pod::Graph::Storage& storage ) {
	std::lock_guard<std::mutex> lock(*storage.mutex);
	bool rebuild = false;

	STATIC_THREAD_LOCAL(uf::stl::vector<pod::Instance>, instances);
	STATIC_THREAD_LOCAL(uf::stl::vector<pod::Instance::Addresses>, addresses);
	STATIC_THREAD_LOCAL(uf::stl::vector<pod::LODMetadata>, lodMetadata);
	STATIC_THREAD_LOCAL(uf::stl::vector<pod::Matrix4f>, joints);
	STATIC_THREAD_LOCAL(uf::stl::vector<pod::Instance::Object>, objects);
	STATIC_THREAD_LOCAL(uf::stl::vector<pod::Material>, materials);
	STATIC_THREAD_LOCAL(uf::stl::vector<pod::Texture>, textures);
	STATIC_THREAD_LOCAL(uf::stl::vector<pod::DrawCommand>, drawCommands);

	for ( auto& key : storage.joints.keys ) {
		joints.insert( joints.end(), storage.joints.map[key].begin(), storage.joints.map[key].end() );
	}

	for ( auto& key : storage.objects.keys ) {
		auto& entity = *storage.entities.map[key];
		auto& object = storage.objects.map[key];

		if ( entity.hasComponent<pod::Transform<>>() ) {
			auto& metadata = entity.getComponent<uf::ObjectBehavior::Metadata>();
			auto& transform = entity.getComponent<pod::Transform<>>();
			
			object.previous = object.model;
			object.model = uf::transform::model( transform );
		}

		objects.emplace_back( object );
	}

	if ( !joints.empty() ) rebuild = storage.buffers.joint.update( (const void*) joints.data(), joints.size() * sizeof(pod::Matrix4f) ) || rebuild;
	rebuild = storage.buffers.object.update( (const void*) objects.data(), objects.size() * sizeof(pod::Instance::Object) ) || rebuild;

	if ( storage.stale ) {
		storage.flattenedPrimitives.clear();

		for ( auto& key : storage.primitives.keys ) {
			auto& primitives = storage.primitives.map[key];
			auto& grouped = storage.instances.map[key];

			auto& mesh = storage.meshes.map[key];
			pod::DrawCommand* commands = nullptr;

			if ( mesh.indirect.count > 0 ) {
				auto& attr = mesh.indirect.attributes.front();
				commands = (pod::DrawCommand*) mesh.buffers[attr.buffer].data();
			}

			size_t count = primitives.empty() ? 0 : grouped.size() / primitives.size();
			for ( size_t drawID = 0; drawID < primitives.size(); ++drawID ) {
				auto& primitive = primitives[drawID];

				primitive.drawCommand.instanceID = instances.size();
				primitive.drawCommand.instances = count;

				if ( commands ) {
					commands[drawID].instanceID = primitive.drawCommand.instanceID;
					commands[drawID].instances = primitive.drawCommand.instances;
				}

				drawCommands.emplace_back( primitive.drawCommand );
				lodMetadata.emplace_back( primitive.lod );

				for ( size_t i = 0; i < count; ++i ) {
					size_t strideIndex = (i * primitives.size()) + drawID;

					auto& p = storage.flattenedPrimitives.emplace_back( primitive );
					p.instance = instances.emplace_back( grouped[strideIndex] );
					addresses.emplace_back( primitive.addresses );
				}
			}

		#if UF_USE_VULKAN
			if ( commands && !grouped.empty() ) {
				uf::stl::unordered_set<uf::Object*> updatedGraphics;

				for ( auto& instance : grouped ) {
					auto objectKeyName = ::keyedID(instance.objectID);
					if ( storage.entities.map.count(objectKeyName) == 0 ) continue;
					auto& entity = *storage.entities.map[objectKeyName];
					if ( !entity.hasComponent<uf::renderer::Graphic>() ) continue;
					if ( updatedGraphics.find(&entity) != updatedGraphics.end() ) continue;

					auto& graphic = entity.getComponent<uf::renderer::Graphic>();
					auto& attr = mesh.indirect.attributes.front();

					graphic.updateBuffer( (const void*) attr.pointer, attr.length, graphic.metadata.buffers["indirect["+attr.descriptor.name+"]"] );

					updatedGraphics.insert(&entity);
				}
			}
		#endif
		}

		for ( auto& key : storage.textures.keys ) textures.emplace_back( storage.textures.map[key] );

		for ( auto& key : storage.materials.keys ) materials.emplace_back( storage.materials.map[key] );

		rebuild = storage.buffers.instance.update( (const void*) instances.data(), instances.size() * sizeof(pod::Instance) ) || rebuild;
		rebuild = storage.buffers.addresses.update( (const void*) addresses.data(), addresses.size() * sizeof(pod::Instance::Addresses) ) || rebuild;
		rebuild = storage.buffers.drawCommands.update( (const void*) drawCommands.data(), drawCommands.size() * sizeof(pod::DrawCommand) ) || rebuild;
		rebuild = storage.buffers.lodMetadata.update( (const void*) lodMetadata.data(), lodMetadata.size() * sizeof(pod::LODMetadata) ) || rebuild;
		rebuild = storage.buffers.material.update( (const void*) materials.data(), materials.size() * sizeof(pod::Material) ) || rebuild;
		rebuild = storage.buffers.texture.update( (const void*) textures.data(), textures.size() * sizeof(pod::Texture) ) || rebuild;

		storage.stale = false;
	}


	if ( rebuild ) {
		UF_MSG_DEBUG("Graph buffers requesting renderer update");
		uf::renderer::states::rebuild = true;

	#if UF_USE_VULKAN
		if ( uf::renderer::hasRenderMode("", true) ) {
			auto& renderMode = uf::renderer::getRenderMode("", true);
			auto& blitter = renderMode.getBlitter();
			if ( blitter.material.hasShader("compute", "deferred") || blitter.material.hasShader("fragment", "deferred") ) {
				auto& shader = blitter.material.getShader(blitter.material.hasShader("compute", "deferred") ? "compute" : "fragment", "deferred");

				shader.metadata.aliases.buffers.clear();

				shader.aliasBuffer( "drawCommands", storage.buffers.drawCommands );
				shader.aliasBuffer( "instance", storage.buffers.instance );
				shader.aliasBuffer( "addresses", storage.buffers.addresses );
				shader.aliasBuffer( "material", storage.buffers.material );
				shader.aliasBuffer( "texture", storage.buffers.texture );
				shader.aliasBuffer( "light", storage.buffers.light );
			}
		}
	#endif
	}

	return rebuild;
}

void uf::graph::aggregate() {
	return uf::graph::aggregate( uf::scene::getCurrentScene(), uf::graph::globalStorage );
}
void uf::graph::aggregate( uf::Object& object, pod::Graph::Storage& storage ) {
	STATIC_THREAD_LOCAL(uf::stl::vector<pod::Instance>, instances);
	STATIC_THREAD_LOCAL(uf::stl::vector<pod::Instance::Addresses>, addresses);
	STATIC_THREAD_LOCAL(uf::stl::vector<pod::LODMetadata>, lodMetadata);
	STATIC_THREAD_LOCAL(uf::stl::vector<pod::Matrix4f>, joints);
	STATIC_THREAD_LOCAL(uf::stl::vector<pod::Instance::Object>, objects);
	STATIC_THREAD_LOCAL(uf::stl::vector<pod::Material>, materials);
	STATIC_THREAD_LOCAL(uf::stl::vector<pod::Texture>, textures);
	STATIC_THREAD_LOCAL(uf::stl::vector<pod::DrawCommand>, drawCommands);

	auto entities = object.as<uf::Scene>().getGraph();
	for ( auto entity : entities ) {
		if ( !entity->hasComponent<pod::Graph>() || !entity->hasComponent<pod::Graph::Storage>() ) continue;

		auto& storage = entity->getComponent<pod::Graph::Storage>();

		uint32_t offsetInstances = instances.size();
		uint32_t offsetObjects   = objects.size();
		uint32_t offsetMaterials = materials.size();
		uint32_t offsetTextures  = textures.size();
		int32_t  offsetJoints	 = joints.size();

		for ( auto& key : storage.objects.keys ) {
			objects.emplace_back( storage.objects.map[key] );
		}

		for ( auto& key : storage.textures.keys ) {
			auto& tex = storage.textures.map[key];
			textures.emplace_back( tex );
		}

		for ( auto& key : storage.materials.keys ) {
			auto mat = storage.materials.map[key];
			if ( mat.indexAlbedo >= 0 ) mat.indexAlbedo += offsetTextures;
			if ( mat.indexNormal >= 0 ) mat.indexNormal += offsetTextures;
			if ( mat.indexEmissive >= 0 ) mat.indexEmissive += offsetTextures;
			if ( mat.indexOcclusion >= 0 ) mat.indexOcclusion += offsetTextures;
			if ( mat.indexMetallicRoughness >= 0 ) mat.indexMetallicRoughness += offsetTextures;
			materials.emplace_back( mat );
		}

		for ( auto& key : storage.joints.keys ) {
			joints.insert( joints.end(), storage.joints.map[key].begin(), storage.joints.map[key].end() );
		}

		for ( auto& key : storage.primitives.keys ) {
			for ( auto primitive : storage.primitives.map[key] ) {
				primitive.instance.materialID += offsetMaterials;
				primitive.instance.objectID += offsetObjects;

				if ( primitive.instance.jointID >= 0 ) primitive.instance.jointID += offsetJoints;
				if ( primitive.instance.lightmapID >= 0 ) primitive.instance.lightmapID += offsetTextures;

				primitive.drawCommand.instanceID += offsetInstances;

				drawCommands.emplace_back( primitive.drawCommand );
				instances.emplace_back( primitive.instance );
				lodMetadata.emplace_back( primitive.lod );
				addresses.emplace_back( primitive.addresses );
			}
		}
	}

	bool rebuild = false;
	rebuild = storage.buffers.instance.update( (const void*) instances.data(), instances.size() * sizeof(pod::Instance) ) || rebuild;
	rebuild = storage.buffers.addresses.update( (const void*) addresses.data(), addresses.size() * sizeof(pod::Instance::Addresses) ) || rebuild;
	rebuild = storage.buffers.drawCommands.update( (const void*) drawCommands.data(), drawCommands.size() * sizeof(pod::DrawCommand) ) || rebuild;
	rebuild = storage.buffers.lodMetadata.update( (const void*) lodMetadata.data(), lodMetadata.size() * sizeof(pod::LODMetadata) ) || rebuild;
	rebuild = storage.buffers.material.update( (const void*) materials.data(), materials.size() * sizeof(pod::Material) ) || rebuild;
	rebuild = storage.buffers.texture.update( (const void*) textures.data(), textures.size() * sizeof(pod::Texture) ) || rebuild;
	rebuild = storage.buffers.object.update( (const void*) objects.data(), objects.size() * sizeof(pod::Instance::Object) ) || rebuild;

	if ( !joints.empty() ) {
		rebuild = storage.buffers.joint.update( (const void*) joints.data(), joints.size() * sizeof(pod::Matrix4f) ) || rebuild;
	}

	if ( rebuild ) {
		uf::renderer::states::rebuild = true;
	}
}

void uf::graph::render() {
	auto& scene = uf::scene::getCurrentScene();

	// render only one graph if scene/global
	switch ( uf::graph::storageMode ) {
		case pod::Graph::Storage::GLOBAL: {
			auto& storage = uf::graph::globalStorage;
			uf::graph::render( storage );
			return;
		} break;
		case pod::Graph::Storage::SCENE: {
			return uf::graph::render( scene );
		} break;
	}

	// render per entity
	auto/*&*/ graph = scene.getGraph();
	for ( auto entity : graph ) {
		if ( !entity->hasComponent<pod::Graph>() ) continue;
		uf::graph::render( *entity );
	}
}
void uf::graph::render( uf::Object& object ) {
	auto& storage = uf::graph::getStorage( object );
	return uf::graph::render( storage );
}
void uf::graph::render( pod::Graph::Storage& storage ) {	
	auto* renderMode = uf::renderer::getCurrentRenderMode();

	if ( renderMode->getName() == "Gui" ) return;

	auto& scene = uf::scene::getCurrentScene();
	auto& controller = scene.getController();
	auto& camera = scene.getCamera( controller );

	auto viewport = camera.data().viewport;
#if UF_USE_FFX_FSR || UF_USE_FFX_SDK
	if ( ext::fsr::initialized && renderMode->getType() == "Deferred" ) {
		auto jitter = ext::fsr::getJitterMatrix();
		for ( auto i = 0; i < uf::camera::maxViews; ++i ) {
			viewport.matrices[i].projection = jitter * viewport.matrices[i].projection;
		}
	}
#endif

	storage.buffers.camera.update( (const void*) &viewport, sizeof(pod::Camera::Viewports) );

#if UF_USE_VULKAN
	if ( !renderMode || !renderMode->hasBuffer("camera") || renderMode->getType() == "Swapchain" ) return;
	auto& buffer = renderMode->getBuffer("camera");
	buffer.update( (const void*) &viewport, sizeof(pod::Camera::Viewports) );
#endif
}
void uf::graph::destroy( bool soft ) {
	soft = false;
	return uf::graph::destroy( uf::scene::getCurrentScene(), soft );
}
void uf::graph::destroy( uf::Object& object, bool soft ) {
	soft = false;
	auto& storage = uf::graph::getStorage( object );
	return uf::graph::destroy( storage, soft );
}
void uf::graph::destroy( pod::Graph::Storage& storage, bool soft ) {
	soft = false;
#if UF_USE_VULKAN
/*
	for ( auto& texture : uf::renderer::gc::textures ) {
		texture.destroy( false );
	}
	uf::renderer::gc::textures.clear();
*/
#endif

	// cleanup graphic handles
	for ( auto pair : storage.images.map ) {
		pair.second.data.clear();
		pair.second.handle.destroy();
	}
	for ( auto& t : storage.shadow2Ds ) t.destroy();
	for ( auto& t : storage.shadowCubes ) t.destroy();

	for ( auto pair : storage.atlases.map ) pair.second.clear();
	for ( auto pair : storage.meshes.map ) pair.second.destroy();

	// cleanup storage cache
	storage.primitives.clear();
	storage.meshes.clear();
	storage.images.clear();
	storage.materials.clear();
	storage.textures.clear();
	storage.samplers.clear();
	storage.skins.clear();
	storage.animations.clear();
	storage.atlases.clear();
	storage.joints.clear();
	storage.entities.clear();
	storage.shadow2Ds.clear();
	storage.shadowCubes.clear();

	// cleanup storage buffers
	if ( !soft ) {
		storage.buffers.camera.destroy(true);
		storage.buffers.drawCommands.destroy(true);
		storage.buffers.lodMetadata.destroy(true);
		storage.buffers.instance.destroy(true);
		storage.buffers.addresses.destroy(true);
		storage.buffers.joint.destroy(true);
		storage.buffers.object.destroy(true);
		storage.buffers.material.destroy(true);
		storage.buffers.texture.destroy(true);
		storage.buffers.light.destroy(true);
		storage.buffers.depthPyramid.destroy(true);
	}

	uf::renderer::states::rebuild = true;
}

void uf::graph::reload( pod::Graph& graph, pod::Node& node ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& storage = uf::graph::getStorage( graph );
	auto& graphMetadataJson = graph.metadata;

	auto& entity = node.entity->as<uf::Object>();
	auto& metadataJson = entity.getComponent<uf::Serializer>();
	auto& primitives = storage.primitives.map[graph.primitives[node.mesh]];
	auto& tag = node.metadata["tag"];

	auto meshName = graph.meshes[node.mesh];
	auto& mesh = storage.meshes.map[meshName];

	bool graphicOwner = graphMetadataJson["renderer"]["render"].as<bool>();
	bool isSkinned = graphMetadataJson["renderer"]["skinned"].as<bool>();
	if ( graphicOwner ) {
		if ( isSkinned ) {
			graphicOwner = true;
		} else {
			auto objectKeyName = ::keyedID(storage.instances.map[graph.primitives[node.mesh]].front().objectID);
			graphicOwner = storage.entities[objectKeyName] == &entity;
		}
	}

	if ( graphicOwner ) {
		bool exists = entity.hasComponent<uf::renderer::Graphic>();
		if ( exists ) {
			auto& graphic = entity.getComponent<uf::renderer::Graphic>();
			bool rebuild = graphic.updateMesh( mesh );
			if ( ::bindTextures( graph, graphic ) ) {
				rebuild = true;
			}
			if ( rebuild ) {
			#if !UF_USE_OPENGL
				::bindBuffers( graph, graphic, mesh );
				::bindAddresses( graph, graphic, mesh, primitives );
			#endif
				uf::renderer::states::rebuild = true;
				storage.stale = true;
			}
		} else {
			uf::graph::initializeGraphics( graph, entity, mesh, primitives );
		}
	}

	// bind mesh to physics state
	{
		auto& phyziks = tag["physics"];
		if ( !ext::json::isObject( phyziks ) ) phyziks = metadataJson["physics"];
		else metadataJson["physics"] = phyziks;

		if ( ext::json::isObject( phyziks ) ) {
			uf::stl::string type = phyziks["type"].as<uf::stl::string>();
			bool isMesh = type == "mesh" || type == "hull";
			// cringe
			if ( isMesh ) {	
				auto& bvh = storage.bvhs[meshName];
				float mass = phyziks["mass"].as(0.0f);
				auto created = entity.hasComponent<pod::PhysicsBody>();
				auto& body = entity.getComponent<pod::PhysicsBody>();
				if ( !created ) {
					auto center = uf::vector::decode( phyziks["center"], pod::Vector3f{} );
					uf::physics::create( entity, mass, center );
				}
				bool initialized = false;
				// to-do: find a better initialization marker
				switch ( body.collider.type ) {
					case pod::ShapeType::MESH:
					case pod::ShapeType::CONVEX_HULL:
						initialized = true;
					break;
				}
				if ( !initialized ) {
					uf::physics::initialize( body, mesh, bvh, type != "mesh" );

					body.material.staticFriction = phyziks["friction"].as(body.material.staticFriction);
					body.material.restitution = phyziks["restitution"].as(body.material.restitution);
					body.inverseInertiaTensor = uf::vector::decode( phyziks["inertia"], body.inverseInertiaTensor );
					body.gravity = uf::vector::decode( phyziks["gravity"], body.gravity );
				} else {
					uf::physics::update( body );
				}
			}
		}
	}
}
void uf::graph::reload( pod::Graph& graph ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& storage = uf::graph::getStorage( graph );
	auto& graphMetadataJson = graph.metadata;

	pod::Vector3f controllerPosition = {};
	auto& controller = scene.getController();

	// bind if there's a non-scene controller
	if ( controller.getUid() != scene.getUid() ) {
		auto& controllerTransform = controller.getComponent<pod::Transform<>>();
		controllerPosition = controllerTransform.position;
	// fallback to spawn node
	} else if ( 0 <= graph.settings.stream.player ) {
		auto& node = graph.nodes[graph.settings.stream.player];
		controllerPosition = node.transform.position;
	}

	uf::stl::unordered_map<int32_t, TextureDescriptor> textureDescriptors;
	uf::stl::unordered_map<int32_t, uf::stl::vector<int32_t>> textureDependentNodes;

	struct PendingMeshWork {
		bool needsFullLoad = false;
		bool needsSparseUpdate = false;
		bool needsFallbackBinding = false;
		bool needsBvhLoad = false;
		uf::stl::vector<int8_t> queuedLODs;
		uf::stl::vector<int32_t> dependentNodes;
	};
	uf::stl::unordered_map<int32_t, PendingMeshWork> pendingMeshes;

	auto oldHash = graph.settings.stream.hash;
	auto newHash = oldHash;

	// populate work
	for ( auto& node : graph.nodes ) {
		if ( !(0 <= node.mesh && node.mesh < graph.meshes.size()) ) continue;
		if ( !node.entity ) continue;

		bool isStreamable = false;
		float radius = graph.settings.stream.radius;
		float radiusSquared = radius * radius;

		auto& entity = node.entity->as<uf::Object>();
		auto& transform = entity.getComponent<pod::Transform<>>();
		auto model = uf::transform::model( transform );

		auto meshName = graph.meshes[node.mesh];
		auto& mesh = storage.meshes.map[meshName];
		auto& meshStream = graph.streams.meshes[meshName];
		auto& primitives = storage.primitives.map[graph.primitives[node.mesh]];

		if ( node.index == graph.settings.stream.world ) isStreamable = true;
		if ( !isStreamable || meshStream.buffers.empty() ) radius = 0;

		bool needsGraphicBinding = !entity.hasComponent<uf::renderer::Graphic>();
		auto& work = pendingMeshes[node.mesh];
		work.dependentNodes.push_back(node.index);
		if ( work.queuedLODs.empty() && primitives.size() > 0 ) {
			work.queuedLODs.resize( primitives.size(), -1 );
		}

		auto& bvh = storage.bvhs.map[meshName];
		auto& bvhStream = graph.streams.bvhs[meshName];
		if ( bvh.indices.empty() && bvhStream.buffer.length > 0 ) {
			work.needsBvhLoad = true;
			UF_MSG_DEBUG("Queued BVH load={}", meshName);
		}

		if ( radius > 0 && mesh.indirect.count && mesh.indirect.count <= primitives.size() ) {
			float closestDistance = std::numeric_limits<float>::max();
			size_t closestDrawID = 0;
			bool found = false;

			for ( size_t drawID = 0; drawID < primitives.size(); ++drawID ) {
				auto& primitive = primitives[drawID];
				pod::Vector3f center = uf::matrix::multiply( model, primitive.instance.bounds.center, 1.0f );
				float distanceSquared = uf::vector::distanceSquared( center, controllerPosition );

				if ( distanceSquared < closestDistance ) {
					closestDistance = distanceSquared;
					closestDrawID = drawID;
				}

				if ( distanceSquared <= radiusSquared ) {
					found = true;
					int8_t lodLevel = 0;
					float distRatio = distanceSquared / radiusSquared;
					if ( distRatio > 0.6f )	  lodLevel = 3;
					else if ( distRatio > 0.3f ) lodLevel = 2;
					else if ( distRatio > 0.1f ) lodLevel = 1;

					while ( lodLevel > 0 && primitive.lod.levels[lodLevel].indices == 0 ) lodLevel--;

					if ( work.queuedLODs[drawID] < lodLevel ) {
						work.queuedLODs[drawID] = lodLevel;
					}
				}
			}

			if ( !found ) {
				if ( work.queuedLODs[closestDrawID] < 3 ) work.queuedLODs[closestDrawID] = 3;
			}

			auto drawCommandHash = uf::algo::fnv1a(work.queuedLODs);
			graph.settings.stream.lastUpdate = uf::physics::time::current;

			if ( drawCommandHash != oldHash ) {
				work.needsSparseUpdate = true;
				newHash = drawCommandHash;
			} else if ( needsGraphicBinding ) {
				work.needsFallbackBinding = true;
			}

		} else {
			bool buffersEmpty = false;
			for ( auto& attribute : mesh.index.attributes ) {
				if ( mesh.buffers[attribute.buffer].empty() && !meshStream.buffers.empty() ) buffersEmpty = true;
			}
			for ( auto& attribute : mesh.vertex.attributes ) {
				if ( mesh.buffers[attribute.buffer].empty() && !meshStream.buffers.empty() ) buffersEmpty = true;
			}

			if ( buffersEmpty ) work.needsFullLoad = true;
			else if ( needsGraphicBinding ) work.needsFallbackBinding = true;
		}

		// gather textures
		if ( graph.settings.stream.textures ) {
			#define INCREMENT_TEXTURE_REFCOUNT( ID, isSRGB ) if ( 0 <= ID && ID < graph.textures.size() ) {\
				auto& key = graph.textures[ID];\
				textureDescriptors[ID].srgb = isSRGB;\
				textureDescriptors[ID].references += visible ? 1 : 0;\
				textureDescriptors[ID].layers = 1;\
				textureDependentNodes[ID].push_back(node.index);\
			}

			for ( size_t drawID = 0; drawID < primitives.size(); ++drawID ) {
				auto& primitive = primitives[drawID];
				bool visible = primitive.drawCommand.instances > 0;

				INCREMENT_TEXTURE_REFCOUNT(primitive.instance.lightmapID, false);
				if ( 0 <= primitive.instance.materialID && primitive.instance.materialID < graph.materials.size() ) {
					auto& material = storage.materials[graph.materials[primitive.instance.materialID]];
					INCREMENT_TEXTURE_REFCOUNT(material.indexAlbedo, true);
					INCREMENT_TEXTURE_REFCOUNT(material.indexNormal, true);
					INCREMENT_TEXTURE_REFCOUNT(material.indexEmissive, true);
					INCREMENT_TEXTURE_REFCOUNT(material.indexOcclusion, true);
					INCREMENT_TEXTURE_REFCOUNT(material.indexMetallicRoughness, true);
				}
			}
			#undef INCREMENT_TEXTURE_REFCOUNT
		}
	}

	graph.settings.stream.hash = newHash;
	storage.stale = true;

	// dispatch texture loads
	for ( auto& [ imageID, descriptor ] : textureDescriptors ) {
		auto& key = graph.images[imageID];
		auto& image = storage.images[key].data;
		auto& texture = storage.images[key].handle;
		bool visible = descriptor.references > 0;

		if ( visible && (!texture.generated() || texture.aliased) ) {
			if ( !image.getPixels().empty() ) continue;
			auto& imgStream = graph.streams.images[key];
			size_t readLen = imgStream.buffer.length > 0 ? imgStream.buffer.length : uf::io::size(imgStream.buffer.filename);

			if ( readLen <= 0 ) continue;
			texture.layers = descriptor.layers;
			texture.srgb = descriptor.srgb;

			auto filter = uf::renderer::enums::Filter::LINEAR;
			auto& tag = graph.metadata["tags"][key];
			if ( !ext::json::isObject( tag ) ) tag["renderer"] = graphMetadataJson["renderer"];

			if ( tag["renderer"]["filter"].is<uf::stl::string>() ) {
				const auto mode = uf::string::lowercase( tag["renderer"]["filter"].as<uf::stl::string>("linear") );
				if ( mode == "linear" ) filter = uf::renderer::enums::Filter::LINEAR;
				else if ( mode == "nearest" ) filter = uf::renderer::enums::Filter::NEAREST;
			}

			texture.sampler.descriptor.filter.min = filter;
			texture.sampler.descriptor.filter.mag = filter;

			uf::asset::read( imgStream.buffer.filename, imgStream.buffer.offset, readLen,
				[key, graphPtr = &graph, deps = std::move(textureDependentNodes[imageID])](uf::stl::vector<uint8_t>&& buffer) mutable {
					auto& graph = *graphPtr;
					auto& storage = uf::graph::getStorage( *graphPtr );

					auto isLightmap = key.starts_with("lightmap");
					auto& image = storage.images[key].data;
					auto& imgStream = graph.streams.images[key];
					
					uf::stl::string formatHint = uf::io::extension(image.getFilename());
					if ( imgStream.buffer.filename.find(".dtex") != uf::stl::string::npos ) formatHint = "dtex";

					uf::Image decodedImage;
					uf::image::open( decodedImage, buffer, formatHint, false );

					#if UF_USE_OPENGL && !UF_ENV_DREAMCAST
					if ( isLightmap ) ::convertLightmap( decodedImage );
					#endif

					uf::thread::queue( uf::thread::mainThreadName, [key, graphPtr, img = std::move(decodedImage), deps = std::move(deps)]() mutable {
						auto& storage = uf::graph::getStorage( *graphPtr );
						auto& texture = storage.images[key].handle;
						storage.images[key].data = std::move(img);

						if ( texture.aliased ) {
							texture.aliased = false;
						#if UF_USE_OPENGL
							texture.image = 0;
						#else
							texture.image = {}; texture.view = {};
						#endif
						}
						texture.loadFromImage( storage.images[key].data );

						for ( int32_t nodeID : deps ) {
							auto& node = graphPtr->nodes[nodeID];
							if ( node.entity && node.entity->hasComponent<uf::renderer::Graphic>() ) {
								if ( ::bindTextures( *graphPtr, node.entity->getComponent<uf::renderer::Graphic>() ) ) {
									uf::renderer::states::rebuild = true;
									storage.stale = true;
								}
							}
						}

					#if UF_ENV_DREAMCAST
						storage.images[key].data.clear();
					#endif
					});
				}
			);
		} else if ( !visible && (texture.generated() && !texture.aliased) ) {
			image.clear();
			texture.destroy( true );
			texture.aliasTexture( uf::renderer::Texture2D::empty );
		}
	}

	// dispatch mesh streams
	struct StreamUpdateState {
		uint32_t vertexCount = 0;
		uint32_t indexCount = 0;
		struct DrawCmdUpdate { size_t drawID; pod::DrawCommand cmd; };
		uf::stl::vector<DrawCmdUpdate> updatedCommands;
	};

	for ( auto& [ meshID, work ] : pendingMeshes ) {
		auto& key = graph.meshes[meshID];
		auto& mesh = storage.meshes.map[key];
		auto& meshStream = graph.streams.meshes[key];
		auto& bvhStream = graph.streams.bvhs[key];
		auto& primitives = storage.primitives.map[graph.primitives[meshID]];

		if ( work.needsSparseUpdate ) {
			struct ActiveDraw { size_t drawID; int8_t lodLevel; uint32_t fileVertexID; };
			uf::stl::vector<ActiveDraw> activeDraws;
			activeDraws.reserve(work.queuedLODs.size());

			struct Chunk { size_t offset; uf::stl::vector<uint8_t> data; };
			struct SparseContext {
				int32_t meshID;
				uf::stl::vector<int32_t> deps;

				uint32_t vertexCount = 0;
				uint32_t indexCount = 0;
				uf::stl::vector<std::pair<size_t, pod::DrawCommand>> updatedCommands;

				uf::stl::unordered_map<size_t, size_t> bufferSizes;
				uf::stl::unordered_map<size_t, uf::stl::vector<Chunk>> chunks;
				uf::stl::unordered_map<size_t, uf::stl::vector<uint8_t>> completedBuffers;

				uf::stl::vector<uint8_t> bvhRawBuffer;
				uf::stl::atomic<int> pendingReads{0};
			};

			auto ctx = std::make_shared<SparseContext>();
			ctx->meshID = meshID;
			ctx->deps = std::move(work.dependentNodes);

			auto& attribute = mesh.indirect.attributes.front();
			pod::DrawCommand* drawCommands = (pod::DrawCommand*) mesh.buffers[attribute.buffer].data();

			for ( size_t drawID = 0; drawID < work.queuedLODs.size(); ++drawID ) {
				auto lodLevel = work.queuedLODs[drawID];
				if ( lodLevel >= 0 ) {
					activeDraws.emplace_back(ActiveDraw{drawID, lodLevel, primitives[drawID].lod.levels[lodLevel].vertexID});
				} else {
					pod::DrawCommand cmd = {};
					if ( drawID < mesh.indirect.count ) cmd = drawCommands[drawID];
					cmd.vertices = 0; cmd.indices = 0; cmd.vertexID = 0; cmd.indexID = 0;
					ctx->updatedCommands.push_back({drawID, cmd});
				}
			}

			std::sort(activeDraws.begin(), activeDraws.end(), [](const ActiveDraw& a, const ActiveDraw& b) {
				return a.fileVertexID < b.fileVertexID;
			});

			uf::stl::unordered_map<size_t, size_t> uniqueVertexBuffers;
			for ( auto& attr : mesh.vertex.attributes ) {
				uniqueVertexBuffers[attr.buffer] = attr.stride > 0 ? attr.stride : attr.descriptor.size;
			}

			uf::stl::unordered_map<size_t, size_t> uniqueIndexBuffers;
			for ( auto& attr : mesh.index.attributes ) {
				uniqueIndexBuffers[attr.buffer] = attr.stride > 0 ? attr.stride : attr.descriptor.size;
			}

			int totalReads = work.needsBvhLoad ? 1 : 0;
			for ( auto& active : activeDraws ) {
				auto& lod = primitives[active.drawID].lod.levels[active.lodLevel];

				if ( lod.indices > 0 ) {
					totalReads += uniqueIndexBuffers.size();
					for ( auto& [bufID, stride] : uniqueIndexBuffers ) {
						ctx->bufferSizes[bufID] += lod.indices * stride;
					}
				}
				if ( lod.vertices > 0 ) {
					totalReads += uniqueVertexBuffers.size();
					for ( auto& [bufID, stride] : uniqueVertexBuffers ) {
						ctx->bufferSizes[bufID] += lod.vertices * stride;
					}
				}
			}

			for ( auto& [bufID, size] : ctx->bufferSizes ) {
				if ( size > 0 ) {
					ctx->completedBuffers[bufID].resize(size);
				}
			}

			ctx->pendingReads.store(totalReads);

			auto finalizeStream = [ctx, graphPtr = &graph]() {
				auto& graph = *graphPtr;
				auto& storage = uf::graph::getStorage(graph);
				auto& meshKey = graph.meshes[ctx->meshID];
				auto& mesh = storage.meshes.map[meshKey];
				auto& primitives = storage.primitives.map[graph.primitives[ctx->meshID]];
				auto& bvh = storage.bvhs.map[meshKey];
				auto& bvhStream = graph.streams.bvhs[meshKey];

				for ( auto& [b, buf] : ctx->completedBuffers ) {
					std::swap(mesh.buffers[b], buf);
				}

				mesh.vertex.count = ctx->vertexCount;
				mesh.index.count = ctx->indexCount;

				bool rebuildBvh = false;
				if ( !ctx->bvhRawBuffer.empty() ) {
					if ( !uf::bvh::deserialize( bvh, ctx->bvhRawBuffer ) ) {
						bvhStream.buffer.length = 0;
					}
					ctx->bvhRawBuffer.clear();
					ctx->bvhRawBuffer.shrink_to_fit();
				}
				if ( bvhStream.buffer.length == 0 ) {
					rebuildBvh = true;
				}
				bool bvhValid = !bvh.indices.empty();

				auto& indirectAttr = mesh.indirect.attributes.front();
				pod::DrawCommand* drawCommands = (pod::DrawCommand*) mesh.buffers[indirectAttr.buffer].data();

				for ( auto& [drawID, cmd] : ctx->updatedCommands ) {
					primitives[drawID].drawCommand = cmd;
					drawCommands[drawID] = cmd;

					if ( bvhValid && !rebuildBvh ) {
						uf::bvh::flagAsActive( bvh, drawID, (cmd.vertices > 0 && cmd.indices > 0) );
					}
				}

				mesh.updateDescriptor();
				if ( rebuildBvh ) {
					uf::bvh::build( bvh, mesh );
				}

				for ( auto nodeID : ctx->deps ) {
					uf::graph::reload( graph, graph.nodes[nodeID] );
				}

			#if UF_USE_OPENGL
				uf::renderer::states::rebuild = true;
			#endif
			};
			
			auto cb = [ctx, finalizeStream]() mutable {
				if ( ctx->pendingReads.fetch_sub(1) == 1 ) {
					uf::thread::queue( uf::thread::mainThreadName, finalizeStream );
				}
			};

			if ( totalReads > 0 ) {
				uf::stl::unordered_map<size_t, size_t> bufferWriteOffsets;

				if ( work.needsBvhLoad ) {
					ctx->bvhRawBuffer.resize( bvhStream.buffer.length );
					uf::asset::read( bvhStream.buffer.filename, bvhStream.buffer.offset, bvhStream.buffer.length, ctx->bvhRawBuffer.data(), cb );
				}

				for ( auto& active : activeDraws ) {
					auto& lod = primitives[active.drawID].lod.levels[active.lodLevel];

					pod::DrawCommand cmd = {};
					if ( active.drawID < mesh.indirect.count ) cmd = drawCommands[active.drawID];
					cmd.vertices = lod.vertices; cmd.indices = lod.indices;
					cmd.vertexID = ctx->vertexCount; cmd.indexID = ctx->indexCount;
					ctx->updatedCommands.push_back({active.drawID, cmd});

					auto dispatchInterleavedRead = [&](const auto& uniqueBuffers, uint32_t count, uint32_t id) {
						if ( count == 0 ) return;

						for ( auto& [bufID, stride] : uniqueBuffers ) {
							size_t readBytes = count * stride;
							size_t writeOffset = bufferWriteOffsets[bufID];

							uint8_t* destPtr = ctx->completedBuffers[bufID].data() + writeOffset;
							auto& region = meshStream.buffers[bufID];

							uf::asset::read( region.filename, region.offset + (id * stride), readBytes, destPtr, cb );
							bufferWriteOffsets[bufID] += readBytes;
						}
					};

					dispatchInterleavedRead(uniqueIndexBuffers, lod.indices, lod.indexID);
					dispatchInterleavedRead(uniqueVertexBuffers, lod.vertices, lod.vertexID);

					ctx->vertexCount += lod.vertices;
					ctx->indexCount  += lod.indices;
				}

			} else {
				uf::thread::queue( uf::thread::mainThreadName, finalizeStream );
			}
		} else if ( work.needsFullLoad || work.needsBvhLoad ) {
			struct FullLoadContext {
				int32_t meshID;
				uf::stl::vector<int32_t> deps;
				uf::stl::unordered_map<size_t, uf::stl::vector<uint8_t>> newBuffers;
				uf::stl::vector<uint8_t> bvhRawBuffer;
				uf::stl::atomic<int> pendingReads{0};
			};

			auto ctx = std::make_shared<FullLoadContext>();
			ctx->meshID = meshID;
			ctx->deps = std::move(work.dependentNodes);

			uf::stl::vector<size_t> buffersToLoad;
			if ( work.needsFullLoad ) {
				uf::stl::unordered_set<size_t> processedBuffers;
				auto collectAttributes = [&](const auto& attributes) {
					for ( auto& attr : attributes ) {
						if ( processedBuffers.count(attr.buffer) ) continue;
						if ( mesh.buffers[attr.buffer].empty() && !meshStream.buffers.empty() ) {
							buffersToLoad.push_back(attr.buffer);
							processedBuffers.insert(attr.buffer);
						}
					}
				};
				collectAttributes(mesh.index.attributes);
				collectAttributes(mesh.vertex.attributes);
			}

			int totalReads = buffersToLoad.size() + (work.needsBvhLoad ? 1 : 0);
			ctx->pendingReads.store(totalReads);

			auto finalizeFullLoad = [ctx, graphPtr = &graph]() {
				auto& graph = *graphPtr;
				auto& storage = uf::graph::getStorage(graph);
				auto& meshKey = graph.meshes[ctx->meshID];
				auto& mesh = storage.meshes.map[meshKey];
				auto& bvh = storage.bvhs.map[meshKey];

				for ( auto& [b, buf] : ctx->newBuffers ) {
					std::swap(mesh.buffers[b], buf);
				}
				mesh.updateDescriptor();

				if ( !ctx->bvhRawBuffer.empty() ) {
					uf::bvh::deserialize( bvh, ctx->bvhRawBuffer );
					ctx->bvhRawBuffer.clear();
					ctx->bvhRawBuffer.shrink_to_fit();
				}

				for ( auto nodeID : ctx->deps ) {
					uf::graph::reload( graph, graph.nodes[nodeID] );
				}
			};
			
			auto cb = [ctx, finalizeFullLoad]() mutable {
				if ( ctx->pendingReads.fetch_sub(1) == 1 ) {
					uf::thread::queue( uf::thread::mainThreadName, finalizeFullLoad );
				}
			};

			if ( totalReads > 0 ) {
				for ( size_t bufID : buffersToLoad ) {
					ctx->newBuffers[bufID].resize(meshStream.buffers[bufID].length);
				}
				if ( work.needsBvhLoad ) {
					ctx->bvhRawBuffer.resize(graph.streams.bvhs[key].buffer.length);
				}


				for ( size_t bufID : buffersToLoad ) {
					auto& region = meshStream.buffers[bufID];
					uf::asset::read( region.filename, region.offset, region.length, ctx->newBuffers[bufID].data(), cb );
				}

				if ( work.needsBvhLoad ) {
					uf::asset::read( bvhStream.buffer.filename, bvhStream.buffer.offset, bvhStream.buffer.length, ctx->bvhRawBuffer.data(), cb );
				}

			} else {
				uf::thread::queue( uf::thread::mainThreadName, finalizeFullLoad );
			}
		} else if ( work.needsFallbackBinding ) {
			uf::thread::queue( uf::thread::mainThreadName, [graphPtr = &graph, deps = std::move(work.dependentNodes)]() {
				for ( auto nodeID : deps ) uf::graph::reload( *graphPtr, graphPtr->nodes[nodeID] );
			});
		}
	}

//	uf::asset::processIO();
//	uf::thread::process( uf::thread::get(uf::thread::mainThreadName) );
}

void uf::graph::reload() {
	switch ( uf::graph::storageMode ) {
		case pod::Graph::Storage::SCENE: {
			auto& storage = uf::scene::getCurrentScene().getComponent<pod::Graph::Storage>();
			storage.stale = true;
		}
		case pod::Graph::Storage::GLOBAL:
		default: {
			uf::graph::globalStorage.stale = true;
		}
	}
}

void uf::graph::update( pod::Graph& graph ) {
	return update( graph, uf::physics::time::delta );
}
void uf::graph::update( pod::Graph& graph, float delta ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& storage = uf::graph::getStorage( graph );

	// rebuild
	if ( storage.shouldRebind ) {
		for ( auto& node : graph.nodes ) {
			if ( !(0 <= node.mesh && node.mesh < graph.meshes.size()) ) continue;
			if ( !node.entity ) continue;

			auto& entity = node.entity->as<uf::Object>();

			if ( !entity.hasComponent<uf::renderer::Graphic>() ) continue;

			auto& graphic = entity.getComponent<uf::renderer::Graphic>();
			auto& mesh = storage.meshes.map[graph.meshes[node.mesh]];
			auto& primitives = storage.primitives.map[graph.primitives[node.mesh]];

			::bindBuffers( graph, graphic, mesh );
			::bindAddresses( graph, graphic, mesh, primitives );
		}
		//storage.shouldRebind = false;
	}

	// get last update time
#if UF_GRAPH_EXTENDED
	if ( graph.settings.stream.enabled && graph.settings.stream.hash != 0 && uf::physics::time::current - graph.settings.stream.lastUpdate > graph.settings.stream.every ) {
		graph.settings.stream.lastUpdate = uf::physics::time::current;
		uf::graph::reload( graph );
	}
#endif

	uf::graph::updateAnimation( graph, delta );
}

uf::stl::string uf::graph::getMaterialName( pod::Graph& graph, size_t materialID ) {
	auto& storage = uf::graph::getStorage( graph );
	if ( !(0 <= materialID && materialID < storage.materials.keys.size() ) ) return "";
	return storage.materials.keys[materialID];
}
pod::Material uf::graph::getMaterial( pod::Graph& graph, size_t materialID ) {
	auto& storage = uf::graph::getStorage( graph );
	auto key = uf::graph::getMaterialName( graph, materialID );
	return storage.materials.map[key];
}

pod::Primitive uf::graph::getPrimitive( pod::Graph& graph, size_t primitiveID ) {
	auto& storage = uf::graph::getStorage( graph );
	if ( !(0 <= primitiveID && primitiveID < storage.flattenedPrimitives.size() ) ) return {};
	return storage.flattenedPrimitives[primitiveID];
}
pod::Instance uf::graph::getInstance( pod::Graph& graph, size_t instanceID ) {
	auto primitive = uf::graph::getPrimitive( graph, instanceID );
	return primitive.instance;
}