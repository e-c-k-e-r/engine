#include <uf/engine/graph/graph.h>
#include <uf/ext/gltf/gltf.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/mesh/grid.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/string/base64.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/memory/map.h>
#include <uf/ext/xatlas/xatlas.h>
#include <uf/ext/ffx/fsr.h>

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

#if UF_USE_OPENGL
	#define UF_GRAPH_SPARSE_READ_MESH 1
#else
	#define UF_GRAPH_SPARSE_READ_MESH 1
#endif
#define UF_GRAPH_EXTENDED 1

namespace {
	bool newGraphAdded = true;
	bool shouldRebind = false;

	// todo: shove it into the "std"lib
	inline uint64_t fnv1aHash(const uf::stl::vector<bool>& bits) {
		uint64_t hash = 1469598103934665603ULL;
		for (bool b : bits) {
			hash ^= static_cast<uint64_t>(b);
			hash *= 1099511628211ULL;
		}
		return hash;
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

	pod::Graph::Storage& getGraphStorage( uf::Object& object ) {
		return uf::graph::globalStorage ? uf::graph::storage : object.getComponent<pod::Graph::Storage>();
	}

	// removes non-uniform aliased buffers
	void resetBuffers( uf::renderer::Shader& shader ) {
		shader.metadata.aliases.buffers.clear();
	}

	void bindTextures( uf::renderer::Graphic& graphic ) {
		graphic.material.textures.clear();

		auto& scene = uf::scene::getCurrentScene();
		auto& storage = ::getGraphStorage( scene );

		for ( auto& key : storage.texture2Ds.keys ) graphic.material.textures.emplace_back().aliasTexture( storage.texture2Ds.map[key] );

		// bind scene's voxel texture
	#if UF_USE_VULKAN
		if ( uf::renderer::settings::pipelines::vxgi ) {
			auto& scene = uf::scene::getCurrentScene();
			auto& sceneTextures = scene.getComponent<pod::SceneTextures>();
			for ( auto& t : sceneTextures.voxels.drawId ) graphic.material.textures.emplace_back().aliasTexture(t);
			for ( auto& t : sceneTextures.voxels.instanceId ) graphic.material.textures.emplace_back().aliasTexture(t);
			for ( auto& t : sceneTextures.voxels.normalX ) graphic.material.textures.emplace_back().aliasTexture(t);
			for ( auto& t : sceneTextures.voxels.normalY ) graphic.material.textures.emplace_back().aliasTexture(t);
			for ( auto& t : sceneTextures.voxels.radianceR ) graphic.material.textures.emplace_back().aliasTexture(t);
			for ( auto& t : sceneTextures.voxels.radianceG ) graphic.material.textures.emplace_back().aliasTexture(t);
			for ( auto& t : sceneTextures.voxels.radianceB ) graphic.material.textures.emplace_back().aliasTexture(t);
			for ( auto& t : sceneTextures.voxels.radianceA ) graphic.material.textures.emplace_back().aliasTexture(t);
			for ( auto& t : sceneTextures.voxels.count ) graphic.material.textures.emplace_back().aliasTexture(t);
			for ( auto& t : sceneTextures.voxels.output ) graphic.material.textures.emplace_back().aliasTexture(t);
		}
	#endif
	}

	void bindShaders( pod::Graph& graph, uf::Object& entity, uf::Mesh& mesh ) {
		auto& scene = uf::scene::getCurrentScene();
		auto& sceneTextures = scene.getComponent<pod::SceneTextures>();
		auto& sceneMetadataJson = scene.getComponent<uf::Serializer>();
		auto& storage = ::getGraphStorage( scene );

		auto& graphic = entity.getComponent<uf::renderer::Graphic>();
		auto& graphMetadataJson = graph.metadata;

		uf::stl::string root = uf::io::directory( graph.name );
		size_t texture2Ds = 0;
		size_t texture3Ds = 0;
		for ( auto& texture : graphic.material.textures ) {
			if ( texture.width > 1 && texture.height > 1 && texture.depth == 1 && texture.layers == 1 ) ++texture2Ds;
			else if ( texture.width > 1 && texture.height > 1 && texture.depth > 1 && texture.layers == 1 ) ++texture3Ds;
		}

		// standard pipeline
		{
			uf::stl::string vertexShaderFilename = graphMetadataJson["shaders"]["vertex"].as<uf::stl::string>("/graph/base/vert.spv"); {
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
			uf::stl::string fragmentShaderFilename = graphMetadataJson["shaders"]["fragment"].as<uf::stl::string>("/graph/base/frag.spv"); {
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
				uint32_t maxTextures = storage.textures.map.size();
				shader.setSpecializationConstants({
					{ "TEXTURES", maxTextures },
				});
				shader.setDescriptorCounts({
					{ "samplerTextures", maxTextures },
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

			uint32_t voxelTypes = 0;
			if ( !sceneTextures.voxels.drawId.empty() ) ++voxelTypes;
			if ( !sceneTextures.voxels.instanceId.empty() ) ++voxelTypes;
			if ( !sceneTextures.voxels.normalX.empty() ) ++voxelTypes;
			if ( !sceneTextures.voxels.normalY.empty() ) ++voxelTypes;
			if ( !sceneTextures.voxels.radianceR.empty() ) ++voxelTypes;
			if ( !sceneTextures.voxels.radianceG.empty() ) ++voxelTypes;
			if ( !sceneTextures.voxels.radianceB.empty() ) ++voxelTypes;
			if ( !sceneTextures.voxels.radianceA.empty() ) ++voxelTypes;
			if ( !sceneTextures.voxels.count.empty() ) ++voxelTypes;
			if ( !sceneTextures.voxels.output.empty() ) ++voxelTypes;

			uint32_t maxTextures = texture2Ds;
			uint32_t maxCascades = texture3Ds / voxelTypes;
			
			// fragment shader
			{
				auto& shader = graphic.material.getShader("fragment", uf::renderer::settings::pipelines::names::vxgi);
				shader.setSpecializationConstants({
					{ "TEXTURES", maxTextures },
					{ "CASCADES", maxCascades },
				});
				shader.setDescriptorCounts({
					{ "samplerTextures", maxTextures },
					{ "voxelDrawId", maxCascades },
					{ "voxelInstanceId", maxCascades },
					{ "voxelNormalX", maxCascades },
					{ "voxelNormalY", maxCascades },
					{ "voxelRadianceR", maxCascades },
					{ "voxelRadianceG", maxCascades },
					{ "voxelRadianceB", maxCascades },
					{ "voxelRadianceA", maxCascades },
					{ "voxelCount", maxCascades },
					{ "voxelOutput", maxCascades },
				});
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
				struct PushConstant {
					uint32_t jointID;
				};

				if ( mesh.isInterleaved( mesh.vertex ) ) {
					uf::stl::string compShaderFilename = graphMetadataJson["shaders"]["skinning"]["compute"].as<uf::stl::string>("/graph/skinning/skinning.interleaved.comp.spv"); {
						compShaderFilename = entity.resolveURI( compShaderFilename, root );
					}
					graphic.material.metadata.autoInitializeUniformBuffers = false;
					graphic.material.attachShader(compShaderFilename, uf::renderer::enums::Shader::COMPUTE, "skinning");
					graphic.material.metadata.autoInitializeUniformBuffers = true;
					
					graphic.descriptor.bind.width = mesh.vertex.count;
					graphic.descriptor.bind.height = 1;
					graphic.descriptor.bind.depth = 1;

					// compute shader
					auto& shader = graphic.material.getShader("compute", "skinning");

					// bind buffers
					struct {
						uint32_t jointID;
					} uniforms = {
						.jointID = 0
					};

					auto& vertexSourceData = mesh.buffers[mesh.vertex.interleaved];
					size_t vertexSourceDataIndex = graphic.initializeBuffer( (const void*) vertexSourceData.data(), vertexSourceData.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR );

					auto& vertexIn = graphic.buffers.at(graphic.descriptor.inputs.vertex.interleaved);
					auto& vertexOut = graphic.buffers.at(vertexSourceDataIndex);
					graphic.metadata.buffers["vertexSkinned"] = vertexSourceDataIndex;

					shader.updateBuffer( (const void*) &uniforms, sizeof(uniforms), shader.getUniformBuffer("UBO") );

					::resetBuffers( shader );
					shader.aliasBuffer( storage.buffers.joint );
					shader.aliasBuffer( vertexIn );
					shader.aliasBuffer( vertexOut );
				} else {
					uf::stl::string compShaderFilename = graphMetadataJson["shaders"]["skinning"]["compute"].as<uf::stl::string>("/graph/skinning/skinning.deinterleaved.comp.spv"); {
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

					struct {
						uint32_t jointID;
					} uniforms = {
						.jointID = 0
					};

					shader.updateBuffer( (const void*) &uniforms, sizeof(uniforms), shader.getUniformBuffer("UBO") );

					// bind buffers
					::resetBuffers( shader );
					shader.aliasBuffer( storage.buffers.joint );
					shader.aliasBuffer( vertexPositionBuffer );
					shader.aliasBuffer( vertexJointsBuffer );
					shader.aliasBuffer( vertexWeightsBuffer );
					shader.aliasBuffer( vertexOutPosition );
				}
			}

			graphic.generateBottomAccelerationStructures();
		}
		#endif
	}

	void bindBuffers( uf::renderer::Graphic& graphic, uf::Mesh& mesh ) {
		auto& scene = uf::scene::getCurrentScene();
		auto& sceneTextures = scene.getComponent<pod::SceneTextures>();
		auto& sceneMetadataJson = scene.getComponent<uf::Serializer>();
		auto& storage = ::getGraphStorage( scene );

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
				shader.aliasBuffer( "instanceAddresses", storage.buffers.instanceAddresses );
				shader.aliasBuffer( "material", storage.buffers.material );
				shader.aliasBuffer( "texture", storage.buffers.texture );
				shader.aliasBuffer( "light", storage.buffers.light );
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
			shader.aliasBuffer( "instanceAddresses", storage.buffers.instanceAddresses );
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
				shader.aliasBuffer( "instanceAddresses", storage.buffers.instanceAddresses );
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
			#if UF_USE_VULKAN
				shader.aliasBuffer( "indirect", *indirect );
			#endif
				shader.aliasBuffer( "instance", storage.buffers.instance );
				shader.aliasBuffer( "object", storage.buffers.object );
				shader.aliasBuffer( "joint", storage.buffers.joint );
			}

			// fragment shader
			{
				auto& shader = graphic.material.getShader("fragment", "baking");

				// bind buffers
				::resetBuffers( shader );
				shader.aliasBuffer( "camera", storage.buffers.camera );
				shader.aliasBuffer( "drawCommands", storage.buffers.drawCommands );
				shader.aliasBuffer( "instance", storage.buffers.instance );
				shader.aliasBuffer( "instanceAddresses", storage.buffers.instanceAddresses );
				shader.aliasBuffer( "material", storage.buffers.material );
				shader.aliasBuffer( "texture", storage.buffers.texture );
				shader.aliasBuffer( "light", storage.buffers.light );
			}
		}
		#endif
	}

	void bindInstanceAddresses( uf::renderer::Graphic& graphic, uf::Mesh& mesh, uf::stl::vector<pod::Instance::Addresses>& addresses ) {
		if ( !uf::renderer::settings::invariant::deviceAddressing || !mesh.indirect.count ) return;
		addresses.resize( mesh.indirect.count );

		pod::DrawCommand* drawCommands = (pod::DrawCommand*) mesh.getBuffer( mesh.indirect ).data();
		for ( size_t drawID = 0; drawID < mesh.indirect.count; ++drawID ) {
			auto& drawCommand = drawCommands[drawID];
			auto instanceID = drawCommand.instanceID;

			auto& instanceAddresses = addresses[drawID]; // THIS IS WRONG (to-do: actually use instanceIDs)
			if ( mesh.vertex.count ) {
				if ( mesh.isInterleaved( mesh.vertex ) ) {
					instanceAddresses.vertex = graphic.buffers.at(graphic.descriptor.inputs.vertex.interleaved).getAddress();
				} else {
					for ( auto& attribute : graphic.descriptor.inputs.vertex.attributes ) {
						if ( attribute.buffer < 0 ) continue;
						if ( attribute.descriptor.name == "position" ) 		instanceAddresses.position = graphic.buffers.at(attribute.buffer).getAddress();
						else if ( attribute.descriptor.name == "uv" ) 		instanceAddresses.uv = graphic.buffers.at(attribute.buffer).getAddress();
						else if ( attribute.descriptor.name == "color" ) 	instanceAddresses.color = graphic.buffers.at(attribute.buffer).getAddress();
						else if ( attribute.descriptor.name == "st" ) 		instanceAddresses.st = graphic.buffers.at(attribute.buffer).getAddress();
						else if ( attribute.descriptor.name == "normal" ) 	instanceAddresses.normal = graphic.buffers.at(attribute.buffer).getAddress();
						else if ( attribute.descriptor.name == "tangent" ) 	instanceAddresses.tangent = graphic.buffers.at(attribute.buffer).getAddress();
						else if ( attribute.descriptor.name == "joints" ) 	instanceAddresses.joints = graphic.buffers.at(attribute.buffer).getAddress();
						else if ( attribute.descriptor.name == "weights" ) 	instanceAddresses.weights = graphic.buffers.at(attribute.buffer).getAddress();
						else if ( attribute.descriptor.name == "id" ) 		instanceAddresses.id = graphic.buffers.at(attribute.buffer).getAddress();
					}
				}
			}
			if ( mesh.index.count ) {
				if ( mesh.isInterleaved( mesh.index ) ) instanceAddresses.index = graphic.buffers.at(graphic.descriptor.inputs.index.interleaved).getAddress();
				else instanceAddresses.index = graphic.buffers.at(graphic.descriptor.inputs.index.attributes.front().buffer).getAddress();
			}

			if ( mesh.indirect.count ) {
				if ( mesh.isInterleaved( mesh.indirect ) ) instanceAddresses.indirect = graphic.buffers.at(graphic.descriptor.inputs.indirect.interleaved).getAddress();
				else instanceAddresses.indirect = graphic.buffers.at(graphic.descriptor.inputs.indirect.attributes.front().buffer).getAddress();

				instanceAddresses.drawID = drawID;
			}
		}
	}
}

size_t uf::graph::initialBufferElements = 1024;

bool uf::graph::globalStorage = false;
pod::Graph::Storage uf::graph::storage;

UF_VERTEX_DESCRIPTOR(uf::graph::mesh::Base,
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base, R32G32_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base, R8G8B8A8_UNORM, color)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base, R32G32_SFLOAT, st)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base, R32G32B32_SFLOAT, normal)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base, R32G32B32_SFLOAT, tangent)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base, R16G16_UINT, id)
);
// it'd be super sugoi if I could somehow macro this annoyance
UF_VERTEX_INTERPOLATE(uf::graph::mesh::Base, {
	return {
		uf::vector::lerp( p1.position, p2.position, t ),
		uf::vector::lerp( p1.uv, p2.uv, t ),
		t < 0.5 ? p1.color : p2.color,
		uf::vector::lerp( p1.st, p2.st, t ),
		uf::vector::normalize( uf::vector::lerp( p1.normal, p2.normal, t ) ),
		uf::vector::normalize( uf::vector::lerp( p1.tangent, p2.tangent, t ) ),
		t < 0.5 ? p1.id : p2.id,
	};
})

UF_VERTEX_DESCRIPTOR(uf::graph::mesh::Skinned,
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R32G32_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R8G8B8A8_UNORM, color)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R32G32_SFLOAT, st)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R32G32B32_SFLOAT, normal)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R32G32B32_SFLOAT, tangent)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R16G16_UINT, id)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R16G16B16A16_UINT, joints)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R32G32B32A32_SFLOAT, weights)
);
UF_VERTEX_INTERPOLATE(uf::graph::mesh::Skinned, {
	return {
		uf::vector::lerp( p1.position, p2.position, t ),
		uf::vector::lerp( p1.uv, p2.uv, t ),
		t < 0.5 ? p1.color : p2.color,
		uf::vector::lerp( p1.st, p2.st, t ),
		uf::vector::normalize( uf::vector::lerp( p1.normal, p2.normal, t ) ),
		uf::vector::normalize( uf::vector::lerp( p1.tangent, p2.tangent, t ) ),
		t < 0.5 ? p1.id : p2.id,
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
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base_16f, R16G16B16_SFLOAT, tangent)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base_16f, R16G16_UINT, id)
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
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_16f, R16G16B16_SFLOAT, tangent)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_16f, R16G16_UINT, id)
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
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base_u16q, R16G16B16_UINT, tangent)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base_u16q, R16G16_UINT, id)
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
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_u16q, R16G16B16_UINT, tangent)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_u16q, R16G16_UINT, id)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_u16q, R16G16B16A16_UINT, joints)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_u16q, R16G16B16A16_UINT, weights)
);

UF_VERTEX_INTERPOLATE(uf::graph::mesh::Skinned_u16q, {
	return t < 0.5 ? p1 : p2;
})

void uf::graph::initializeGraphics( pod::Graph& graph, uf::Object& entity, uf::Mesh& mesh, uf::stl::vector<pod::Instance::Addresses>& addresses ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& sceneTextures = scene.getComponent<pod::SceneTextures>();
	auto& sceneMetadataJson = scene.getComponent<uf::Serializer>();
	auto& storage = ::getGraphStorage( scene );
	
	auto& graphMetadataJson = graph.metadata;

	auto& graphic = entity.getComponent<uf::renderer::Graphic>();
	graphic.initialize();
	graphic.initializeMesh( mesh );

	graphic.device = &uf::renderer::device;
	graphic.material.device = &uf::renderer::device;
	graphic.descriptor.frontFace = graphMetadataJson["renderer"]["invert"].as<bool>(true) ? uf::renderer::enums::Face::CW : uf::renderer::enums::Face::CCW;
	graphic.descriptor.cullMode = uf::renderer::enums::CullMode::BACK;

	auto tag = ext::json::find( entity.getName(), graphMetadataJson["tags"] );
	if ( !ext::json::isObject( tag ) ) {
		tag["renderer"] = graphMetadataJson["renderer"];
	}

	if ( tag["renderer"]["front face"].is<uf::stl::string>() ) {
		const auto mode = uf::string::lowercase( tag["renderer"]["front face"].as<uf::stl::string>() );
		if ( mode == "cw" ) graphic.descriptor.frontFace = uf::renderer::enums::Face::CW;
		else if ( mode == "ccw" ) graphic.descriptor.frontFace = uf::renderer::enums::Face::CCW;
		else if ( mode == "auto" ) {
			if ( uf::matrix::reverseInfiniteProjection ) {
				graphic.descriptor.frontFace = graphMetadataJson["renderer"]["invert"].as<bool>(true) ? uf::renderer::enums::Face::CW : uf::renderer::enums::Face::CCW;
			} else {
				graphic.descriptor.frontFace = graphMetadataJson["renderer"]["invert"].as<bool>(true) ? uf::renderer::enums::Face::CW : uf::renderer::enums::Face::CCW;
			}
		}
		else UF_MSG_WARNING("Invalid Face enum string specified: {}", mode);
	}
	if ( tag["renderer"]["cull mode"].is<uf::stl::string>() ) {
		const auto mode = uf::string::lowercase( tag["renderer"]["cull mode"].as<uf::stl::string>() );
		if ( mode == "back" ) graphic.descriptor.cullMode = uf::renderer::enums::CullMode::BACK;
		else if ( mode == "front" ) graphic.descriptor.cullMode = uf::renderer::enums::CullMode::FRONT;
		else if ( mode == "none" ) graphic.descriptor.cullMode = uf::renderer::enums::CullMode::NONE;
		else if ( mode == "both" ) graphic.descriptor.cullMode = uf::renderer::enums::CullMode::BOTH;
		else UF_MSG_WARNING("Invalid CullMode enum string specified: {}", mode);
	}

	::bindTextures( graphic );
	::bindShaders( graph, entity, mesh );
	::bindBuffers( graphic, mesh );
	::bindInstanceAddresses( graphic, mesh, addresses );

	graphic.process = true;
}

void uf::graph::process( pod::Graph& graph ) {
	UF_DEBUG_TIMER_MULTITRACE_START("Processing {}", graph.name);

	//
	if ( !graph.root.entity ) graph.root.entity = new uf::Object;
	
	// copy lighting settings from graph
	auto& scene = uf::scene::getCurrentScene();
	auto& sceneMetadataJson = scene.getComponent<uf::Serializer>();
	auto& graphMetadataJson = graph.metadata;
	auto& storage = ::getGraphStorage( scene );

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

	//
	uf::stl::unordered_map<uf::stl::string, bool> isSrgb;

	// process lightmap
	UF_DEBUG_TIMER_MULTITRACE("Parsing lightmaps");
	if ( true ) {
		constexpr const char* UF_GRAPH_DEFAULT_LIGHTMAP = "./lightmap.%i.png";
		uf::stl::unordered_map<size_t, uf::stl::string> filenames;
		uf::stl::unordered_map<size_t, size_t> lightmapIDs;
		uint32_t lightmapCount = 0;

		for ( auto& name : graph.primitives ) {
			auto& primitives = storage.primitives[name];
			for ( auto& primitive : primitives ) {
				filenames[primitive.instance.auxID] = uf::string::replace(UF_GRAPH_DEFAULT_LIGHTMAP, "%i", std::to_string(primitive.instance.auxID));

				lightmapCount = std::max( lightmapCount, primitive.instance.auxID + 1 );
			}
		}

		if ( graphMetadataJson["lights"]["lightmap"].is<bool>() && !graphMetadataJson["lights"]["lightmap"].as<bool>() ) {
			graphMetadataJson["baking"]["enabled"] = false;
		}
		if ( !sceneMetadataJson["light"]["useLightmaps"].as<bool>(true) ) {
			graphMetadataJson["lights"]["lightmap"] = false;
			graphMetadataJson["baking"]["enabled"] = false;
		}

		if ( graphMetadataJson["lights"]["lightmap"].is<uf::stl::string>() && graphMetadataJson["lights"]["lightmap"].as<uf::stl::string>() == "auto" ) {
			uint32_t mtime = uf::io::mtime( graph.name );
			// lightmaps are considered stale if they're older than the graph's source
			bool stale = false;
			for ( auto& pair : filenames ) {
				uf::stl::string filename = uf::io::sanitize( pair.second, uf::io::directory( graph.name ) );
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
				auto f = uf::io::sanitize( pair.second, uf::io::directory( graph.name ) );
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
				auto f = uf::io::sanitize( pair.second, uf::io::directory( graph.name ) );

				auto textureID = graph.textures.size();
				auto imageID = graph.images.size();

				auto& texture = storage.textures[graph.textures.emplace_back(f)];
				auto& image = storage.images[graph.images.emplace_back(f)];
				if ( !graph.settings.stream.textures ) {
					image.open( f, false );
				}

				texture.index = imageID;

				lightmapIDs[i] = textureID;

				graphMetadataJson["lights"]["lightmaps"][i] = f;
				graphMetadataJson["baking"]["enabled"] = false;

				isSrgb[f] = false;
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

	// setup textures
	storage.texture2Ds.reserve( storage.images.map.size() );

	// figure out what texture is what exactly
	UF_DEBUG_TIMER_MULTITRACE("Determining format of textures");
	for ( auto& key : graph.materials ) {
		auto& material = storage.materials[key];
		auto ID = material.indexAlbedo;

		if ( !(0 <= ID && ID < graph.textures.size()) ) continue;

		auto texName = graph.textures[ID];
		isSrgb[texName] = true;
	}

	UF_DEBUG_TIMER_MULTITRACE("Processing images...");
	for ( auto& key : graph.images ) {
		auto& image = storage.images[key];
		auto& texture = storage.texture2Ds[key];
		if ( !texture.generated() ) {
			// set as null
			if ( graph.settings.stream.textures ) {
				texture.aliasTexture(uf::renderer::Texture2D::empty);
				continue;
			}

			auto filter = uf::renderer::enums::Filter::LINEAR;
			auto tag = ext::json::find( key, graphMetadataJson["tags"] );
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
			texture.srgb = isSrgb[key];

			// to-do: figure out why I need to skip rendering the next frame to avoid a crash here if the storage buffers need to be resized on the GPU side
			// i suppose timing is consistent enough to where this is loaded asynchronously and rendering throws a device lost error
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
		if ( node.entity ) UF_DEBUG_TIMER_MULTITRACE("Processed node: {}", node.name);
	}

	// patch materials/textures
	UF_DEBUG_TIMER_MULTITRACE("Patching textures/materials");
	for ( auto& name : graph.materials ) {
		auto& material = storage.materials[name];
		auto tag = ext::json::find( name, graphMetadataJson["tags"] );
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
				if ( mode == "opaque" ) material.modeAlpha = 0;
				else if ( mode == "blend" ) material.modeAlpha = 1;
				else if ( mode == "mask" ) material.modeAlpha = 2;
				else UF_MSG_WARNING("Invalid AlphaMode enum string specified: {}", mode);
			} else {
				material.modeAlpha = tag["material"]["modeAlpha"].as(material.modeAlpha);
			}
		}
	}

	// remap textures->images IDs
	UF_DEBUG_TIMER_MULTITRACE("Remapping texture -> image IDs");
	for ( auto& name : graph.textures ) {
		auto& texture = storage.textures[name];
		auto& keys = storage.images.keys;
		auto& indices = storage.images.indices;

		if ( !(0 <= texture.index && texture.index < graph.images.size()) ) continue;

		auto& needle = graph.images[texture.index];
		texture.index = indices[needle];
	}

	// remap materials->texture IDs
	UF_DEBUG_TIMER_MULTITRACE("Remapping material -> texture IDs");
	for ( auto& name : graph.materials ) {
		auto& material = storage.materials[name];
		auto& keys = storage.textures.keys;
		auto& indices = storage.textures.indices;
		int32_t* IDs[] = { &material.indexAlbedo, &material.indexNormal, &material.indexEmissive, &material.indexOcclusion, &material.indexMetallicRoughness };
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

			// remap a skinID as an actual jointID
			if ( 0 <= instance.jointID && instance.jointID < graph.skins.size() ) {
				auto& name = graph.skins[instance.jointID];
				instance.jointID = 0;
				for ( auto key : storage.joints.keys ) {
					if ( key == name ) break;
					auto& joints = storage.joints[key];
					instance.jointID += joints.size();
				}
			}
		}
	}
/*
	if ( graphMetadataJson["debug"]["print"]["lights"].as<bool>() ) {
		UF_MSG_DEBUG("Lights: {}", graph.lights.size());
		for ( auto& pair : graph.lights ) {
			UF_MSG_DEBUG("\tLight: {}", pair.first);
		}
	}
	if ( graphMetadataJson["debug"]["print"]["meshes"].as<bool>() ) {
		UF_MSG_DEBUG("Meshs: {}", graph.meshes.size());
		for ( auto& name : graph.meshes ) {
			UF_MSG_DEBUG("\tMesh: {}", name);
		}
	}
	if ( graphMetadataJson["debug"]["print"]["materials"].as<bool>() ) {
		UF_MSG_DEBUG("Materials: {}", graph.materials.size());
		for ( auto& name : graph.materials ) {
			auto& material = storage.materials[name];
			UF_MSG_DEBUG("\tMaterial: {} | {}", name, material.indexAlbedo);
		}
	}
	if ( graphMetadataJson["debug"]["print"]["textures"].as<bool>() ) {
		UF_MSG_DEBUG("Textures: {}", graph.textures.size());
		for ( auto& name : graph.textures ) {
			auto& texture = storage.textures[name];
			UF_MSG_DEBUG("\tTexture: {} | {}", name, texture.index);
		}
	}
	if ( graphMetadataJson["debug"]["print"]["images"].as<bool>() ) {
		UF_MSG_DEBUG("Images: {}", graph.images.size());
		for ( auto& name : graph.images ) {
			UF_MSG_DEBUG("\tImage: {}", name);
		}
	}
*/

	UF_DEBUG_TIMER_MULTITRACE("Updating master graph");
#if UF_GRAPH_EXTENDED
	uf::graph::reload( graph );
#endif
	uf::graph::reload();
	
	UF_DEBUG_TIMER_MULTITRACE_END("Processed graph.");
}
void uf::graph::process( pod::Graph& graph, int32_t index, uf::Object& parent ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& storage = ::getGraphStorage( scene );

	auto& graphMetadataJson = graph.metadata;
	auto& node = graph.nodes[index];
	// 
	bool ignore = false;
	// ignore pesky light_Orientation nodes
	if ( uf::string::split( node.name, "_" ).back() == "Orientation" ) ignore = true;

	ext::json::Value tag = ext::json::find( node.name, graphMetadataJson["tags"] );
	if ( ext::json::isObject( tag ) ) {
		if ( graphMetadataJson["baking"]["enabled"].as<bool>(false) && !tag["bake"].as<bool>(true) ) ignore = true;
		if ( tag["ignore"].as<bool>() ) ignore = true;
	}
	bool isLight = graph.lights.count(node.name) > 0;

	if ( ignore ) return;
		
	// create child
	uf::Object* pointer = new uf::Object;
	parent.addChild(*pointer);

	uf::Object& entity = *pointer;
	node.entity = &entity;
	
	bool setName = entity.getName() == "Entity";
	auto& metadata = entity.getComponent<uf::ObjectBehavior::Metadata>();
	auto& metadataJson = entity.getComponent<uf::Serializer>();
	metadataJson["system"]["graph"]["name"] = node.name;
	metadataJson["system"]["graph"]["index"] = index;

	if ( ext::json::isObject( tag ) ) {
		if ( tag["action"].as<uf::stl::string>() == "load" ) {
			if ( tag["filename"].is<uf::stl::string>() ) {
				uf::stl::string filename = uf::io::resolveURI( tag["filename"].as<uf::stl::string>(), graphMetadataJson["root"].as<uf::stl::string>() );
				entity.load(filename);
			} else if ( ext::json::isObject( tag["payload"] ) ) {
				uf::Serializer json = tag["payload"];
				json["root"] = graphMetadataJson["root"];
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
	}

	// create as light
	{
		if ( isLight ) {
			auto& l = graph.lights[node.name];
			
		#if UF_USE_OPENGL
			metadata.system.ignoreGraph = true;
		#else
			metadata.system.ignoreGraph = graphMetadataJson["debug"]["static"].as<bool>();
		#endif
			float powerScale = graphMetadataJson["lights"]["scale"].as<float>(1);
			
			uf::Serializer metadataLight;
			metadataLight["radius"][0] = 0.001;
			metadataLight["radius"][1] = l.range;
			metadataLight["power"] = l.intensity * powerScale;

			metadataLight["color"][0] = l.color.x;
			metadataLight["color"][1] = l.color.y;
			metadataLight["color"][2] = l.color.z;

			metadataLight["shadows"] = graphMetadataJson["lights"]["shadows"].as<bool>();
			metadataLight["dynamic"] = false;

			if ( uf::string::matched( node.name, R"(/\bspot\b/)" ) ) {
				metadataLight["type"] = "spot";
			}

			if ( ext::json::isArray( graphMetadataJson["lights"]["radius"] ) ) {
				metadataLight["radius"] = graphMetadataJson["lights"]["radius"];
			}
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
		transform.reference = &parent.getComponent<pod::Transform<>>();
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

	// what a mess
	auto model = uf::transform::model( transform );
	
	// 
	if ( 0 <= node.mesh && node.mesh < graph.meshes.size() ) {
		{
			node.object = ::allocateObjectID( storage );
			auto objectKeyName = std::to_string( node.object );

			storage.entities[objectKeyName] = &entity;	
			storage.objects[objectKeyName] = pod::Instance::Object{
				.model = model,
				.previous = model,
			};
		}

		auto& mesh = storage.meshes.map[graph.meshes[node.mesh]];
		auto& primitives = storage.primitives.map[graph.primitives[node.mesh]];
		auto& instanceAddresses = storage.instanceAddresses.map[graph.primitives[node.mesh]];

		pod::Instance::Bounds bounds = {};
		size_t baseInstanceID = ::allocateInstanceID( storage, graph.primitives[node.mesh] );
		// setup instances
		for ( auto i = 0; i < primitives.size(); ++i ) {
			auto& primitive = primitives[i];
			auto& instance = primitive.instance;
			size_t instanceID = baseInstanceID + i;

			instance.objectID = node.object;
			instance.jointID = graphMetadataJson["renderer"]["skinned"].as<bool>() ? 0 : -1;

			bounds.min = uf::vector::min( bounds.min, instance.bounds.min );
			bounds.max = uf::vector::max( bounds.max, instance.bounds.max );

			if ( mesh.indirect.count && mesh.indirect.count <= primitives.size() ) {
				auto& attribute = mesh.indirect.attributes.front();
				auto& buffer = mesh.buffers[mesh.isInterleaved(mesh.indirect.interleaved) ? mesh.indirect.interleaved : attribute.buffer];
				pod::DrawCommand* drawCommands = (pod::DrawCommand*) buffer.data();
				auto& drawCommand = drawCommands[i];
				drawCommand.instanceID = instanceID;
			}
		}
	#if !UF_GRAPH_EXTENDED
		if ( graphMetadataJson["renderer"]["render"].as<bool>() ) {
			uf::graph::initializeGraphics( graph, entity, mesh, addresses );
		}
	#endif
		
		{
			auto phyziks = tag["physics"];
			if ( !ext::json::isObject( phyziks ) ) phyziks = metadataJson["physics"];
			else metadataJson["physics"] = phyziks;
			
			if ( ext::json::isObject( phyziks ) ) {
				uf::stl::string type = phyziks["type"].as<uf::stl::string>();		

				if ( type != "mesh" ) {
					auto min = bounds.min; // uf::matrix::multiply<float>( model, bounds.min, 1.0f );
					auto max = bounds.max; // uf::matrix::multiply<float>( model, bounds.max, 1.0f );

					pod::Vector3f center = (max + min) * 0.5f;
					pod::Vector3f corner = uf::vector::abs(max - min) * 0.5f;

					if ( ext::json::isNull( metadataJson["physics"]["center"] ) ) metadataJson["physics"]["center"] = uf::vector::encode( center );
					if ( ext::json::isNull( metadataJson["physics"]["corner"] ) ) metadataJson["physics"]["corner"] = uf::vector::encode( corner );
					if ( ext::json::isNull( metadataJson["physics"]["min"] ) ) metadataJson["physics"]["min"] = uf::vector::encode( min );
					if ( ext::json::isNull( metadataJson["physics"]["max"] ) ) metadataJson["physics"]["max"] = uf::vector::encode( max );
				}
			#if !UF_GRAPH_EXTENDED
				if ( type == "mesh" ) {
					auto& physicsBody = entity.getComponent<pod::PhysicsBody>();
					float mass = phyziks["mass"].as(physicsBody.mass);
					
					physicsBody.material.staticFriction = phyziks["friction"].as(physicsBody.material.staticFriction);
					physicsBody.material.restitution = phyziks["restitution"].as(physicsBody.material.restitution);
					physicsBody.inertiaTensor = uf::vector::decode( phyziks["inertia"], physicsBody.inertiaTensor );
					physicsBody.gravity = uf::vector::decode( phyziks["gravity"], physicsBody.gravity );
				
					uf::physics::impl::create( entity.as<uf::Object>(), mesh, mass );
				}
			#endif
			}
		}
	}

	for ( auto index : node.children ) uf::graph::process( graph, index, entity );
}

void uf::graph::destroy( pod::Graph& graph ) {
}

void uf::graph::initialize() {
	uf::graph::initialize( uf::scene::getCurrentScene() );
}
void uf::graph::initialize( uf::Object& object, size_t initialElements ) {
	return uf::graph::initialize( uf::graph::globalStorage ? uf::graph::storage : object.getComponent<pod::Graph::Storage>(), initialElements );
}
void uf::graph::initialize( pod::Graph::Storage& storage, size_t initialElements ) {
	storage.buffers.camera.initialize( (const void*) nullptr, sizeof(pod::Camera::Viewports), uf::renderer::enums::Buffer::UNIFORM );
	// to-do: check if opengl really needs these
	storage.buffers.drawCommands.initialize( (const void*) nullptr, sizeof(pod::DrawCommand)  * initialElements, uf::renderer::enums::Buffer::STORAGE );
	storage.buffers.instance.initialize( (const void*) nullptr, sizeof(pod::Instance) * initialElements, uf::renderer::enums::Buffer::STORAGE );
	storage.buffers.instanceAddresses.initialize( (const void*) nullptr, sizeof(pod::Instance::Addresses) * initialElements, uf::renderer::enums::Buffer::STORAGE );
	storage.buffers.lodMetadata.initialize( (const void*) nullptr, sizeof(pod::LODMetadata) * initialElements, uf::renderer::enums::Buffer::STORAGE );
	storage.buffers.joint.initialize( (const void*) nullptr, sizeof(pod::Matrix4f) * initialElements, uf::renderer::enums::Buffer::STORAGE );
	storage.buffers.object.initialize( (const void*) nullptr, sizeof(pod::Instance::Object) * initialElements, uf::renderer::enums::Buffer::STORAGE );
	storage.buffers.material.initialize( (const void*) nullptr, sizeof(pod::Material) * initialElements, uf::renderer::enums::Buffer::STORAGE );
	storage.buffers.texture.initialize( (const void*) nullptr, sizeof(pod::Texture) * initialElements, uf::renderer::enums::Buffer::STORAGE );
	storage.buffers.light.initialize( (const void*) nullptr, sizeof(pod::Light) * initialElements, uf::renderer::enums::Buffer::STORAGE );
}

void uf::graph::initialize( pod::Graph& graph ) {
	if ( graph.metadata["baking"]["enabled"].as<bool>() ) {
		auto& metadataJson = graph.root.entity->getComponent<uf::Serializer>();
		metadataJson["baking"] = graph.metadata["baking"];
		metadataJson["baking"]["root"] = uf::io::directory( graph.name );
		uf::instantiator::bind( "BakingBehavior", *graph.root.entity );
	}

	graph.root.entity->initialize();
	graph.root.entity->process([&]( uf::Entity* entity ) {
		if ( entity->getUid() == 0 ) entity->initialize();
	});

	auto& scene = uf::scene::getCurrentScene();
	scene.invalidateGraph();
}

void uf::graph::tick() {
	uf::graph::tick( uf::scene::getCurrentScene() );
}
void uf::graph::tick( uf::Object& object ) {
	auto& storage = uf::graph::globalStorage ? uf::graph::storage : object.getComponent<pod::Graph::Storage>();
	::shouldRebind = uf::graph::tick( storage );
}
bool uf::graph::tick( pod::Graph::Storage& storage ) {
	bool rebuild = false;

	static thread_local uf::stl::vector<pod::Instance> instances;
	static thread_local uf::stl::vector<pod::Instance::Addresses> instanceAddresses;
	static thread_local uf::stl::vector<pod::LODMetadata> lodMetadata;
	static thread_local uf::stl::vector<pod::Matrix4f> joints;
	static thread_local uf::stl::vector<pod::Instance::Object> objects;
	static thread_local uf::stl::vector<pod::Material> materials;
	static thread_local uf::stl::vector<pod::Texture> textures;
	static thread_local uf::stl::vector<pod::DrawCommand> drawCommands;

	joints.clear();
	for ( auto& key : storage.joints.keys ) {
		joints.insert( joints.end(), storage.joints.map[key].begin(), storage.joints.map[key].end() );
	}

	objects.clear();
	for ( auto& key : storage.objects.keys ) {
		auto& entity = *storage.entities.map[key];
		auto& object = storage.objects.map[key];

		if ( entity.isValid() ) {
			auto& metadata = entity.getComponent<uf::ObjectBehavior::Metadata>();
			auto& transform = entity.getComponent<pod::Transform<>>();
			
			if ( !metadata.system.ignoreGraph ) {
				object.previous = object.model;
				object.model = uf::transform::model( transform );
			}
		}

		objects.emplace_back( object );
	}

	if ( !joints.empty() ) rebuild = storage.buffers.joint.update( (const void*) joints.data(), joints.size() * sizeof(pod::Matrix4f) ) || rebuild;
	rebuild = storage.buffers.object.update( (const void*) objects.data(), objects.size() * sizeof(pod::Instance::Object) ) || rebuild;

	if ( ::newGraphAdded ) {
		drawCommands.clear();
		instances.clear();
		lodMetadata.clear();
		
		for ( auto& key : storage.primitives.keys ) {
			for ( auto& primitive : storage.primitives[key] ) {
				drawCommands.emplace_back( primitive.drawCommand );
				instances.emplace_back( primitive.instance );
				lodMetadata.emplace_back( primitive.lod );
			}
		}

		instanceAddresses.clear();
		for ( auto& key : storage.instanceAddresses.keys ) {
			instanceAddresses.insert( instanceAddresses.end(), storage.instanceAddresses.map[key].begin(), storage.instanceAddresses.map[key].end() );
		}

		textures.clear();
		for ( auto& key : storage.textures.keys ) textures.emplace_back( storage.textures.map[key] );

		materials.clear();
		for ( auto& key : storage.materials.keys ) materials.emplace_back( storage.materials.map[key] );

		rebuild = storage.buffers.instance.update( (const void*) instances.data(), instances.size() * sizeof(pod::Instance) ) || rebuild;
		rebuild = storage.buffers.instanceAddresses.update( (const void*) instanceAddresses.data(), instanceAddresses.size() * sizeof(pod::Instance::Addresses) ) || rebuild;
		rebuild = storage.buffers.drawCommands.update( (const void*) drawCommands.data(), drawCommands.size() * sizeof(pod::DrawCommand) ) || rebuild;
		rebuild = storage.buffers.lodMetadata.update( (const void*) lodMetadata.data(), lodMetadata.size() * sizeof(pod::LODMetadata) ) || rebuild;
		rebuild = storage.buffers.material.update( (const void*) materials.data(), materials.size() * sizeof(pod::Material) ) || rebuild;
		rebuild = storage.buffers.texture.update( (const void*) textures.data(), textures.size() * sizeof(pod::Texture) ) || rebuild;

		::newGraphAdded = false;
	}


	if ( rebuild ) {
		UF_MSG_DEBUG("Graph buffers requesting renderer update");
		uf::renderer::states::rebuild = true;

		if ( uf::renderer::hasRenderMode("", true) ) {
			auto& renderMode = uf::renderer::getRenderMode("", true);

		#if UF_USE_VULKAN
			auto& blitter = renderMode.getBlitter();
			auto& shader = blitter.material.getShader(blitter.material.hasShader("compute", "deferred") ? "compute" : "fragment", "deferred");

			shader.metadata.aliases.buffers.clear();

			shader.aliasBuffer( "drawCommands", storage.buffers.drawCommands );
			shader.aliasBuffer( "instance", storage.buffers.instance );
			shader.aliasBuffer( "instanceAddresses", storage.buffers.instanceAddresses );
			shader.aliasBuffer( "material", storage.buffers.material );
			shader.aliasBuffer( "texture", storage.buffers.texture );
			shader.aliasBuffer( "light", storage.buffers.light );
		#endif
		}
	}

	return rebuild;
}
void uf::graph::render() {
	uf::graph::render( uf::scene::getCurrentScene() );
}
void uf::graph::render( uf::Object& object ) {
	return uf::graph::render( uf::graph::globalStorage ? uf::graph::storage : object.getComponent<pod::Graph::Storage>() );
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

//	if ( controller.getName() == "Player" ) UF_MSG_DEBUG("frame={}, camera={}, renderMode={}, {}", uf::time::frame, uf::matrix::toString( viewport.matrices[0].view ), renderMode->getName(), renderMode->getType() );

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
	return uf::graph::destroy( uf::graph::globalStorage ? uf::graph::storage : object.getComponent<pod::Graph::Storage>(), soft );
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
	for ( auto pair : storage.texture2Ds.map ) pair.second.destroy();
	for ( auto& t : storage.shadow2Ds ) t.destroy();
	for ( auto& t : storage.shadowCubes ) t.destroy();

	for ( auto pair : storage.atlases.map ) pair.second.clear();
	for ( auto pair : storage.images.map ) pair.second.clear();
	for ( auto pair : storage.meshes.map ) pair.second.destroy();

	// cleanup storage cache
	storage.instanceAddresses.clear();
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
	storage.texture2Ds.clear();
	storage.entities.clear();
	storage.shadow2Ds.clear();
	storage.shadowCubes.clear();

	// cleanup storage buffers
	if ( !soft ) {
		storage.buffers.camera.destroy(true);
		storage.buffers.drawCommands.destroy(true);
		storage.buffers.instance.destroy(true);
		storage.buffers.instanceAddresses.destroy(true);
		storage.buffers.joint.destroy(true);
		storage.buffers.object.destroy(true);
		storage.buffers.material.destroy(true);
		storage.buffers.texture.destroy(true);
		storage.buffers.light.destroy(true);
	}

	uf::renderer::states::rebuild = true;
}

void uf::graph::reload( pod::Graph& graph, pod::Node& node ) {
	if ( !(0 <= node.mesh && node.mesh < graph.meshes.size()) ) return;
	if ( !node.entity ) return;

	auto& scene = uf::scene::getCurrentScene();
	auto& storage = ::getGraphStorage( scene );

	auto& entity = node.entity->as<uf::Object>();

	auto& metadata = entity.getComponent<uf::ObjectBehavior::Metadata>();
	auto& metadataJson = entity.getComponent<uf::Serializer>();
	auto& transform = entity.getComponent<pod::Transform<>>();

	auto& graphMetadataJson = graph.metadata;
	
	ext::json::Value tag = ext::json::find( node.name, graphMetadataJson["tags"] );

	pod::Vector3f controllerPosition = {};
	auto& controller = scene.getController();
	if ( controller.getName() != "Scene" ) {
		auto& controllerTransform = controller.getComponent<pod::Transform<>>();
		controllerPosition = controllerTransform.position;
	} else {
		// find info_player_spawn
		// to-do: deduce the node via tag that attaches the player
		for ( auto& node : graph.nodes ) {
			if ( node.name != graph.settings.stream.player ) continue;
			auto& controllerTransform = node.entity->getComponent<pod::Transform<>>();
			controllerPosition = controllerTransform.position;
			break;
		}
	}

	auto model = uf::transform::model( transform );
	auto& mesh = storage.meshes.map[graph.meshes[node.mesh]];
	auto& primitives = storage.primitives.map[graph.primitives[node.mesh]];
	auto& instanceAddresses = storage.instanceAddresses.map[graph.primitives[node.mesh]];

	float radius = graph.settings.stream.radius;
	float radiusSquared = radius * radius;

	// disable if not tagged for streaming
	// to-do: check tag
	if ( graph.settings.stream.tag != "" && node.name != graph.settings.stream.tag ) {
		radius = 0;
	}

	if ( mesh.buffer_paths.empty() ) {
		radius = 0;
	}

	if ( radius > 0 && mesh.indirect.count && mesh.indirect.count <= primitives.size() ) {
		// deduce draw command (indirect) buffer to write to
		auto& attribute = mesh.indirect.attributes.front();
		auto& buffer = mesh.buffers[mesh.isInterleaved(mesh.indirect.interleaved) ? mesh.indirect.interleaved : attribute.buffer];
		pod::DrawCommand* drawCommands = (pod::DrawCommand*) buffer.data();
		// queues
		uf::stl::unordered_map<size_t, uf::stl::vector<pod::Range>> ranges;
		uf::stl::vector<bool> queuedDrawIDs( primitives.size(), false ); // this is to maintain draw command order because apparently my code requires draw commands to stay in order
		// fallbacks for when no draw calls are requested (mainly for the collision mesh)
		float closestDistance = std::numeric_limits<float>::max();
		size_t closestDrawID = 0;
		bool found = false;

		// iterate through meshlets and cull if out of radius
		for ( size_t drawID = 0; drawID < primitives.size(); ++drawID ) {
			auto& primitive = primitives[drawID];
			auto& instance = primitive.instance;
			auto& drawCommand = primitive.drawCommand;

			pod::Vector3f center = uf::matrix::multiply( model, (instance.bounds.max + instance.bounds.min) * 0.5f, 1.0f ); // transform the center of the draw call
			float distanceSquared = uf::vector::distanceSquared( center, controllerPosition ); // saves a sqrt()

			// store closest draw call
			if ( distanceSquared < closestDistance ) {
				closestDistance = distanceSquared;
				closestDrawID = drawID;
			}
			// queue if we're within the radius
			if ( (queuedDrawIDs[drawID] = distanceSquared <= radiusSquared) ) {
				found = true;
			}
		}

		// insert closest primitive if all are out of range (because of cringe logic)
		if ( !found ) {
			queuedDrawIDs[closestDrawID] = true;
		}

		// bail if no update is detected
		auto drawCommandHash = ::fnv1aHash(queuedDrawIDs);
		graph.settings.stream.lastUpdate = uf::physics::time::current;

		if ( drawCommandHash == graph.settings.stream.hash ) {
			return;
		}
		graph.settings.stream.hash = drawCommandHash;

	// read from disk
	#if UF_GRAPH_SPARSE_READ_MESH
		// needs to be dequantized first, naively copying the descriptor settings just doesn't work
		{
		#if UF_ENV_DREAMCAST && GL_QUANTIZED_SHORT
			mesh.convert<uint16_t, float>();
		#else
			auto conversion = graphMetadataJson["decode"]["conversion"].as<uf::stl::string>();
			if ( conversion != "" ) {
			#if UF_USE_FLOAT16
				if ( conversion == "float16" ) mesh.convert<float16, float>();
				else if ( conversion == "float" ) mesh.convert<float, float16>();
			#endif
			#if UF_USE_BFLOAT16
				if ( conversion == "bfloat16" ) mesh.convert<bfloat16, float>();
				else if ( conversion == "float" ) mesh.convert<float, bfloat16>();
			#endif
				if ( conversion == "uint16_t" ) mesh.convert<uint16_t, float>();
				else if ( conversion == "float" ) mesh.convert<float, uint16_t>();
			}
		#endif
		}

		// reset counts
		mesh.vertex.count = 0;
		mesh.index.count = 0;

		for (size_t drawID = 0; drawID < queuedDrawIDs.size(); ++drawID) {
			bool queued = queuedDrawIDs[drawID];
			auto& primitive = primitives[drawID];
			auto& drawCommand = drawCommands[drawID];
			
			// disable draw call
			if ( !queued ) {
				drawCommand.instances = 0;
				drawCommand.vertices = 0;
				drawCommand.indices = 0;
				drawCommand.vertexID = 0;
				drawCommand.indexID = 0;
				continue;
			}

			// queue up ranges to read from disk
			for (auto& attribute : mesh.index.attributes) {
				auto size = attribute.descriptor.size;
				ranges[attribute.buffer].emplace_back(pod::Range{
					primitive.drawCommand.indexID * size,
					primitive.drawCommand.indices * size,
				});
			}
			for (auto& attribute : mesh.vertex.attributes) {
				auto size = attribute.descriptor.size;
				ranges[attribute.buffer].emplace_back(pod::Range{
					primitive.drawCommand.vertexID * size,
					primitive.drawCommand.vertices * size,
				});
			}

			// reset draw call and remap
			drawCommand = primitive.drawCommand;
			drawCommand.vertexID = mesh.vertex.count;
			drawCommand.indexID  = mesh.index.count;
			// increment remap indices
			mesh.vertex.count += drawCommand.vertices;
			mesh.index.count  += drawCommand.indices;
		}

		#define STREAM_MESH_DATA( N ) \
			for ( auto& attribute : mesh.N.attributes ) {\
				if ( ranges.count(attribute.buffer) <= 0 ) { \
					mesh.buffers[attribute.buffer].clear();\
				} else {\
					uf::io::readAsBuffer( mesh.buffers[attribute.buffer], mesh.buffer_paths[attribute.buffer], ranges[attribute.buffer] );\
				}\
			}

		STREAM_MESH_DATA( index );
		STREAM_MESH_DATA( vertex );
	// keep the vertex data intact
	#else
		// disable remaining draw commands
		for ( auto drawID = 0; drawID < primitives.size(); ++drawID ) {
			bool queued = queuedDrawIDs[drawID];
			if ( !queued ) {
				drawCommands[drawID].instances = 0;
				drawCommands[drawID].vertices = 0;
				drawCommands[drawID].indices = 0;
				drawCommands[drawID].indexID = 0;
				drawCommands[drawID].vertexID = 0;
			} else {
				drawCommands[drawID] = primitives[drawID].drawCommand;
			}
		}

		#define STREAM_MESH_DATA( N ) \
			for ( auto& attribute : mesh.N.attributes ) {\
				if ( !mesh.buffers[attribute.buffer].empty() || mesh.buffer_paths.empty() ) continue;\
				uf::io::readAsBuffer( mesh.buffers[attribute.buffer], mesh.buffer_paths[attribute.buffer] );\
			}

		STREAM_MESH_DATA( index );
		STREAM_MESH_DATA( vertex );
	#endif
		
		if ( graph.settings.stream.textures ) {
			// cringe macro that ensures a texture ID is mapped properly, regardless if its visible or not
			// lightmaps are not sRGB, while textures (usually) are
			#define INCREMENT_TEXTURE_REFCOUNT( ID, isSRGB ) if ( 0 <= ID && ID < graph.textures.size() ) {\
				auto& key = graph.textures[ID];\
				textureReferences[key] += visible ? 1 : 0;\
				isSrgb[key] = isSRGB;\
			}

			uf::stl::unordered_map<uf::stl::string, bool> isSrgb; // cringe
			uf::stl::unordered_map<uf::stl::string, size_t> textureReferences;
			// determine which textures are in use or not
			for ( size_t drawID = 0; drawID < primitives.size(); ++drawID ) {
				auto& primitive = primitives[drawID];
				auto& instance = primitive.instance;
				auto& drawCommand = drawCommands[drawID];

				bool visible = drawCommand.instances > 0;
				
				INCREMENT_TEXTURE_REFCOUNT(instance.lightmapID, false);
				// no material information bound
				if ( !(0 <= instance.materialID && instance.materialID < graph.materials.size()) ) {
					continue;
				}
				auto& material = storage.materials[graph.materials[instance.materialID]];
				INCREMENT_TEXTURE_REFCOUNT(material.indexAlbedo, true);
				INCREMENT_TEXTURE_REFCOUNT(material.indexNormal, true);
				INCREMENT_TEXTURE_REFCOUNT(material.indexEmissive, true);
				INCREMENT_TEXTURE_REFCOUNT(material.indexOcclusion, true);
				INCREMENT_TEXTURE_REFCOUNT(material.indexMetallicRoughness, true);
			}

			// iterate through our ref counts
			for ( auto& [ key, count ] : textureReferences ) {
				auto& texture = storage.texture2Ds[key];
				auto& image = storage.images[key];
				bool visible = count > 0;

				if ( visible && (!texture.generated() || texture.aliased) ) {
					// load image
					if ( image.getPixels().empty() ) image.open(image.getFilename(), false);

					auto filter = uf::renderer::enums::Filter::LINEAR;
					auto tag = ext::json::find( key, graphMetadataJson["tags"] );
					if ( !ext::json::isObject( tag ) ) {
						tag["renderer"] = graphMetadataJson["renderer"];
					}
					if ( tag["renderer"]["filter"].is<uf::stl::string>() ) {
						const auto mode = uf::string::lowercase( tag["renderer"]["filter"].as<uf::stl::string>("linear") );
						if ( mode == "linear" ) filter = uf::renderer::enums::Filter::LINEAR;
						else if ( mode == "nearest" ) filter = uf::renderer::enums::Filter::NEAREST;
						else UF_MSG_WARNING("Invalid Filter enum string specified: {}", mode);
					}

					// avoids manipulating the aliased texture
					if ( texture.aliased ) {
						texture.aliased = false;
					#if UF_USE_OPENGL
						texture.image = 0;
					#else
						texture.image = {};
						texture.view = {};
					#endif
					}

					texture.sampler.descriptor.filter.min = filter;
					texture.sampler.descriptor.filter.mag = filter;
					texture.srgb = isSrgb[key];

					texture.loadFromImage( image );
				#if UF_ENV_DREAMCAST
					image.clear();
				#endif
				} else if ( !visible && (texture.generated() && !texture.aliased) ) {
					// unload image
					image.clear();
					// defer destruction of texture
					texture.destroy( true );
					// alias to null texture
					texture.aliasTexture(uf::renderer::Texture2D::empty);
				}
			}
		}
	} else { // this shouldn't be reached
		#define LOAD_MESH_DATA( N ) \
			for ( auto& attribute : mesh.N.attributes ) {\
				if ( !mesh.buffers[attribute.buffer].empty() || mesh.buffer_paths.empty() ) continue;\
				uf::io::readAsBuffer( mesh.buffers[attribute.buffer], mesh.buffer_paths[attribute.buffer] );\
			}

		LOAD_MESH_DATA( index );
		LOAD_MESH_DATA( vertex );
	}

	// in the event streamed in mesh data from any pathway isn't already converted
	{
	#if UF_ENV_DREAMCAST && GL_QUANTIZED_SHORT
		mesh.convert<float, uint16_t>();
	#else
		auto conversion = graphMetadataJson["decode"]["conversion"].as<uf::stl::string>();
		if ( conversion != "" ) {
		#if UF_USE_FLOAT16
			if ( conversion == "float16" ) mesh.convert<float, float16>();
			else if ( conversion == "float" ) mesh.convert<float16, float>();
		#endif
		#if UF_USE_BFLOAT16
			if ( conversion == "bfloat16" ) mesh.convert<float, bfloat16>();
			else if ( conversion == "float" ) mesh.convert<bfloat16, float>();
		#endif
			if ( conversion == "uint16_t" ) mesh.convert<float, uint16_t>();
			else if ( conversion == "float" ) mesh.convert<uint16_t, float>();
		}
	#endif
	}

	mesh.updateDescriptor();

	// necessary for OpenGL because recorded descriptors have invalidated pointers
	// Vulkan doesn't care about the CPU-side mesh data
#if UF_USE_OPENGL
	uf::renderer::states::rebuild = true;
#endif

	// update graphic
	if ( graphMetadataJson["renderer"]["render"].as<bool>() ) {
		bool exists = entity.hasComponent<uf::renderer::Graphic>();
		if ( exists ) {
			auto& graphic = entity.getComponent<uf::renderer::Graphic>();
			bool rebuild = graphic.updateMesh( mesh );
			// update texture descriptors
			::bindTextures( graphic );
			// update buffers if any of them were resized (because my aliasing system is weak)
			if ( rebuild ) {
				::bindBuffers( graphic, mesh );
				
				::bindInstanceAddresses( graphic, mesh, instanceAddresses );
				uf::renderer::states::rebuild = true;
			}
		} else {
			uf::graph::initializeGraphics( graph, entity, mesh, instanceAddresses );
		}
	}
	// bind mesh to physics state
	// to-do: figure out why the mesh just suddenly breaks when re-streamed in dreamcast (could just be the version of reactphysics)
	{
		auto phyziks = tag["physics"];
		if ( !ext::json::isObject( phyziks ) ) phyziks = metadataJson["physics"];
		else metadataJson["physics"] = phyziks;

		if ( ext::json::isObject( phyziks ) ) {
			uf::stl::string type = phyziks["type"].as<uf::stl::string>();	
			if ( type == "mesh" ) {
				bool exists = entity.hasComponent<pod::PhysicsBody>();
				if ( exists ) {
					uf::physics::impl::destroy( entity );
				}
				
				auto& physicsBody = entity.getComponent<pod::PhysicsBody>();
				float mass = phyziks["mass"].as(physicsBody.mass);
				
				physicsBody.material.staticFriction = phyziks["friction"].as(physicsBody.material.staticFriction);
				physicsBody.material.restitution = phyziks["restitution"].as(physicsBody.material.restitution);
				physicsBody.inertiaTensor = uf::vector::decode( phyziks["inertia"], physicsBody.inertiaTensor );
				physicsBody.gravity = uf::vector::decode( phyziks["gravity"], physicsBody.gravity );
			
				uf::physics::impl::create( entity.as<uf::Object>(), mesh, mass );
			}
		}
	}
}
void uf::graph::reload( pod::Graph& graph ) {
	// update graphics
	for ( auto& node : graph.nodes ) uf::graph::reload( graph, node );

	// setup combined mesh if requested
	// ::combineMesh( graph );
}
void uf::graph::reload() {
	::newGraphAdded = true;
}

void uf::graph::update( pod::Graph& graph ) {
	return update( graph, uf::physics::time::delta );
}
void uf::graph::update( pod::Graph& graph, float delta ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& storage = ::getGraphStorage( scene );

	// rebuild
	if ( ::shouldRebind ) {
		for ( auto& node : graph.nodes ) {
			if ( !(0 <= node.mesh && node.mesh < graph.meshes.size()) ) continue;
			if ( !node.entity ) continue;

			auto& entity = node.entity->as<uf::Object>();

			if ( !entity.hasComponent<uf::renderer::Graphic>() ) continue;

			auto& graphic = entity.getComponent<uf::renderer::Graphic>();
			auto& mesh = storage.meshes.map[graph.meshes[node.mesh]];
			auto& instanceAddresses = storage.instanceAddresses.map[graph.primitives[node.mesh]];

			::bindBuffers( graphic, mesh );
			::bindInstanceAddresses( graphic, mesh, instanceAddresses );
		}
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
