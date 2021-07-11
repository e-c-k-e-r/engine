#include <uf/engine/graph/graph.h>
#include <uf/ext/bullet/bullet.h>
#include <uf/ext/gltf/gltf.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/mesh/grid.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/string/base64.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/ext/xatlas/xatlas.h>

#if UF_ENV_DREAMCAST
	#define UF_GRAPH_LOAD_MULTITHREAD 0
#else
	#define UF_GRAPH_LOAD_MULTITHREAD 1
#endif

pod::Graph::Storage uf::graph::storage;

UF_VERTEX_DESCRIPTOR(uf::graph::mesh::Base,
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base, R32G32_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base, R32G32_SFLOAT, st)
);
UF_VERTEX_DESCRIPTOR(uf::graph::mesh::ID,
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::ID, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::ID, R32G32_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::ID, R32G32_SFLOAT, st)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::ID, R32G32B32_SFLOAT, normal)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::ID, R32G32B32_SFLOAT, tangent)
);
UF_VERTEX_DESCRIPTOR(uf::graph::mesh::Skinned,
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R32G32_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R32G32_SFLOAT, st)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R32G32B32_SFLOAT, normal)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R32G32B32_SFLOAT, tangent)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R16G16B16A16_SINT, joints)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R32G32B32A32_SFLOAT, weights)
);

namespace {
	void initializeShaders( pod::Graph& graph, uf::Object& entity ) {
		auto& graphic = entity.getComponent<uf::Graphic>();
		if ( graphic.material.shaders.size() > 0 ) return;

		uf::stl::string root = uf::io::directory( graph.name );
		size_t texture2Ds = 0;
		size_t texture3Ds = 0;
		for ( auto& texture : graphic.material.textures ) {
			if ( texture.width > 1 && texture.height > 1 && texture.depth == 1 && texture.layers == 1 ) ++texture2Ds;
			else if ( texture.width > 1 && texture.height > 1 && texture.depth > 1 && texture.layers == 1 ) ++texture3Ds;
		}

		// standard pipeline
		{
			uf::stl::string vertexShaderFilename = graph.metadata["shaders"]["vertex"].as<uf::stl::string>("/gltf/base.vert.spv");
			uf::stl::string geometryShaderFilename = graph.metadata["shaders"]["geometry"].as<uf::stl::string>("");
			uf::stl::string fragmentShaderFilename = graph.metadata["shaders"]["fragment"].as<uf::stl::string>("/gltf/base.frag.spv");
			if ( uf::renderer::settings::experimental::deferredSampling ) {
				fragmentShaderFilename = uf::string::replace( fragmentShaderFilename, "frag", "deferredSampling.frag" );
			}
			{
				if ( !graph.metadata["flags"]["SEPARATE"].as<bool>() ) {
					vertexShaderFilename = graph.metadata["flags"]["SKINNED"].as<bool>() ? "/gltf/skinned.instanced.vert.spv" : "/gltf/instanced.vert.spv";
				} else if ( graph.metadata["flags"]["SKINNED"].as<bool>() ) vertexShaderFilename = "/gltf/skinned.vert.spv";
				vertexShaderFilename = entity.grabURI( vertexShaderFilename, root );
				graphic.material.attachShader(vertexShaderFilename, uf::renderer::enums::Shader::VERTEX);
			}
			{
				fragmentShaderFilename = entity.grabURI( fragmentShaderFilename, root );
				graphic.material.attachShader(fragmentShaderFilename, uf::renderer::enums::Shader::FRAGMENT);
			}
		#if UF_USE_VULKAN
			if ( geometryShaderFilename != "" && uf::renderer::device.enabledFeatures.geometryShader ) {
				geometryShaderFilename = entity.grabURI( geometryShaderFilename, root );
				graphic.material.attachShader(geometryShaderFilename, uf::renderer::enums::Shader::GEOMETRY);
			}
			{
				uint32_t maxPasses = 6;

				auto& shader = graphic.material.getShader("vertex");
				uint32_t* specializationConstants = (uint32_t*) (void*) &shader.specializationConstants;
				ext::json::forEach( shader.metadata.json["specializationConstants"], [&]( size_t i, ext::json::Value& sc ){
					uf::stl::string name = sc["name"].as<uf::stl::string>();
					if ( name == "PASSES" ) sc["value"] = (specializationConstants[i] = maxPasses);
				});
			}
			{
				uint32_t maxTextures = texture2Ds;

				auto& shader = graphic.material.getShader("fragment");
				uint32_t* specializationConstants = (uint32_t*) (void*) &shader.specializationConstants;
				ext::json::forEach( shader.metadata.json["specializationConstants"], [&]( size_t i, ext::json::Value& sc ){
					uf::stl::string name = sc["name"].as<uf::stl::string>();
					if ( name == "TEXTURES" ) sc["value"] = (specializationConstants[i] = maxTextures);
				});

				ext::json::forEach( shader.metadata.json["definitions"]["textures"], [&]( ext::json::Value& t ){
					if ( t["name"].as<uf::stl::string>() != "samplerTextures" ) return;
					size_t binding = t["binding"].as<size_t>();
					for ( auto& layout : shader.descriptorSetLayoutBindings ) {
						if ( layout.binding == binding ) layout.descriptorCount = maxTextures;
					}
				});
			}
		#endif
		}
		// vxgi pipeline
		if ( uf::renderer::settings::experimental::vxgi ) {
			uf::stl::string vertexShaderFilename = graph.metadata["shaders"]["vertex"].as<uf::stl::string>("/gltf/base.vert.spv");
			uf::stl::string geometryShaderFilename = graph.metadata["shaders"]["geometry"].as<uf::stl::string>("/gltf/voxelize.geom.spv");
			uf::stl::string fragmentShaderFilename = graph.metadata["shaders"]["fragment"].as<uf::stl::string>("/gltf/voxelize.frag.spv");
			if ( uf::renderer::settings::experimental::deferredSampling ) {
			//	fragmentShaderFilename = uf::string::replace( fragmentShaderFilename, "frag", "deferredSampling.frag" );
			}
			{
				fragmentShaderFilename = entity.grabURI( fragmentShaderFilename, root );
				graphic.material.attachShader(fragmentShaderFilename, uf::renderer::enums::Shader::FRAGMENT, "vxgi");
			}
		#if UF_USE_VULKAN
			if ( geometryShaderFilename != "" && uf::renderer::device.enabledFeatures.geometryShader ) {
				geometryShaderFilename = entity.grabURI( geometryShaderFilename, root );
				graphic.material.attachShader(geometryShaderFilename, uf::renderer::enums::Shader::GEOMETRY, "vxgi");
			}
			{
				auto& scene = uf::scene::getCurrentScene();
				auto& sceneTextures = scene.getComponent<pod::SceneTextures>();
				uint32_t voxelTypes = 0;
				if ( !sceneTextures.voxels.id.empty() ) ++voxelTypes;
				if ( !sceneTextures.voxels.normal.empty() ) ++voxelTypes;
				if ( !sceneTextures.voxels.uv.empty() ) ++voxelTypes;
				if ( !sceneTextures.voxels.radiance.empty() ) ++voxelTypes;
				if ( !sceneTextures.voxels.depth.empty() ) ++voxelTypes;

				uint32_t maxTextures = texture2Ds;
				uint32_t maxCascades = texture3Ds / voxelTypes;

				auto& shader = graphic.material.getShader("fragment", "vxgi");
				uint32_t* specializationConstants = (uint32_t*) (void*) &shader.specializationConstants;

				ext::json::forEach( shader.metadata.json["specializationConstants"], [&]( size_t i, ext::json::Value& sc ){
					uf::stl::string name = sc["name"].as<uf::stl::string>();
					if ( name == "TEXTURES" ) sc["value"] = (specializationConstants[i] = maxTextures);
					else if ( name == "CASCADES" ) sc["value"] = (specializationConstants[i] = maxCascades);
				});
				ext::json::forEach( shader.metadata.json["definitions"]["textures"], [&]( ext::json::Value& t ){
					size_t binding = t["binding"].as<size_t>();
					uf::stl::string name = t["name"].as<uf::stl::string>();
					for ( auto& layout : shader.descriptorSetLayoutBindings ) {
						if ( layout.binding != binding ) continue;
						if ( name == "samplerTextures" ) layout.descriptorCount = maxTextures;
						else if ( name == "voxelId" ) layout.descriptorCount = maxCascades;
						else if ( name == "voxelUv" ) layout.descriptorCount = maxCascades;
						else if ( name == "voxelNormal" ) layout.descriptorCount = maxCascades;
						else if ( name == "voxelRadiance" ) layout.descriptorCount = maxCascades;
						else if ( name == "voxelDepth" ) layout.descriptorCount = maxCascades;
					}
				});
			}
		#endif
		}
	//	graphic.process = true;
	}
	void initializeGraphics( pod::Graph& graph, uf::Object& entity ) {
		auto& graphic = entity.getComponent<uf::Graphic>();
		graphic.device = &uf::renderer::device;
		graphic.material.device = &uf::renderer::device;
		graphic.descriptor.frontFace = uf::renderer::enums::Face::CCW;
		graphic.descriptor.cullMode = !(graph.metadata["flags"]["INVERT"].as<bool>()) ? uf::renderer::enums::CullMode::BACK : uf::renderer::enums::CullMode::FRONT;
		if ( graph.metadata["cull mode"].is<uf::stl::string>() ) {
			if ( graph.metadata["cull mode"].as<uf::stl::string>() == "back" ) {
				graphic.descriptor.cullMode = uf::renderer::enums::CullMode::BACK;
			} else if ( graph.metadata["cull mode"].as<uf::stl::string>() == "front" ) {
				graphic.descriptor.cullMode = uf::renderer::enums::CullMode::FRONT;
			} else if ( graph.metadata["cull mode"].as<uf::stl::string>() == "none" ) {
				graphic.descriptor.cullMode = uf::renderer::enums::CullMode::NONE;
			} else if ( graph.metadata["cull mode"].as<uf::stl::string>() == "both" ) {
				graphic.descriptor.cullMode = uf::renderer::enums::CullMode::BOTH;
			}
		}
		{
			for ( auto& t : graph.textures ) {
				auto& texture2D = uf::graph::storage.texture2Ds[t];
				graphic.material.textures.emplace_back().aliasTexture(uf::graph::storage.texture2Ds[t]);
			}
			for ( auto& s : graph.samplers ) {
				graphic.material.samplers.emplace_back( uf::graph::storage.samplers[s] );
			}
			// bind scene's voxel texture
			if ( uf::renderer::settings::experimental::vxgi ) {
				auto& scene = uf::scene::getCurrentScene();
				auto& sceneTextures = scene.getComponent<pod::SceneTextures>();
				for ( auto& t : sceneTextures.voxels.id ) graphic.material.textures.emplace_back().aliasTexture(t);
				for ( auto& t : sceneTextures.voxels.normal ) graphic.material.textures.emplace_back().aliasTexture(t);
				for ( auto& t : sceneTextures.voxels.uv ) graphic.material.textures.emplace_back().aliasTexture(t);
				for ( auto& t : sceneTextures.voxels.radiance ) graphic.material.textures.emplace_back().aliasTexture(t);
				for ( auto& t : sceneTextures.voxels.depth ) graphic.material.textures.emplace_back().aliasTexture(t);
			}
		}
		
		initializeShaders( graph, entity );
	}
}

pod::Matrix4f uf::graph::local( pod::Graph& graph, int32_t index ) {
	auto& node = 0 < index && index <= graph.nodes.size() ? graph.nodes[index] : graph.root;
	return
		uf::matrix::translate( uf::matrix::identity(), node.transform.position ) *
		uf::quaternion::matrix(node.transform.orientation) *
		uf::matrix::scale( uf::matrix::identity(), node.transform.scale ) *
		node.transform.model;
}
pod::Matrix4f uf::graph::matrix( pod::Graph& graph, int32_t index ) {
	pod::Matrix4f matrix = local( graph, index );
	auto& node = *uf::graph::find( graph, index );
	int32_t parent = node.parent;
	while ( 0 < parent && parent <= graph.nodes.size() ) {
		matrix = local( graph, parent ) * matrix;
		parent = graph.nodes[parent].parent;
	}
	return matrix;
}
pod::Node* uf::graph::find( pod::Graph& graph, int32_t index ) {
	return 0 <= index && index < graph.nodes.size() ? &graph.nodes[index] : NULL;
}
pod::Node* uf::graph::find( pod::Graph& graph, const uf::stl::string& name ) {
	for ( auto& node : graph.nodes ) if ( node.name == name ) return &node;
	return NULL;
}

void uf::graph::process( uf::Object& entity ) {
	auto& graph = entity.getComponent<pod::Graph>();
	for ( auto index : graph.root.children ) process( graph, index, entity );
}
void uf::graph::process( pod::Graph& graph ) {
	//
	if ( !graph.root.entity ) graph.root.entity = new uf::Object;

	// process lightmap

	// add atlas

	// setup textures
	
	// process nodes
	for ( auto index : graph.root.children ) process( graph, index, *graph.root.entity );

	// setup meshes
	if ( !(graph.metadata["flags"]["SEPARATE"].as<bool>()) ) {
		initializeGraphics( graph, *graph.root.entity );
	}

	// setup collision
#if 0
	graph.root.entity->process([&]( uf::Entity* entity ) {
		auto& metadata = entity->getComponent<uf::Serializer>();
		size_t index = metadata["system"]["graph"]["index"].as<size_t>();
		auto& node = graph.nodes[nodeName];
		auto& nodeName = node.name;

		uf::Mesh* mesh = NULL;
		if ( 0 <= node.mesh && node.mesh < graph.meshes.size() ) {
			mesh = &uf::graphs::storage[graph.meshes[node.mesh]];
		}

		if ( !ext::json::isNull( graph.metadata["tags"][nodeName] ) ) {
			auto& info = graph.metadata["tags"][nodeName];
			if ( ext::json::isObject( info["collision"] ) ) {
				uf::stl::string type = info["collision"]["type"].as<uf::stl::string>();
				float mass = info["collision"]["mass"].as<float>();
				bool dynamic = !info["collision"]["static"].as<bool>();
				bool recenter = info["collision"]["recenter"].as<bool>();
				pod::Vector3f corner = (max - min) * 0.5f;
				pod::Vector3f center = (max + min) * 0.5f;
			#if UF_USE_BULLET
				if ( type == "mesh" && mesh ) {
					auto& collider = ext::bullet::create( entity->as<uf::Object>(), mesh, dynamic );
					if ( recenter ) collider.transform.position = center;
				} else if ( type == "bounding box" ) {
					auto& collider = ext::bullet::create( entity->as<uf::Object>(), corner, mass );
					collider.shared = true;
					if ( recenter ) collider.transform.position = center - transform.position;
				} else if ( type == "capsule" ) {
					float radius = info["collision"]["radius"].as<float>();
					float height = info["collision"]["height"].as<float>();
					auto& collider = ext::bullet::create( entity->as<uf::Object>(), radius, height, mass );
					collider.shared = true;
					if ( recenter ) collider.transform.position = center - transform.position;
				}
			#endif
			}
		}
	});
#endif

#if 0
	//UF_MSG_DEBUG( "Processing graph..." );
	// for OpenGL (immediate mode), implicitly load a lightmap so I don't have to keep commenting it out for the Vulkan version
#if UF_USE_OPENGL
	#define UF_GRAPH_DEFAULT_LIGHTMAP "./lightmap.min.png"
#else
	#define UF_GRAPH_DEFAULT_LIGHTMAP ""
#endif
	bool lightmapped = false;
	const uf::stl::string defaultLightmap = UF_GRAPH_DEFAULT_LIGHTMAP;
	// load lightmap, if requested
	if ( defaultLightmap != "" || graph.metadata["lightmap"].is<uf::stl::string>() ) {
		// check if valid filename, if not it's a texture name
		uf::stl::string f = uf::io::sanitize( graph.metadata["lightmap"].as<uf::stl::string>(defaultLightmap), uf::io::directory( graph.name ) );
		if ( (lightmapped = uf::io::exists( f )) ) {
			for ( auto& material : graph.materials ) {
				material.storage.indexLightmap = graph.textures.size();
			}
			
			auto& texture = graph.textures.emplace_back();
			texture.name = "lightmap";
			texture.bind = true;
			texture.storage.index = graph.images.size();
			texture.storage.sampler = -1;
			texture.storage.remap = graph.images.size();
			texture.storage.blend = 0;
			auto& image = graph.images.emplace_back();
		#if UF_USE_VULKAN
			bool flip = false;
		#elif UF_USE_OPENGL
			bool flip = false;
		#endif
			image.open( f, flip );

			graph.metadata["lightmap"] = texture.name;
			graph.metadata["lightmapped"] = true;

			texture.texture.loadFromImage( image );

			texture.texture.sampler.descriptor.filter.min = uf::renderer::enums::Filter::NEAREST;
			texture.texture.sampler.descriptor.filter.mag = uf::renderer::enums::Filter::NEAREST;
		}
	// invalidate our ST's if we're in OpenGL
	}
	
	// using an atlas texture will not bind other textures
	if ( graph.metadata["flags"]["ATLAS"].as<bool>() && graph.atlas.generated() ) {
		//UF_MSG_DEBUG( "Processing atlas..." );

		auto& image = *graph.images.emplace(graph.images.begin(), graph.atlas.getAtlas());
		auto& texture = *graph.textures.emplace(graph.textures.begin());
		texture.name = "atlas";
		texture.bind = true;
		texture.storage.index = 0;
		texture.storage.sampler = 0;
		texture.storage.remap = -1;
		texture.storage.blend = 0;

		if ( graph.metadata["filter"].is<uf::stl::string>() ) {
			uf::stl::string filter = graph.metadata["filter"].as<uf::stl::string>();
			if ( filter == "NEAREST" ) {
				texture.texture.sampler.descriptor.filter.min = uf::renderer::enums::Filter::NEAREST;
				texture.texture.sampler.descriptor.filter.mag = uf::renderer::enums::Filter::NEAREST;
			} else if ( filter == "LINEAR" ) {
				texture.texture.sampler.descriptor.filter.min = uf::renderer::enums::Filter::LINEAR;
				texture.texture.sampler.descriptor.filter.mag = uf::renderer::enums::Filter::LINEAR;
			}
		}
		for ( auto& material : graph.materials ) {
			if ( 0 <= material.storage.indexAlbedo ) ++material.storage.indexAlbedo;
			if ( 0 <= material.storage.indexNormal ) ++material.storage.indexNormal;
			if ( 0 <= material.storage.indexEmissive ) ++material.storage.indexEmissive;
			if ( 0 <= material.storage.indexOcclusion ) ++material.storage.indexOcclusion;
			if ( 0 <= material.storage.indexMetallicRoughness ) ++material.storage.indexMetallicRoughness;
			material.storage.indexAtlas = texture.storage.index;
			if ( 0 <= material.storage.indexLightmap ) ++material.storage.indexLightmap;
		}
		texture.texture.loadFromImage( image );

		// remap texture slots
		size_t textureSlot = 0;
		for ( auto& texture : graph.textures ) {
			texture.storage.index = -1;
			if ( !texture.bind ) continue;
			texture.storage.index = textureSlot++;
		}
	} else {
		//UF_MSG_DEBUG( "Processing textures..." );
		for ( size_t i = 0; i < graph.textures.size() && i < graph.images.size(); ++i ) {
			auto& image = graph.images[i];
			auto& texture = graph.textures[i];
			texture.bind = true;
			if ( graph.metadata["filter"].is<uf::stl::string>() ) {
				uf::stl::string filter = graph.metadata["filter"].as<uf::stl::string>();
				if ( filter == "NEAREST" ) {
					texture.texture.sampler.descriptor.filter.min = uf::renderer::enums::Filter::NEAREST;
					texture.texture.sampler.descriptor.filter.mag = uf::renderer::enums::Filter::NEAREST;
				} else if ( filter == "LINEAR" ) {
					texture.texture.sampler.descriptor.filter.min = uf::renderer::enums::Filter::LINEAR;
					texture.texture.sampler.descriptor.filter.mag = uf::renderer::enums::Filter::LINEAR;
				}
			}
			UF_MSG_DEBUG("image: " << i);
			texture.texture.loadFromImage( image );
			image.clear();
		}
	}

	//UF_MSG_DEBUG( "Processing nodes..." );
	if ( !graph.root.entity ) graph.root.entity = new uf::Object;
	for ( auto index : graph.root.children ) process( graph, index, *graph.root.entity );

	//UF_MSG_DEBUG( "Processing graphics..." );
	if ( !(graph.metadata["flags"]["SEPARATE"].as<bool>()) ) {
		initializeGraphics( graph, *graph.root.entity );

		auto& graphic = graph.root.entity->getComponent<uf::Graphic>();
		
		uf::stl::vector<pod::Matrix4f> instances;
		instances.reserve( graph.nodes.size() );
		for ( auto& node : graph.nodes ) {
			instances.emplace_back( uf::transform::model( node.transform ) );
		}
		// Models storage buffer
		graph.buffers.instance = graphic.initializeBuffer(
			(const void*) instances.data(),
			instances.size() * sizeof(pod::Matrix4f),
			uf::renderer::enums::Buffer::STORAGE
		);
		// Joints storage buffer
		if ( graph.metadata["flags"]["SKINNED"].as<bool>() ) {
			for ( auto& node : graph.nodes ) {
				if ( node.skin < 0 ) continue;
				auto& skin = graph.skins[node.skin];
				node.buffers.joint = graphic.initializeBuffer(
					(const void*) skin.inverseBindMatrices.data(),
					(1 + skin.inverseBindMatrices.size()) * sizeof(pod::Matrix4f),
					uf::renderer::enums::Buffer::STORAGE
				);
				break;
			}
		}
		// Failsafe
		if ( graph.materials.empty() ) graph.materials.emplace_back();
		if ( graph.textures.empty() ) graph.textures.emplace_back();
		
		// Materials storage buffer
		uf::stl::vector<pod::Material::Storage> materials( graph.materials.size() );
		for ( size_t i = 0; i < graph.materials.size(); ++i ) {
			materials[i] = graph.materials[i].storage;
		}
		graph.root.buffers.material = graphic.initializeBuffer(
			(const void*) materials.data(),
			materials.size() * sizeof(pod::Material::Storage),
			uf::renderer::enums::Buffer::STORAGE
		);
		// Texture storage buffer
		uf::stl::vector<pod::Texture::Storage> textures( graph.textures.size() );
		for ( size_t i = 0; i < graph.textures.size(); ++i ) {
			textures[i] = graph.textures[i].storage;
		}
		graph.root.buffers.texture = graphic.initializeBuffer(
			(const void*) textures.data(),
			textures.size() * sizeof(pod::Texture::Storage),
			uf::renderer::enums::Buffer::STORAGE
		);
	}

	// calculate extents
	pod::Vector3f extentMin = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
	pod::Vector3f extentMax = { -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max() };

	//UF_MSG_DEBUG( "Post-processing nodes..." );
	graph.root.entity->process([&]( uf::Entity* entity ) {
		if ( !entity->hasComponent<uf::Mesh>() ) return;
		//UF_MSG_DEBUG( "Post-processing node...: " << uf::string::toString( entity->as<uf::Object>() ) );
		
		auto& metadata = entity->getComponent<uf::Serializer>();
		uf::stl::string nodeName = metadata["system"]["graph"]["name"].as<uf::stl::string>( entity->getName() );
	
		auto& transform = entity->getComponent<pod::Transform<>>();
	#if UF_GRAPH_VARYING_MESH
		auto& mesh = entity->getComponent<uf::Mesh>().get();
		mesh.updateDescriptor();
	#else
		auto& mesh = entity->getComponent<uf::Mesh>();
	#endif

		pod::Vector3f min = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
		pod::Vector3f max = { -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max() };

		auto model = uf::transform::model( transform );

	#if UF_GRAPH_VARYING_MESH
		size_t indices = mesh.attributes.index.length;
		size_t indexStride = mesh.attributes.index.size;
		uint8_t* indexPointer = (uint8_t*) mesh.attributes.index.pointer;

		size_t vertices = mesh.attributes.vertex.length;
		size_t vertexStride = mesh.attributes.vertex.size;
		uint8_t* vertexPointer = (uint8_t*) mesh.attributes.vertex.pointer;

		uf::renderer::AttributeDescriptor vertexAttributePosition;
		uf::renderer::AttributeDescriptor vertexAttributeNormal;

		for ( auto& attribute : mesh.attributes.vertex.descriptor ) {
			if ( attribute.name == "position" ) vertexAttributePosition = attribute;
			else if ( attribute.name == "normal" ) vertexAttributeNormal = attribute;
		}
		UF_ASSERT( vertexAttributePosition.name != "" );
		
		for ( size_t currentIndex = 0; currentIndex < vertices; ++currentIndex ) {
			uint8_t* vertexSrc = vertexPointer + (currentIndex * vertexStride);
			const pod::Vector3f& position = *((pod::Vector3f*) (vertexSrc + vertexAttributePosition.offset));	
			min.x = std::min( min.x, position.x );
			min.y = std::min( min.y, position.y );
			min.z = std::min( min.z, position.z );

			max.x = std::max( max.x, position.x );
			max.y = std::max( max.y, position.y );
			max.z = std::max( max.z, position.z );
		}
	#else
		for ( auto& vertex : mesh.vertices ) {
		//	auto position = uf::matrix::multiply<float>( model, vertex.position, 1.0f );
			min.x = std::min( min.x, vertex.position.x );
			min.y = std::min( min.y, vertex.position.y );
			min.z = std::min( min.z, vertex.position.z );

			max.x = std::max( max.x, vertex.position.x );
			max.y = std::max( max.y, vertex.position.y );
			max.z = std::max( max.z, vertex.position.z );
		}
	#endif

		min = uf::matrix::multiply<float>( model, min, 1.0f );
		max = uf::matrix::multiply<float>( model, max, 1.0f );

		extentMin.x = std::min( min.x, extentMin.x );
		extentMin.y = std::min( min.y, extentMin.y );
		extentMin.z = std::min( min.z, extentMin.z );

		extentMax.x = std::max( max.x, extentMax.x );
		extentMax.y = std::max( max.y, extentMax.y );
		extentMax.z = std::max( max.z, extentMax.z );

		if ( graph.metadata["flags"]["NORMALS"].as<bool>() ) {
			// bool invert = false;
			bool INVERTED = graph.metadata["flags"]["INVERT"].as<bool>();
		#if UF_GRAPH_VARYING_MESH
			if ( mesh.attributes.index.pointer ) {
				uint32_t indexes[3] = {};
				pod::Vector3f positions[3] = {};
				pod::Vector3f normal = {};

				for ( size_t currentIndex = 0; currentIndex < indices; currentIndex += 3 ) {
					for ( auto i = 0; i < 3; ++i ) {
						uint8_t* indexSrc = indexPointer + ((currentIndex + i) * indexStride);
						switch ( indexStride ) {
							case sizeof( uint8_t): indexes[i] = *(( uint8_t*) indexSrc); break;
							case sizeof(uint16_t): indexes[i] = *((uint16_t*) indexSrc); break;
							case sizeof(uint32_t): indexes[i] = *((uint32_t*) indexSrc); break;
						}
						
						uint8_t* vertexSrc = vertexPointer + (indexes[i] * vertexStride);
						positions[i] = *((pod::Vector3f*) (vertexSrc + vertexAttributePosition.offset));
					}

				//	if ( INVERTED ) std::swap( positions[1], positions[2] );
				//	normal = uf::vector::normalize( uf::vector::cross( positions[2] - positions[0], positions[1] - positions[0] ) );
					normal = uf::vector::normalize( uf::vector::cross( positions[INVERTED ? 1 : 2] - positions[INVERTED ? 0 : 0], positions[INVERTED ? 2 : 1] - positions[INVERTED ? 0 : 0] ) );
				//	if ( INVERTED ) normal *= -1;

					for ( auto i = 0; i < 3; ++i ) {
						uint8_t* vertexSrc = vertexPointer + (indexes[i] * vertexStride);
						*((pod::Vector3f*) (vertexSrc + vertexAttributeNormal.offset)) = normal;
					}
				}
			} else {
				pod::Vector3f positions[3] = {};
				pod::Vector3f normal = {};
				for ( size_t currentIndex = 0; currentIndex < vertices; currentIndex+=3 ) {
					for ( auto i = 0; i < 3; ++i ) {
						uint8_t* vertexSrc = vertexPointer + ((currentIndex + i) * vertexStride);
						positions[i] = *((pod::Vector3f*) (vertexSrc + vertexAttributePosition.offset));
					}

				//	if ( INVERTED ) std::swap( positions[1], positions[2] );
				//	normal = uf::vector::normalize( uf::vector::cross( positions[2] - positions[0], positions[1] - positions[0] ) );
					normal = uf::vector::normalize( uf::vector::cross( positions[INVERTED ? 1 : 2] - positions[INVERTED ? 0 : 0], positions[INVERTED ? 2 : 1] - positions[INVERTED ? 0 : 0] ) );
				//	if ( INVERTED ) normal *= -1;

					for ( auto i = 0; i < 3; ++i ) {
						uint8_t* vertexSrc = vertexPointer + ((currentIndex + i) * vertexStride);
						*((pod::Vector3f*) (vertexSrc + vertexAttributeNormal.offset)) = normal;
					}
				}
			}
		#else
			if ( mesh.indices.empty() ) {
				for ( size_t i = 0; i < mesh.vertices.size(); i+=3 ) {
					auto& a = mesh.vertices[i+(INVERTED ? 0 : 0)].position;
					auto& b = mesh.vertices[i+(INVERTED ? 1 : 2)].position;
					auto& c = mesh.vertices[i+(INVERTED ? 2 : 1)].position;

					pod::Vector3f normal = uf::vector::normalize( uf::vector::cross( b - a, c - a ) );

					mesh.vertices[i+0].normal = normal;
					mesh.vertices[i+1].normal = normal;
					mesh.vertices[i+2].normal = normal;
				}
			} else {
				for ( size_t i = 0; i < mesh.indices.size(); i+=3 ) {
					auto& A = mesh.vertices[mesh.indices[i+(INVERTED ? 0 : 0)]];
					auto& B = mesh.vertices[mesh.indices[i+(INVERTED ? 1 : 2)]];
					auto& C = mesh.vertices[mesh.indices[i+(INVERTED ? 2 : 1)]];

					auto& a = A.position;
					auto& b = B.position;
					auto& c = C.position;

					pod::Vector3f normal = uf::vector::normalize( uf::vector::cross( b - a, c - a ) );
					
					A.normal = normal;
					B.normal = normal;
					C.normal = normal;
				}
			}
		#endif
		}
		if ( entity->hasComponent<uf::Graphic>() ) {
			auto& graphic = entity->getComponent<uf::Graphic>();
			graphic.initialize();
		#if 1
		#if UF_GRAPH_VARYING_MESH
			graphic.initializeAttributes( mesh.attributes );
		#else
			graphic.initializeMesh( mesh );
			graphic.process = true;
		#endif
		#else
		#if 1
			if ( entity->getName() == "worldspawn_20" && ext::json::isObject( graph.metadata["grid"] ) ) {
				auto sliced = mesh;
				auto& grid = entity->getComponent<uf::MeshGrid>();
				grid.initialize( sliced, uf::vector::decode( graph.metadata["grid"]["size"], pod::Vector3ui{1,1,1} ) );
				grid.analyze();

				auto center = uf::vector::decode( graph.metadata["grid"]["center"], grid.center() );
				sliced.indices = grid.get<>( center );
				graphic.initializeMesh( sliced );
			} else {
				graphic.initializeMesh( mesh );
			}
		#elif 0
			graphic.initialize();
			graphic.initializeMesh( mesh, 0 );
			if ( ext::json::isObject( graph.metadata["grid"] ) ) {
				auto& grid = entity->getComponent<uf::MeshGrid>();
				grid.initialize( mesh, uf::vector::decode( graph.metadata["grid"]["size"], pod::Vector3ui{1,1,1} ) );
				grid.analyze();

				auto center = uf::vector::decode( graph.metadata["grid"]["center"], grid.center() );
				auto indices = grid.get<>( center );
				graphic.descriptor.indices = indices.size();
				UF_MSG_DEBUG("Center: " << uf::vector::toString(center) << " | Indices: " << indices.size());
				int32_t indexBuffer = -1;
				for ( size_t i = 0; i < graphic.buffers.size(); ++i ) {
					if ( graphic.buffers[i].usage & uf::renderer::enums::Buffer::INDEX ) indexBuffer = i;
				}
				if ( indexBuffer >= 0 && !indices.empty() && indices.size() % 3 == 0 ) {
					shader.updateBuffer(
						(const void*) indices.data(),
						indices.size() * mesh.attributes.index.size,
						indexBuffer
					);
				}
			}
		#endif
		#endif
		}
		if ( entity->hasComponent<uf::Mesh>() ) {
			auto& mesh = entity->getComponent<uf::Mesh>();
			auto& meshAttributes = entity->getComponent<pod::Mesh>();
			meshAttributes = mesh;
		}
		
		if ( !ext::json::isNull( graph.metadata["tags"][nodeName] ) ) {
			auto& info = graph.metadata["tags"][nodeName];
			if ( ext::json::isObject( info["collision"] ) ) {
				uf::stl::string type = info["collision"]["type"].as<uf::stl::string>();
				float mass = info["collision"]["mass"].as<float>();
				bool dynamic = !info["collision"]["static"].as<bool>();
				bool recenter = info["collision"]["recenter"].as<bool>();
				pod::Vector3f corner = (max - min) * 0.5f;
				pod::Vector3f center = (max + min) * 0.5f;
			#if UF_USE_BULLET
				if ( type == "mesh" ) {
					auto& collider = ext::bullet::create( entity->as<uf::Object>(), mesh, dynamic );
					if ( recenter ) collider.transform.position = center;
				} else if ( type == "bounding box" ) {
					auto& collider = ext::bullet::create( entity->as<uf::Object>(), corner, mass );
					collider.shared = true;
					if ( recenter ) collider.transform.position = center - transform.position;
				} else if ( type == "capsule" ) {
					float radius = info["collision"]["radius"].as<float>();
					float height = info["collision"]["height"].as<float>();
					auto& collider = ext::bullet::create( entity->as<uf::Object>(), radius, height, mass );
					collider.shared = true;
					if ( recenter ) collider.transform.position = center - transform.position;
				}
			#endif
			}
		}

		//UF_MSG_DEBUG( "Post-processed node...: " << uf::string::toString( entity->as<uf::Object>() ) );
	});

	graph.metadata["extents"]["min"] = uf::vector::encode( extentMin * graph.metadata["extents"]["scale"].as<float>(1.0f) );
	graph.metadata["extents"]["max"] = uf::vector::encode( extentMax * graph.metadata["extents"]["scale"].as<float>(1.0f) );
	//UF_MSG_DEBUG( "Processed graph" );
#endif
}
void uf::graph::process( pod::Graph& graph, int32_t index, uf::Object& parent ) {
	auto& node = graph.nodes[index];
	// ignore pesky light_Orientation nodes
	if ( uf::string::split( node.name, "_" ).back() == "Orientation" ) return;
	// for dreamcast, ignore lights if we're baked
#if UF_USE_OPENGL
	if ( graph.metadata["lightmapped"].as<bool>() ) {
		if ( graph.lights.count(node.name) > 0 ) return;
	}
#endif
	//UF_MSG_DEBUG( "Loading " << node.name );
	// create child if requested
	uf::Object* pointer = new uf::Object;
	parent.addChild(*pointer);

	uf::Object& entity = *pointer;
	node.entity = &entity;
	
	bool setName = entity.getName() == "Entity";
	auto& metadata = entity.getComponent<uf::ObjectBehavior::Metadata>();
	auto& metadataJson = entity.getComponent<uf::Serializer>();
	metadataJson["system"]["graph"]["name"] = node.name;
	metadataJson["system"]["graph"]["index"] = index;
	// on systems where frametime is very, very important, we can set all static nodes to not tick
	// tie to tag
	if ( !ext::json::isNull( graph.metadata["tags"][node.name] ) ) {
		auto& info = graph.metadata["tags"][node.name];
		if ( info["ignore"].as<bool>() ) {
			return;
		}
		if ( info["action"].as<uf::stl::string>() == "load" ) {
			if ( info["filename"].is<uf::stl::string>() ) {
				uf::stl::string filename = uf::io::resolveURI( info["filename"].as<uf::stl::string>(), graph.metadata["root"].as<uf::stl::string>() );
				entity.load(filename);
			} else if ( ext::json::isObject( info["payload"] ) ) {
				uf::Serializer json = info["payload"];
				json["root"] = graph.metadata["root"];
				entity.load(json);
			}
		} else if ( info["action"].as<uf::stl::string>() == "attach" ) {
			uf::stl::string filename = uf::io::resolveURI( info["filename"].as<uf::stl::string>(), graph.metadata["root"].as<uf::stl::string>() );
			auto& child = entity.loadChild( filename, false );
			auto& childTransform = child.getComponent<pod::Transform<>>();
			auto flatten = uf::transform::flatten( node.transform );
			if ( !info["preserve position"].as<bool>() ) childTransform.position = flatten.position;
			if ( !info["preserve orientation"].as<bool>() ) childTransform.orientation = flatten.orientation;
		}
		if ( info["static"].is<bool>() ) {
			metadata.system.ignoreGraph = info["static"].as<bool>();
		}
	}
	// create as light
	{
		if ( graph.lights.count(node.name) > 0 ) {
			auto& l = graph.lights[node.name];
		#if UF_ENV_DREAMCAST
			metadata.system.ignoreGraph = graph.metadata["debug"]["static"].as<bool>();
		#endif
			uf::Serializer metadataLight;
			metadataLight["radius"][0] = 0.001;
			metadataLight["radius"][1] = l.range; // l.range <= 0.001f ? graph.metadata["lights"]["range cap"].as<float>() : l.range;
			metadataLight["power"] = l.intensity; //  * graph.metadata["lights"]["power scale"].as<float>();
			metadataLight["color"][0] = l.color.x;
			metadataLight["color"][1] = l.color.y;
			metadataLight["color"][2] = l.color.z;
			metadataLight["shadows"] = graph.metadata["lights"]["shadows"].as<bool>();
			if ( metadataLight["shadows"].as<bool>() ) {
				metadataLight["radius"][1] = 0;
			}
			if ( ext::json::isArray( graph.metadata["lights"]["radius"] ) ) {
				metadataLight["radius"] = graph.metadata["lights"]["radius"];
			}
			if ( graph.metadata["lights"]["bias"].is<float>() ) {
				metadataLight["bias"] = graph.metadata["lights"]["bias"].as<float>();
			}
			// copy from tag information
			ext::json::forEach( graph.metadata["tags"][node.name]["light"], [&]( const uf::stl::string& key, ext::json::Value& value ){
				if ( key == "transform" ) return;
				metadataLight[key] = value;
			});
			if ( !(graph.metadata["lightmapped"].as<bool>() && !(metadataLight["shadows"].as<bool>() || metadataLight["dynamic"].as<bool>())) ) {
				auto& metadataJson = entity.getComponent<uf::Serializer>();
				entity.load("/light.json");
				// copy
				ext::json::forEach( metadataLight, [&]( const uf::stl::string& key, ext::json::Value& value ) {
					metadataJson["light"][key] = value;
				});
			}
		}
	}

	// set name
	if ( setName ) entity.setName( node.name );

	// reference transform to parent
	{
		auto& transform = entity.getComponent<pod::Transform<>>();
		transform = node.transform;
		transform.reference = &parent.getComponent<pod::Transform<>>();
		// override transform
		if ( !ext::json::isNull( graph.metadata["tags"][node.name] ) ) {
			auto& info = graph.metadata["tags"][node.name];
			if ( info["transform"]["offset"].as<bool>() ) {
				auto parsed = uf::transform::decode( info["transform"], pod::Transform<>{} );
				transform.position += parsed.position;
				transform.orientation = uf::quaternion::multiply( transform.orientation, parsed.orientation );
			} else {
				transform = uf::transform::decode( info["transform"], transform );
				if ( info["transform"]["parent"].is<uf::stl::string>() ) {
					auto* parentPointer = uf::graph::find( graph, info["transform"]["parent"].as<uf::stl::string>() );
					if ( parentPointer ) {
						auto& parentNode = *parentPointer;
						// entity already exists, bind to its transform
						if ( parentNode.entity && parentNode.entity->hasComponent<pod::Transform<>>() ) {
							auto& parentTransform = parentNode.entity->getComponent<pod::Transform<>>();
							transform = uf::transform::reference( transform, parentTransform, info["transform"]["reorient"].as<bool>() );
							transform.position = -transform.position;
						// doesnt exist, bind to the node transform
						} else {
							transform = uf::transform::reference( transform, parentNode.transform, info["transform"]["reorient"].as<bool>() );
						}
					}
				}
			}
		}
	}
	if ( 0 <= node.mesh && node.mesh < graph.meshes.size() ) {
		auto& nodeMesh = graph.meshes[node.mesh];
		auto& mesh = entity.getComponent<uf::Mesh>();

	}

#if 0
	// copy mesh
	if ( 0 <= node.mesh && node.mesh < graph.meshes.size() ) {
		auto& nodeMesh = graph.meshes[node.mesh];
		auto& mesh = entity.getComponent<uf::Mesh>();
		// preprocess mesh
	#if UF_GRAPH_VARYING_MESH
		{
			size_t vertices = nodeMesh.attributes.vertex.length;
			size_t vertexStride = nodeMesh.attributes.vertex.size;
			uint8_t* vertexPointer = (uint8_t*) nodeMesh.attributes.vertex.pointer;

			uf::renderer::AttributeDescriptor vertexAttributeId;

			for ( auto& attribute : nodeMesh.attributes.vertex.descriptor ) if ( attribute.name == "id" ) vertexAttributeId = attribute;
			UF_ASSERT( vertexAttributeId.name != "" );
			
			for ( size_t currentIndex = 0; currentIndex < vertices; ++currentIndex ) {
				uint8_t* vertexSrc = vertexPointer + (currentIndex * vertexStride);
				pod::Vector<uf::graph::mesh::id_t,2>& id = *((pod::Vector<uf::graph::mesh::id_t,2>*) (vertexSrc + vertexAttributeId.offset));	
				id.x = node.index;
			}
		}
		
		if ( nodeMesh.is<uf::graph::mesh::Base>() ) mesh.insert<uf::graph::mesh::Base>( nodeMesh );
		else if ( nodeMesh.is<uf::graph::mesh::ID>() ) mesh.insert<uf::graph::mesh::ID>( nodeMesh );
		else if ( nodeMesh.is<uf::graph::mesh::Skinned>() ) mesh.insert<uf::graph::mesh::Skinned>( nodeMesh );
		else UF_EXCEPTION("unrecognized mesh type encountered...");
	#else
		for ( auto& vertex : nodeMesh.vertices ) vertex.id.x = node.index;
		mesh.insert( nodeMesh );
	#endif

		if ( !(graph.metadata["flags"]["SEPARATE"].as<bool>()) ) {
			auto& mesh = graph.root.entity->getComponent<uf::Mesh>();
		#if UF_GRAPH_VARYING_MESH
			if ( nodeMesh.is<uf::graph::mesh::Base>() ) mesh.insert<uf::graph::mesh::Base>( nodeMesh );
			else if ( nodeMesh.is<uf::graph::mesh::ID>() ) mesh.insert<uf::graph::mesh::ID>( nodeMesh );
			else if ( nodeMesh.is<uf::graph::mesh::Skinned>() ) mesh.insert<uf::graph::mesh::Skinned>( nodeMesh );
			else UF_EXCEPTION("unrecognized mesh type encountered...");
		#else
			mesh.insert( nodeMesh );
		#endif
		} else if ( graph.metadata["flags"]["RENDER"].as<bool>() ) {
			uf::instantiator::bind("RenderBehavior", entity);
			initializeGraphics( graph, entity );
			auto& graphic = entity.getComponent<uf::Graphic>();
			// Joints storage buffer
			if ( graph.metadata["flags"]["SKINNED"].as<bool>() && node.skin >= 0 ) {
				auto& skin = graph.skins[node.skin];
				node.buffers.joint = graphic.initializeBuffer(
					(const void*) skin.inverseBindMatrices.data(),
					skin.inverseBindMatrices.size() * sizeof(pod::Matrix4f),
					uf::renderer::enums::Buffer::STORAGE
				);
			}
			// Materials storage buffer
			uf::stl::vector<pod::Material::Storage> materials( graph.materials.size() );
			for ( size_t i = 0; i < graph.materials.size(); ++i ) {
				materials[i] = graph.materials[i].storage;
			}
			node.buffers.material = graphic.initializeBuffer(
				(const void*) materials.data(),
				materials.size() * sizeof(pod::Material::Storage),
				uf::renderer::enums::Buffer::STORAGE
			);
			// Texture storage buffer
			uf::stl::vector<pod::Texture::Storage> textures( graph.textures.size() );
			for ( size_t i = 0; i < graph.textures.size(); ++i ) {
				textures[i] = graph.textures[i].storage;
			//	textures[i].remap = -1;
			}
			graph.root.buffers.texture = graphic.initializeBuffer(
				(const void*) textures.data(),
				textures.size() * sizeof(pod::Texture::Storage),
				uf::renderer::enums::Buffer::STORAGE
			);
		}
	}
#endif
	//UF_MSG_DEBUG( "Loaded " << node.name );
	for ( auto index : node.children ) uf::graph::process( graph, index, entity );
}
void uf::graph::cleanup( pod::Graph& graph ) {
//	for ( auto& m : graph.meshes ) m.destroy();

//	graph.images.clear();
//	graph.meshes.clear();
//	graph.atlas.clear();
}
void uf::graph::override( pod::Graph& graph ) {
	graph.settings.animations.override.a = 0;
	graph.settings.animations.override.map.clear();
	bool toNeutralPose = graph.sequence.empty();
	// store every node's current transform
	for ( auto& node : graph.nodes ) {
		graph.settings.animations.override.map[node.index].first = node.transform;
		graph.settings.animations.override.map[node.index].second = node.transform;
		if ( toNeutralPose ) {
			graph.settings.animations.override.map[node.index].second.position = { 0, 0, 0 };
			graph.settings.animations.override.map[node.index].second.orientation = { 0, 0, 0, 1 };
			graph.settings.animations.override.map[node.index].second.scale = { 1, 1, 1 };
		}
	}
	// set our destination transform per node
	if ( !toNeutralPose ) {
		uf::stl::string name = graph.sequence.front();
		pod::Animation& animation = graph.animations[name];
		for ( auto& channel : animation.channels ) {
			auto& override = graph.settings.animations.override.map[channel.node];
			auto& sampler = animation.samplers[channel.sampler];
			if ( sampler.interpolator != "LINEAR" ) continue;
			for ( size_t i = 0; i < sampler.inputs.size() - 1; ++i ) {
				if ( !(animation.start >= sampler.inputs[i] && animation.start <= sampler.inputs[i+1]) ) continue;
				if ( channel.path == "translation" ) {
					override.second.position = sampler.outputs[i];
				} else if ( channel.path == "rotation" ) {
					override.second.orientation = uf::quaternion::normalize( sampler.outputs[i] );
				} else if ( channel.path == "scale" ) {
					override.second.scale = sampler.outputs[i];
				}
			}
		}
	}
}

void uf::graph::initialize( pod::Graph& graph ) {
	graph.root.entity->initialize();
	graph.root.entity->process([&]( uf::Entity* entity ) {
	#if 0
		//UF_MSG_DEBUG( "Initializing... " << uf::string::toString( entity->as<uf::Object>() ) );
		if ( !entity->hasComponent<uf::Graphic>() ) {
			if ( entity->getUid() == 0 ) entity->initialize();
			//UF_MSG_DEBUG( "Initialized " << uf::string::toString( entity->as<uf::Object>() ) );
			return;
		}
		initializeShaders( graph, entity->as<uf::Object>() );

		uf::instantiator::bind( "GraphBehavior", *entity );
		uf::instantiator::unbind( "RenderBehavior", *entity );
	#endif
		if ( entity->getUid() == 0 ) entity->initialize();
		//UF_MSG_DEBUG( "Initialized " << uf::string::toString( entity->as<uf::Object>() ) );
	});
}

void uf::graph::animate( pod::Graph& graph, const uf::stl::string& name, float speed, bool immediate ) {
	if ( !(graph.metadata["flags"]["SKINNED"].as<bool>()) ) return;
	if ( graph.animations.count( name ) > 0 ) {
		// if already playing, ignore it
		if ( !graph.sequence.empty() && graph.sequence.front() == name ) return;
		if ( immediate ) {
			while ( !graph.sequence.empty() ) graph.sequence.pop();
		}
		bool empty = graph.sequence.empty();
		graph.sequence.emplace(name);
		if ( empty ) uf::graph::override( graph );
		graph.settings.animations.speed = speed;
	}
	update( graph, 0 );
}
void uf::graph::update( pod::Graph& graph ) {
	return update( graph, uf::physics::time::delta );
}
void uf::graph::update( pod::Graph& graph, float delta ) {
	// no override
	if ( !(graph.metadata["flags"]["SKINNED"].as<bool>()) ) return;
	if ( graph.sequence.empty() ) goto UPDATE;
	if ( graph.settings.animations.override.a >= 0 ) goto OVERRIDE;
	{
		uf::stl::string name = graph.sequence.front();
		pod::Animation* animation = &graph.animations[name];
	//	std::cout << "ANIMATION: " << name << "\t" << animation->cur << std::endl;
		animation->cur += delta * graph.settings.animations.speed; // * graph.settings.animations.override.speed;
		if ( animation->end < animation->cur ) {
			animation->cur = graph.settings.animations.loop ? animation->cur - animation->end : 0;
			// go-to next animation
			if ( !graph.settings.animations.loop ) {
				graph.sequence.pop();
				// out of animations, set to neutral pose
				if ( graph.sequence.empty() ) {
					uf::graph::override( graph );
					goto OVERRIDE;
				}
				name = graph.sequence.front();
				animation = &graph.animations[name];
			}
		}
		for ( auto& channel : animation->channels ) {
			auto& sampler = animation->samplers[channel.sampler];
			if ( sampler.interpolator != "LINEAR" ) continue;
			for ( size_t i = 0; i < sampler.inputs.size() - 1; ++i ) {
				if ( !(animation->cur >= sampler.inputs[i] && animation->cur <= sampler.inputs[i+1]) ) continue;
				float a = (animation->cur - sampler.inputs[i]) / (sampler.inputs[i+1] - sampler.inputs[i]);
				auto& transform = graph.nodes[channel.node].transform;
				if ( channel.path == "translation" ) {
					transform.position = uf::vector::mix( sampler.outputs[i], sampler.outputs[i+1], a );
				} else if ( channel.path == "rotation" ) {
					transform.orientation = uf::quaternion::normalize( uf::quaternion::slerp(sampler.outputs[i], sampler.outputs[i+1], a) );
				} else if ( channel.path == "scale" ) {
					transform.scale = uf::vector::mix( sampler.outputs[i], sampler.outputs[i+1], a );
				}
			}
		}
		goto UPDATE;
	}
OVERRIDE:
	// std::cout << "OVERRIDED: " << graph.settings.animations.override.a << "\t" << -std::numeric_limits<float>::max() << std::endl;
	for ( auto pair : graph.settings.animations.override.map ) {
		graph.nodes[pair.first].transform.position = uf::vector::mix( pair.second.first.position, pair.second.second.position, graph.settings.animations.override.a );
		graph.nodes[pair.first].transform.orientation = uf::quaternion::normalize( uf::quaternion::slerp(pair.second.first.orientation, pair.second.second.orientation, graph.settings.animations.override.a) );
		graph.nodes[pair.first].transform.scale = uf::vector::mix( pair.second.first.scale, pair.second.second.scale, graph.settings.animations.override.a );
	}
	// finished our overrided interpolation, clear it
	if ( (graph.settings.animations.override.a += delta * graph.settings.animations.override.speed) >= 1 ) {
		graph.settings.animations.override.a = -std::numeric_limits<float>::max();
		graph.settings.animations.override.map.clear();
	}
UPDATE:
	for ( auto& node : graph.nodes ) uf::graph::update( graph, node );
}
void uf::graph::update( pod::Graph& graph, pod::Node& node ) {
#if 0
	if ( node.skin >= 0 ) {
		pod::Matrix4f nodeMatrix = uf::graph::matrix( graph, node.index );
		pod::Matrix4f inverseTransform = uf::matrix::inverse( nodeMatrix );
		auto& skin = graph.skins[node.skin];
		uf::stl::vector<pod::Matrix4f> joints;
		joints.reserve( skin.joints.size() );
		for ( size_t i = 0; i < skin.joints.size(); ++i ) {
			joints.emplace_back( uf::matrix::identity() );
		}
		if ( graph.settings.animations.override.a >= 0 || !graph.sequence.empty() ) {
			for ( size_t i = 0; i < skin.joints.size(); ++i ) {
				joints[i] = inverseTransform * (uf::graph::matrix(graph, skin.joints[i]) * skin.inverseBindMatrices[i]);
			}
		}
		if ( node.entity && node.entity->hasComponent<uf::Graphic>() ) {
			auto& graphic = node.entity->getComponent<uf::Graphic>();
			if ( node.buffers.joint < graphic.buffers.size() ) {
				auto& buffer = graphic.buffers.at(node.buffers.joint);
				shader.updateBuffer( (const void*) joints.data(), joints.size() * sizeof(pod::Matrix4f), buffer );
			}
		}
	}
#endif
}
void uf::graph::destroy( pod::Graph& graph ) {
#if 0
	for ( auto& t : graph.textures ) {
		t.texture.destroy();
	}
	for ( auto& m : graph.meshes ) {
		m.destroy();
	}
#endif
}

namespace {
	uf::stl::string decodeImage( ext::json::Value& json, const pod::Graph& graph ) {
		if ( json.is<uf::stl::string>() ) {
			const uf::stl::string directory = uf::io::directory( graph.name );
			const uf::stl::string filename = uf::io::filename( json.as<uf::stl::string>() );
			const uf::stl::string name = directory + "/" + filename;

			uf::Image& image = uf::graph::storage.images[name];
			image.open(name, false);
			return name;
		}
		auto name = json["name"].as<uf::stl::string>();
		
		uf::Image& image = uf::graph::storage.images[name];
		auto size = uf::vector::decode( json["size"], pod::Vector2ui{} );
		size_t bpp = json["bpp"].as<size_t>();
		size_t channels = json["channels"].as<size_t>();
		auto pixels = uf::base64::decode( json["data"].as<uf::stl::string>() );
		image.loadFromBuffer( &pixels[0], size, bpp, channels, true );
		
		return name;
	}

	uf::stl::string decodeTexture( ext::json::Value& json, const pod::Graph& graph ) {
		auto name = json["name"].as<uf::stl::string>();

		pod::Texture& texture = uf::graph::storage.textures[name];
		texture.index = json["index"].as(texture.index);
		texture.sampler = json["sampler"].as(texture.sampler);
		texture.remap = json["remap"].as(texture.remap);
		texture.blend = json["blend"].as(texture.blend);
		texture.lerp = uf::vector::decode( json["lerp"], pod::Vector4f{} );
		
		return name;
	}

	uf::stl::string decodeSampler( ext::json::Value& json, const pod::Graph& graph ) {
		auto name = json["name"].as<uf::stl::string>();

		uf::renderer::Sampler& sampler = uf::graph::storage.samplers[name];
		sampler.descriptor.filter.min = (uf::renderer::enums::Filter::type_t) json["min"].as<size_t>();
		sampler.descriptor.filter.mag = (uf::renderer::enums::Filter::type_t) json["mag"].as<size_t>();
		sampler.descriptor.addressMode.u = (uf::renderer::enums::AddressMode::type_t) json["u"].as<size_t>();
		sampler.descriptor.addressMode.v = (uf::renderer::enums::AddressMode::type_t) json["v"].as<size_t>();
		sampler.descriptor.addressMode.w = sampler.descriptor.addressMode.v;
		
		return name;
	}

	uf::stl::string decodeMaterial( ext::json::Value& json, const pod::Graph& graph ) {
		auto name = json["name"].as<uf::stl::string>();

		pod::Material& material = uf::graph::storage.materials[name];
		material.colorBase = uf::vector::decode( json["base"], material.colorBase );
		material.colorEmissive = uf::vector::decode( json["emissive"], material.colorEmissive );
		material.factorMetallic = json["fMetallic"].as(material.factorMetallic);
		material.factorRoughness = json["fRoughness"].as(material.factorRoughness);
		material.factorOcclusion = json["fOcclusion"].as(material.factorOcclusion);
		material.factorAlphaCutoff = json["fAlphaCutoff"].as(material.factorAlphaCutoff);
		material.indexAlbedo = json["iAlbedo"].as(material.indexAlbedo);
		material.indexNormal = json["iNormal"].as(material.indexNormal);
		material.indexEmissive = json["iEmissive"].as(material.indexEmissive);
		material.indexOcclusion = json["iOcclusion"].as(material.indexOcclusion);
		material.indexMetallicRoughness = json["iMetallicRoughness"].as(material.indexMetallicRoughness);
		material.modeAlpha = json["modeAlpha"].as(material.modeAlpha);
		
		return name;
	}

	pod::Light decodeLight( ext::json::Value& json, const pod::Graph& graph ) {
		pod::Light light;
		light.color = uf::vector::decode( json["color"], light.color );
		light.intensity = json["intensity"].as(light.intensity);
		light.range = json["range"].as(light.range);
		return light;
	}

	pod::Animation decodeAnimation( ext::json::Value& json, const pod::Graph& graph ) {
		pod::Animation animation;
		animation.name = json["name"].as(animation.name);
		animation.start = json["start"].as(animation.start);
		animation.end = json["end"].as(animation.end);

		ext::json::forEach( json["samplers"], [&]( ext::json::Value& value ){
			auto& sampler = animation.samplers.emplace_back();
			sampler.interpolator = value["interpolator"].as(sampler.interpolator);

			sampler.inputs.reserve( value["inputs"].size() );
			ext::json::forEach( value["inputs"], [&]( ext::json::Value& input ){
				sampler.inputs.emplace_back( input.as<float>() );
			});
			
			sampler.outputs.reserve( value["outputs"].size() );
			ext::json::forEach( value["outputs"], [&]( ext::json::Value& output ){
				sampler.outputs.emplace_back( uf::vector::decode( output, pod::Vector4f{} ) );
			});
		});

		ext::json::forEach( json["channels"], [&]( ext::json::Value& value ){
			auto& channel = animation.channels.emplace_back();
			channel.path = value["path"].as(channel.path);
			channel.node = value["node"].as(channel.node);
			channel.sampler = value["sampler"].as(channel.sampler);
		});
		return animation;
	}

	pod::Skin decodeSkin( ext::json::Value& json, const pod::Graph& graph ) {
		pod::Skin skin;
		skin.name = json["name"].as(skin.name);
		
		skin.joints.reserve( json["joints"].size() );
		ext::json::forEach( json["joints"], [&]( ext::json::Value& value ){
			skin.joints.emplace_back( value.as<int32_t>() );
		});
		
		skin.inverseBindMatrices.reserve( json["inverseBindMatrices"].size() );
		ext::json::forEach( json["inverseBindMatrices"], [&]( ext::json::Value& value ){
			skin.inverseBindMatrices.emplace_back( uf::matrix::decode( value, pod::Matrix4f{} ) );
		});
		
		return skin;
	}

	uf::stl::vector<pod::DrawCommand> decodeDrawCommand( ext::json::Value& json, const pod::Graph& graph ) {
		uf::stl::vector<pod::DrawCommand> drawCommands;
		drawCommands.reserve( json.size() );
		ext::json::forEach( json, [&]( ext::json::Value& value ){
			auto& drawCommand = drawCommands.emplace_back();
			drawCommand.indices = value["indices"].as(drawCommand.indices);
			drawCommand.instances = value["instances"].as(drawCommand.instances);
			drawCommand.indexID = value["indexID"].as(drawCommand.indexID);
			drawCommand.vertexID = value["vertexID"].as(drawCommand.vertexID);

			drawCommand.instanceID = value["instanceID"].as(drawCommand.instanceID);
			drawCommand.materialID = value["materialID"].as(drawCommand.materialID);
			drawCommand.objectID = value["objectID"].as(drawCommand.objectID);
			drawCommand.vertices = value["vertices"].as(drawCommand.vertices);
		});
		return drawCommands;
	}

	pod::Instance decodeInstance( ext::json::Value& json, const pod::Graph& graph ) {
		pod::Instance instance;

		return instance;
	}

	uf::stl::string decodeMesh( ext::json::Value& json, const pod::Graph& graph ) {
		auto name = json["name"].as<uf::stl::string>();
		auto& pair = uf::graph::storage.meshes[name];
		pair.drawCommands = decodeDrawCommand( json["drawCommands"], graph );

		auto& mesh = pair.mesh;
		mesh.vertex.attributes.reserve( json["inputs"]["vertex"]["attributes"].size() );
		ext::json::forEach( json["inputs"]["vertex"]["attributes"], [&]( ext::json::Value& value ){
			auto& attribute = mesh.vertex.attributes.emplace_back();

			attribute.descriptor.offset = value["offset"].as(attribute.descriptor.offset);
			attribute.descriptor.size = value["size"].as(attribute.descriptor.size);
			attribute.descriptor.format = (uf::renderer::enums::Format::type_t) value["format"].as<size_t>(attribute.descriptor.format);
			attribute.descriptor.name = value["name"].as(attribute.descriptor.name);
			attribute.descriptor.type = (uf::renderer::enums::Type::type_t) value["type"].as(attribute.descriptor.type);
			attribute.descriptor.components = value["components"].as(attribute.descriptor.components);
			attribute.buffer = value["buffer"].as(attribute.buffer);
		});
		mesh.index.attributes.reserve( json["inputs"]["index"]["attributes"].size() );
		ext::json::forEach( json["inputs"]["index"]["attributes"], [&]( ext::json::Value& value ){
			auto& attribute = mesh.index.attributes.emplace_back();

			attribute.descriptor.offset = value["offset"].as(attribute.descriptor.offset);
			attribute.descriptor.size = value["size"].as(attribute.descriptor.size);
			attribute.descriptor.format = (uf::renderer::enums::Format::type_t) value["format"].as<size_t>(attribute.descriptor.format);
			attribute.descriptor.name = value["name"].as(attribute.descriptor.name);
			attribute.descriptor.type = (uf::renderer::enums::Type::type_t) value["type"].as(attribute.descriptor.type);
			attribute.descriptor.components = value["components"].as(attribute.descriptor.components);
			attribute.buffer = value["buffer"].as(attribute.buffer);
		});
		mesh.instance.attributes.reserve( json["inputs"]["instance"]["attributes"].size() );
		ext::json::forEach( json["inputs"]["instance"]["attributes"], [&]( ext::json::Value& value ){
			auto& attribute = mesh.instance.attributes.emplace_back();

			attribute.descriptor.offset = value["offset"].as(attribute.descriptor.offset);
			attribute.descriptor.size = value["size"].as(attribute.descriptor.size);
			attribute.descriptor.format = (uf::renderer::enums::Format::type_t) value["format"].as<size_t>(attribute.descriptor.format);
			attribute.descriptor.name = value["name"].as(attribute.descriptor.name);
			attribute.descriptor.type = (uf::renderer::enums::Type::type_t) value["type"].as(attribute.descriptor.type);
			attribute.descriptor.components = value["components"].as(attribute.descriptor.components);
			attribute.buffer = value["buffer"].as(attribute.buffer);
		});
		mesh.indirect.attributes.reserve( json["inputs"]["indirect"]["attributes"].size() );
		ext::json::forEach( json["inputs"]["indirect"]["attributes"], [&]( ext::json::Value& value ){
			auto& attribute = mesh.indirect.attributes.emplace_back();

			attribute.descriptor.offset = value["offset"].as(attribute.descriptor.offset);
			attribute.descriptor.size = value["size"].as(attribute.descriptor.size);
			attribute.descriptor.format = (uf::renderer::enums::Format::type_t) value["format"].as<size_t>(attribute.descriptor.format);
			attribute.descriptor.name = value["name"].as(attribute.descriptor.name);
			attribute.descriptor.type = (uf::renderer::enums::Type::type_t) value["type"].as(attribute.descriptor.type);
			attribute.descriptor.components = value["components"].as(attribute.descriptor.components);
			attribute.buffer = value["buffer"].as(attribute.buffer);
		});

		mesh.buffers.reserve( json["buffers"].size() );
		ext::json::forEach( json["buffers"], [&]( ext::json::Value& value ){
			const uf::stl::string filename = value.as<uf::stl::string>();
			const uf::stl::string directory = uf::io::directory( graph.name );
			mesh.buffers.emplace_back(uf::io::readAsBuffer( directory + "/" + filename ));
		});
		
		return name;
	}

	pod::Node decodeNode( ext::json::Value& json, const pod::Graph& graph ) {
		pod::Node node = pod::Node{
			.name = json["name"].as<uf::stl::string>(),
			.index = json["index"].as<int32_t>(),
			.parent = json["parent"].as<int32_t>(-1),
			.mesh = json["mesh"].as<int32_t>(-1),
			.skin = json["skin"].as<int32_t>(-1),
			.entity = NULL,
			.transform = uf::transform::decode( json["transform"], pod::Transform<>{} ),
		};

		node.children.reserve( json["children"].size() );
		ext::json::forEach( json["children"], [&]( ext::json::Value& value ){
			node.children.emplace_back( value.as<int32_t>() );
		});
		return node;
	}
}

pod::Graph uf::graph::load( const uf::stl::string& filename, const uf::Serializer& metadata ) {
#if UF_ENV_DREAMCAST
	#define UF_DEBUG_TIMER_MULTITRACE_START(...) UF_TIMER_MULTITRACE_START(__VA_ARGS__)
	#define UF_DEBUG_TIMER_MULTITRACE(...) UF_TIMER_MULTITRACE(__VA_ARGS__)
	#define UF_DEBUG_TIMER_MULTITRACE_END(...) UF_TIMER_MULTITRACE_END(__VA_ARGS__)
#else
	#define UF_DEBUG_TIMER_MULTITRACE_START(...)
	#define UF_DEBUG_TIMER_MULTITRACE(...)
	#define UF_DEBUG_TIMER_MULTITRACE_END(...)
#endif
	const uf::stl::string extension = uf::io::extension( filename );
	if ( extension == "glb" || extension == "gltf" ) return ext::gltf::load( filename, metadata );
	const uf::stl::string directory = uf::io::directory( filename ) + "/";
	pod::Graph graph;
	uf::Serializer serializer;
	UF_DEBUG_TIMER_MULTITRACE_START("Reading " << filename);
	serializer.readFromFile( filename );
	// load metadata
	graph.name = filename; //serializer["name"].as<uf::stl::string>();
	graph.metadata = metadata; // serializer["metadata"];

	pod::Thread::container_t jobs;
	jobs.emplace_back([&]{
		// load draw command information
		UF_DEBUG_TIMER_MULTITRACE("Reading draw commands..."); 
		graph.drawCommands.reserve( serializer["drawCommands"].size() );
		ext::json::forEach( serializer["drawCommands"], [&]( ext::json::Value& value ){
			graph.drawCommands.emplace_back(decodeDrawCommand( value, graph ));
		});
	});
	jobs.emplace_back([&]{
		// load draw command information
		UF_DEBUG_TIMER_MULTITRACE("Reading instances..."); 
		graph.instances.reserve( serializer["instances"].size() );
		ext::json::forEach( serializer["instances"], [&]( ext::json::Value& value ){
			graph.instances.emplace_back(decodeInstance( value, graph ));
		});
	});
	jobs.emplace_back([&]{
		// load mesh information
		UF_DEBUG_TIMER_MULTITRACE("Reading meshes..."); 
		graph.meshes.reserve( serializer["meshes"].size() );
		ext::json::forEach( serializer["meshes"], [&]( ext::json::Value& value ){
			if ( value.is<uf::stl::string>() ) {
				UF_DEBUG_TIMER_MULTITRACE(directory + "/" + value.as<uf::stl::string>());
				uf::Serializer json;
				json.readFromFile( directory + "/" + value.as<uf::stl::string>() );
				graph.meshes.emplace_back(decodeMesh( json, graph ));
			} else {
				graph.meshes.emplace_back(decodeMesh( value, graph ));
			}
		});
	});
	#if 0
	#if !UF_ENV_DREAMCAST
	jobs.emplace_back([&]{
		if ( !ext::json::isNull( serializer["atlas"] ) ) {
			UF_DEBUG_TIMER_MULTITRACE("Reading atlas...");
			auto& image = graph.atlas.getAtlas();
			auto& value = serializer["atlas"];
			if ( value.is<uf::stl::string>() ) {
				uf::stl::string filename = directory + "/" + value.as<uf::stl::string>();
				UF_DEBUG_TIMER_MULTITRACE("Reading atlas " << filename);
				image.open(filename, false);
			} else {
				decode( value, image, graph );
			}
		}
	});
	#endif
	#endif
	jobs.emplace_back([&]{
		// load texture information
		UF_DEBUG_TIMER_MULTITRACE("Reading texture information...");
		graph.textures.reserve( serializer["textures"].size() );
		ext::json::forEach( serializer["textures"], [&]( ext::json::Value& value ){
			graph.textures.emplace_back(decodeTexture( value, graph ));
		});
	});
	jobs.emplace_back([&]{
		// load images
		UF_DEBUG_TIMER_MULTITRACE("Reading images...");
		graph.images.reserve( serializer["images"].size() );
		ext::json::forEach( serializer["images"], [&]( ext::json::Value& value ){
			graph.images.emplace_back(decodeImage( value, graph ));
		});
	});
	jobs.emplace_back([&]{
		// load sampler information
		UF_DEBUG_TIMER_MULTITRACE("Reading sampler information...");
		graph.samplers.reserve( serializer["samplers"].size() );
		ext::json::forEach( serializer["samplers"], [&]( ext::json::Value& value ){
			graph.samplers.emplace_back(decodeSampler( value, graph ));
		});
	});
	jobs.emplace_back([&]{
		// load material information
		UF_DEBUG_TIMER_MULTITRACE("Reading material information...");
		graph.materials.reserve( serializer["materials"].size() );
		ext::json::forEach( serializer["materials"], [&]( ext::json::Value& value ){
			graph.materials.emplace_back(decodeMaterial( value, graph ));
		});
	});
	jobs.emplace_back([&]{
		// load light information
		UF_DEBUG_TIMER_MULTITRACE("Reading lighting information...");
		graph.lights.reserve( serializer["lighting"].size() );
		ext::json::forEach( serializer["lighting"], [&]( ext::json::Value& value ){
			auto name = value["name"].as<uf::stl::string>();
			graph.lights[name] = decodeLight( value, graph );
		});
	});
	jobs.emplace_back([&]{
		// load animation information
		UF_DEBUG_TIMER_MULTITRACE("Reading animation information...");
		graph.animations.reserve( serializer["animations"].size() );
		ext::json::forEach( serializer["animations"], [&]( ext::json::Value& value ){
			if ( value.is<uf::stl::string>() ) {
				uf::Serializer json;
				json.readFromFile( directory + "/" + value.as<uf::stl::string>() );
				uf::stl::string key = json["name"].as<uf::stl::string>();
				graph.animations[key] = decodeAnimation( json, graph );
			} else {
				uf::stl::string key = value["name"].as<uf::stl::string>();
				graph.animations[key] = decodeAnimation( value, graph );
			}
		});
	});
	jobs.emplace_back([&]{
		// load skin information
		UF_DEBUG_TIMER_MULTITRACE("Reading skinning information...");
		graph.skins.reserve( serializer["skins"].size() );
		ext::json::forEach( serializer["skins"], [&]( ext::json::Value& value ){
			graph.skins.emplace_back(decodeSkin( value, graph ));
		});
	});
	jobs.emplace_back([&]{
		// load node information
		UF_DEBUG_TIMER_MULTITRACE("Reading nodes...");
		graph.nodes.reserve( serializer["nodes"].size() );
		ext::json::forEach( serializer["nodes"], [&]( ext::json::Value& value ){
			graph.nodes.emplace_back(decodeNode( value, graph ));
		});
		graph.root = decodeNode( serializer["root"], graph );
	});
#if UF_GRAPH_LOAD_MULTITHREAD
	if ( !jobs.empty() ) uf::thread::batchWorkers( jobs );
#else
	for ( auto& job : jobs ) job();
#endif

	// re-reference all transform parents
	for ( auto& node : graph.nodes ) {
		if ( 0 <= node.parent && node.parent < graph.nodes.size() && node.index != node.parent ) {
			node.transform.reference = &graph.nodes[node.parent].transform;
		}
	}
	UF_DEBUG_TIMER_MULTITRACE_END("Processing graph...");
	return graph;
}

namespace {
	struct EncodingSettings : public ext::json::EncodingSettings {
		bool combined = false;
		bool encodeBuffers = true;
		bool unwrap = true;
		uf::stl::string filename = "";
	};

	uf::Serializer encode( const uf::Image& image, const EncodingSettings& settings ) {
		uf::Serializer json;
		json["size"] = uf::vector::encode( image.getDimensions() );
		json["bpp"] = image.getBpp() / image.getChannels();
		json["channels"] = image.getChannels();
		json["data"] = uf::base64::encode( image.getPixels() );
		return json;
	}
	uf::Serializer encode( const pod::Texture& texture, const EncodingSettings& settings ) {
		uf::Serializer json;
		json["index"] = texture.index;
		json["sampler"] = texture.sampler;
		json["remap"] = texture.remap;
		json["blend"] = texture.blend;
		json["lerp"] = uf::vector::encode( texture.lerp, settings );
		return json;
	}
	uf::Serializer encode( const uf::renderer::Sampler& sampler, const EncodingSettings& settings ) {
		uf::Serializer json;
		json["min"] = sampler.descriptor.filter.min;
		json["mag"] = sampler.descriptor.filter.mag;
		json["u"] = sampler.descriptor.addressMode.u;
		json["v"] = sampler.descriptor.addressMode.v;
		return json;
	}
	uf::Serializer encode( const pod::Material& material, const EncodingSettings& settings ) {
		uf::Serializer json;
		json["base"] = uf::vector::encode( material.colorBase, settings );
		json["emissive"] = uf::vector::encode( material.colorEmissive, settings );
		json["fMetallic"] = material.factorMetallic;
		json["fRoughness"] = material.factorRoughness;
		json["fOcclusion"] = material.factorOcclusion;
		json["fAlphaCutoff"] = material.factorAlphaCutoff;
		if ( material.indexAlbedo >= 0 ) json["iAlbedo"] = material.indexAlbedo;
		if ( material.indexNormal >= 0 ) json["iNormal"] = material.indexNormal;
		if ( material.indexEmissive >= 0 ) json["iEmissive"] = material.indexEmissive;
		if ( material.indexOcclusion >= 0 ) json["iOcclusion"] = material.indexOcclusion;
		if ( material.indexMetallicRoughness >= 0 ) json["iMetallicRoughness"] = material.indexMetallicRoughness;
		json["modeAlpha"] = material.modeAlpha;
		return json;
	}
	uf::Serializer encode( const pod::Light& light, const EncodingSettings& settings ) {
		uf::Serializer json;
		json["color"] = uf::vector::encode( light.color, settings );
		json["intensity"] = light.intensity;
		json["range"] = light.range;
		return json;
	}
	uf::Serializer encode( const pod::Animation& animation, const EncodingSettings& settings ) {
		uf::Serializer json;
		json["name"] = animation.name;
		json["start"] = animation.start;
		json["end"] = animation.end;

		ext::json::reserve( json["samplers"], animation.samplers.size() );
		auto& samplers = json["samplers"];
		for ( auto& sampler : animation.samplers ) {
			auto& json = samplers.emplace_back();
			json["interpolator"] = sampler.interpolator;
			for ( auto& input : sampler.inputs ) {
				json["inputs"].emplace_back(input);
			}
			for ( auto& output : sampler.outputs ) {
				json["outputs"].emplace_back(uf::vector::encode( output, settings ));
			}
		}
		ext::json::reserve( json["channels"], animation.channels.size() );
		auto& channels = json["channels"];
		for ( auto& channel : animation.channels ) {
			auto& json = channels.emplace_back();
			json["path"] = channel.path;
			json["node"] = channel.node;
			json["sampler"] = channel.sampler;
		}
		return json;
	}	
	uf::Serializer encode( const pod::Skin& skin, const EncodingSettings& settings ) {
		uf::Serializer json;
		json["name"] = skin.name;

		ext::json::reserve( json["joints"], skin.joints.size() );
		for ( auto& joint : skin.joints ) json["joints"].emplace_back( joint );

		ext::json::reserve( json["inverseBindMatrices"], skin.inverseBindMatrices.size() );
		for ( auto& inverseBindMatrix : skin.inverseBindMatrices )
			json["inverseBindMatrices"].emplace_back( uf::matrix::encode(inverseBindMatrix, settings) );
		return json;
	}
	uf::Serializer encode( const uf::stl::vector<pod::DrawCommand>& drawCommands, const EncodingSettings& settings ) {
		uf::Serializer json;
		ext::json::reserve( json, drawCommands.size() );

		for ( auto& drawCommand : drawCommands ) {
			auto& d = json.emplace_back();
			d["indices"] = drawCommand.indices;
			d["instances"] = drawCommand.instances;
			d["indexID"] = drawCommand.indexID;
			d["vertexID"] = drawCommand.vertexID;

			d["instanceID"] = drawCommand.instanceID;
			d["materialID"] = drawCommand.materialID;
			d["objectID"] = drawCommand.objectID;
			d["vertices"] = drawCommand.vertices;
		}

		return json;
	}
	uf::Serializer encode( const pod::Instance& instance, const EncodingSettings& settings ) {
		uf::Serializer json;
		return json;
	}
	uf::Serializer encode( const pod::Mesh& pair, const EncodingSettings& settings ) {
		uf::Serializer json;
		json["drawCommands"] = encode( pair.drawCommands, settings );


		auto& mesh = pair.mesh;
		json["inputs"]["vertex"]["count"] = mesh.vertex.count;
		json["inputs"]["vertex"]["first"] = mesh.vertex.first;
		json["inputs"]["vertex"]["stride"] = mesh.vertex.stride;
		json["inputs"]["vertex"]["offset"] = mesh.vertex.offset;
		ext::json::reserve( json["inputs"]["vertex"]["attributes"], mesh.vertex.attributes.size() );
		for ( auto& attribute : mesh.vertex.attributes ) {
			auto& a = json["inputs"]["vertex"]["attributes"].emplace_back();
			a["descriptor"]["offset"] = attribute.descriptor.offset;
			a["descriptor"]["size"] = attribute.descriptor.size;
			a["descriptor"]["format"] = attribute.descriptor.format;
			a["descriptor"]["name"] = attribute.descriptor.name;
			a["descriptor"]["type"] = attribute.descriptor.type;
			a["descriptor"]["components"] = attribute.descriptor.components;
			a["buffer"] = attribute.buffer;
		}

		json["inputs"]["index"]["count"] = mesh.index.count;
		json["inputs"]["index"]["first"] = mesh.index.first;
		json["inputs"]["index"]["stride"] = mesh.index.stride;
		json["inputs"]["index"]["offset"] = mesh.index.offset;
		ext::json::reserve( json["inputs"]["index"]["attributes"], mesh.index.attributes.size() );
		for ( auto& attribute : mesh.index.attributes ) {
			auto& a = json["inputs"]["index"]["attributes"].emplace_back();
			a["descriptor"]["offset"] = attribute.descriptor.offset;
			a["descriptor"]["size"] = attribute.descriptor.size;
			a["descriptor"]["format"] = attribute.descriptor.format;
			a["descriptor"]["name"] = attribute.descriptor.name;
			a["descriptor"]["type"] = attribute.descriptor.type;
			a["descriptor"]["components"] = attribute.descriptor.components;
			a["buffer"] = attribute.buffer;
		}

		json["inputs"]["instance"]["count"] = mesh.instance.count;
		json["inputs"]["instance"]["first"] = mesh.instance.first;
		json["inputs"]["instance"]["stride"] = mesh.instance.stride;
		json["inputs"]["instance"]["offset"] = mesh.instance.offset;
		ext::json::reserve( json["inputs"]["instance"]["attributes"], mesh.instance.attributes.size() );
		for ( auto& attribute : mesh.instance.attributes ) {
			auto& a = json["inputs"]["instance"]["attributes"].emplace_back();
			a["descriptor"]["offset"] = attribute.descriptor.offset;
			a["descriptor"]["size"] = attribute.descriptor.size;
			a["descriptor"]["format"] = attribute.descriptor.format;
			a["descriptor"]["name"] = attribute.descriptor.name;
			a["descriptor"]["type"] = attribute.descriptor.type;
			a["descriptor"]["components"] = attribute.descriptor.components;
			a["buffer"] = attribute.buffer;
		}

		json["inputs"]["indirect"]["count"] = mesh.indirect.count;
		json["inputs"]["indirect"]["first"] = mesh.indirect.first;
		json["inputs"]["indirect"]["stride"] = mesh.indirect.stride;
		json["inputs"]["indirect"]["offset"] = mesh.indirect.offset;
		ext::json::reserve( json["inputs"]["indirect"]["attributes"], mesh.indirect.attributes.size() );
		for ( auto& attribute : mesh.indirect.attributes ) {
			auto& a = json["inputs"]["indirect"]["attributes"].emplace_back();
			a["descriptor"]["offset"] = attribute.descriptor.offset;
			a["descriptor"]["size"] = attribute.descriptor.size;
			a["descriptor"]["format"] = attribute.descriptor.format;
			a["descriptor"]["name"] = attribute.descriptor.name;
			a["descriptor"]["type"] = attribute.descriptor.type;
			a["descriptor"]["components"] = attribute.descriptor.components;
			a["buffer"] = attribute.buffer;
		}

		ext::json::reserve( json["buffers"], mesh.buffers.size() );
		for ( auto i = 0; i < mesh.buffers.size(); ++i ) {
			const uf::stl::string filename = settings.filename + ".buffer." + std::to_string(i) + "." + ( settings.compress ? "gz" : "bin" );
			uf::io::write( filename, mesh.buffers[i] );
			json["buffers"].emplace_back(uf::io::filename( filename ));
		}

	#if 0
		ext::json::reserve( json["attributes"], mesh.attributes.vertex.descriptor.size() );
		for ( auto& attribute : mesh.attributes.vertex.descriptor ) {
			auto& a = json["attributes"].emplace_back();
			a["name"] = attribute.name;
			a["size"] = attribute.size;
			a["type"] = attribute.type;
			a["format"] = attribute.format;
			a["offset"] = attribute.offset;
			a["components"] = attribute.components;
		}

		// validation metadata
		json["metadata"]["vertices"]["size"] = mesh.attributes.vertex.size;
		json["metadata"]["vertices"]["count"] = mesh.attributes.vertex.length;

		json["metadata"]["indices"]["size"] = mesh.attributes.index.size;
		json["metadata"]["indices"]["count"] = mesh.attributes.index.length;

		if ( !settings.encodeBuffers ){
		#if UF_GRAPH_VARYING_MESH
			size_t indices = mesh.attributes.index.length;
			size_t indexStride = mesh.attributes.index.size;
			uint8_t* indexPointer = (uint8_t*) mesh.attributes.index.pointer;

			size_t vertices = mesh.attributes.vertex.length;
			size_t vertexStride = mesh.attributes.vertex.size;
			uint8_t* vertexPointer = (uint8_t*) mesh.attributes.vertex.pointer;
			
			ext::json::reserve( json["vertices"], mesh.attributes.vertex.length );
			ext::json::reserve( json["indices"], indices );
			for ( size_t currentIndex = 0; currentIndex < indices; ++currentIndex ) {
				uint32_t index {};
				uint8_t* indexSrc = indexPointer + (currentIndex * indexStride);
				switch ( indexStride ) {
					case sizeof( uint8_t): index = *(( uint8_t*) indexSrc); break;
					case sizeof(uint16_t): index = *((uint16_t*) indexSrc); break;
					case sizeof(uint32_t): index = *((uint32_t*) indexSrc); break;
				}
				json["indices"].emplace_back( index );

				auto& v = json["vertices"].emplace_back();
				for ( auto& attribute : mesh.attributes.vertex.descriptor ) {
					uint8_t* vertexSrc = (vertexPointer + (index * vertexStride)) + attribute.offset;
					if (  attribute.name == "id" || attribute.name == "joints" )
						switch ( attribute.components ) {
							case 1: v.emplace_back( uf::vector::encode( *((pod::Vector<uint32_t, 1>*) vertexSrc), settings ) ); break;
							case 2: v.emplace_back( uf::vector::encode( *((pod::Vector<uint32_t, 2>*) vertexSrc), settings ) ); break;
							case 3: v.emplace_back( uf::vector::encode( *((pod::Vector<uint32_t, 3>*) vertexSrc), settings ) ); break;
							case 4: v.emplace_back( uf::vector::encode( *((pod::Vector<uint32_t, 4>*) vertexSrc), settings ) ); break;
						}
					else
						switch ( attribute.components ) {
							case 1: v.emplace_back( uf::vector::encode( *((pod::Vector<float, 1>*) vertexSrc), settings ) ); break;
							case 2: v.emplace_back( uf::vector::encode( *((pod::Vector<float, 2>*) vertexSrc), settings ) ); break;
							case 3: v.emplace_back( uf::vector::encode( *((pod::Vector<float, 3>*) vertexSrc), settings ) ); break;
							case 4: v.emplace_back( uf::vector::encode( *((pod::Vector<float, 4>*) vertexSrc), settings ) ); break;
						}
				}
			}
		#if 0
			if ( mesh.is<uf::graph::mesh::Base>() ) {
				auto& m = mesh.get<uf::graph::mesh::Base>();
				for ( auto& vertex : m.vertices ) {
					auto& v = json["vertices"].emplace_back();
					v.emplace_back(!vertex.position ? ext::json::array() : uf::vector::encode(vertex.position, settings));
					v.emplace_back(!vertex.uv ? ext::json::array() : uf::vector::encode(vertex.uv, settings));
					v.emplace_back(!vertex.st ? ext::json::array() : uf::vector::encode(vertex.st, settings));
					v.emplace_back(!vertex.normal ? ext::json::array() : uf::vector::encode(vertex.normal, settings));
				}
			} else if ( mesh.is<uf::graph::mesh::ID>() ) {
				auto& m = mesh.get<uf::graph::mesh::ID>();
				for ( auto& vertex : m.vertices ) {
					auto& v = json["vertices"].emplace_back();
					v.emplace_back(!vertex.position ? ext::json::array() : uf::vector::encode(vertex.position, settings));
					v.emplace_back(!vertex.uv ? ext::json::array() : uf::vector::encode(vertex.uv, settings));
					v.emplace_back(!vertex.st ? ext::json::array() : uf::vector::encode(vertex.st, settings));
					v.emplace_back(!vertex.normal ? ext::json::array() : uf::vector::encode(vertex.normal, settings));
					v.emplace_back(!vertex.tangent ? ext::json::array() : uf::vector::encode(vertex.tangent, settings));
					v.emplace_back(!vertex.id ? ext::json::array() : uf::vector::encode(vertex.id, settings));
				}
			} else if ( mesh.is<uf::graph::mesh::Skinned>() ) {
				auto& m = mesh.get<uf::graph::mesh::Skinned>();
				for ( auto& vertex : m.vertices ) {
					auto& v = json["vertices"].emplace_back();
					v.emplace_back(!vertex.position ? ext::json::array() : uf::vector::encode(vertex.position, settings));
					v.emplace_back(!vertex.uv ? ext::json::array() : uf::vector::encode(vertex.uv, settings));
					v.emplace_back(!vertex.st ? ext::json::array() : uf::vector::encode(vertex.st, settings));
					v.emplace_back(!vertex.normal ? ext::json::array() : uf::vector::encode(vertex.normal, settings));
					v.emplace_back(!vertex.tangent ? ext::json::array() : uf::vector::encode(vertex.tangent, settings));
					v.emplace_back(!vertex.id ? ext::json::array() : uf::vector::encode(vertex.id, settings));
					v.emplace_back(!vertex.joints ? ext::json::array() : uf::vector::encode(vertex.joints, settings));
					v.emplace_back(!vertex.weights ? ext::json::array() : uf::vector::encode(vertex.weights, settings));
				}
			}
		#endif
		#else
			for ( auto& vertex : mesh.vertices ) {
				auto& v = json["vertices"].emplace_back();
				v.emplace_back(!vertex.position ? ext::json::array() : uf::vector::encode(vertex.position, settings));
				v.emplace_back(!vertex.uv ? ext::json::array() : uf::vector::encode(vertex.uv, settings));
				v.emplace_back(!vertex.st ? ext::json::array() : uf::vector::encode(vertex.st, settings));
				v.emplace_back(!vertex.normal ? ext::json::array() : uf::vector::encode(vertex.normal, settings));
				v.emplace_back(!vertex.tangent ? ext::json::array() : uf::vector::encode(vertex.tangent, settings));
				v.emplace_back(!vertex.id ? ext::json::array() : uf::vector::encode(vertex.id, settings));
				v.emplace_back(!vertex.joints ? ext::json::array() : uf::vector::encode(vertex.joints, settings));
				v.emplace_back(!vertex.weights ? ext::json::array() : uf::vector::encode(vertex.weights, settings));
			}
		#endif
		} else {
			const uf::stl::string verticesFilename = settings.filename + ".vertices." + ( settings.compress ? "gz" : "bin" );
			uf::io::write( verticesFilename, mesh.attributes.vertex.pointer, mesh.attributes.vertex.size * mesh.attributes.vertex.length );
			json["vertices"] = uf::io::filename( verticesFilename );

			if ( mesh.attributes.index.length > 0 ) {
				const uf::stl::string indicesFilename = settings.filename + ".indices." + ( settings.compress ? "gz" : "bin" );
				uf::io::write( indicesFilename, mesh.attributes.index.pointer, mesh.attributes.index.size * mesh.attributes.index.length );
				json["indices"] = uf::io::filename( indicesFilename );
			}
		}
	#endif
		return json;
	}
	uf::Serializer encode( const pod::Node& node, const EncodingSettings& settings ) {
		uf::Serializer json;
		json["name"] = node.name;
		json["index"] = node.index;
		if ( node.skin >= 0 ) json["skin"] = node.skin;
		if ( node.mesh >= 0 ) json["mesh"] = node.mesh;
		if ( node.parent >= 0 ) json["parent"] = node.parent;
		ext::json::reserve( json["children"], node.children.size() );
		for ( auto& child : node.children ) json["children"].emplace_back(child);

		json["transform"] = uf::transform::encode( node.transform, false, settings );
		return json;
	}
}
void uf::graph::save( const pod::Graph& graph, const uf::stl::string& filename ) {
	uf::stl::string directory = uf::io::directory( filename ) + "/" + uf::io::basename(filename) + "/";
	uf::stl::string target = uf::io::directory( filename ) + "/" + uf::io::basename(filename) + ".graph";

	uf::Serializer serializer;
	uf::Serializer metadata;

	const ::EncodingSettings settings = ::EncodingSettings{
		{
			/*.pretty = */graph.metadata["export"]["pretty"].as<bool>(),
			/*.compress = */graph.metadata["export"]["compress"].as<bool>(),
			/*.quantize = */graph.metadata["export"]["quantize"].as<bool>(),
			/*.precision = */graph.metadata["export"]["precision"].as<uint8_t>(),
		},
		/*.combined = */graph.metadata["export"]["combined"].as<bool>(),
		/*.encodeBuffers = */graph.metadata["export"]["encode buffers"].as<bool>(true),
		/*.unwrap = */graph.metadata["export"]["unwrap"].as<bool>(true),
		/*.filename = */directory + "/graph.json",
	};
	if ( !settings.combined ) uf::io::mkdir(directory);
	if ( settings.unwrap ) {
		pod::Graph& g = const_cast<pod::Graph&>(graph);
		auto size = ext::xatlas::unwrap( g );
		serializer["wrapped"] = uf::vector::encode( size );
	}

	pod::Thread::container_t jobs;
	// store images
	jobs.emplace_back([&]{
	#if 0
		if ( graph.atlas.generated() ) {
			auto& image = graph.atlas.getAtlas();
			if ( !settings.combined ) {
				image.save(directory + "/atlas.png");
				serializer["atlas"] = "atlas.png";
			} else {
				serializer["atlas"] = encode(image, settings);
			}
		}
	#endif
	});
	jobs.emplace_back([&]{
		ext::json::reserve( serializer["images"], graph.images.size() );
		if ( !settings.combined ) {
			for ( size_t i = 0; i < graph.images.size(); ++i ) {
				auto& name = graph.images[i];
				auto& image = uf::graph::storage.images[name];
				uf::stl::string f = "image."+std::to_string(i)+".png";

				image.save(directory + "/" + f);
				uf::Serializer json;
				json["name"] = name;
				json["filename"] = f;
				serializer["images"].emplace_back( json );
			}
		} else {
			for ( auto& name : graph.images ) {
				auto& image = uf::graph::storage.images[name];
				auto json = encode(image, settings);
				json["name"] = name;
				serializer["images"].emplace_back( json );
			}
		}
	});
	jobs.emplace_back([&]{
		// store texture information
		ext::json::reserve( serializer["textures"], graph.textures.size() );
		for ( auto& name : graph.textures ) {
			auto& texture = uf::graph::storage.textures[name];
			auto json = encode(texture, settings);
			json["name"] = name;
			serializer["textures"].emplace_back(json);
		}
	});
	jobs.emplace_back([&]{
		// store sampler information
		ext::json::reserve( serializer["samplers"], graph.samplers.size() );
		for ( auto& name : graph.samplers ) {
			auto& sampler = uf::graph::storage.samplers[name];
			auto json = encode(sampler, settings);
			json["name"] = name;
			serializer["samplers"].emplace_back(json);
		}
	});
	jobs.emplace_back([&]{
		// store material information
		ext::json::reserve( serializer["materials"], graph.materials.size() );
		for ( auto& name : graph.materials ) {
			auto& material = uf::graph::storage.materials[name];
			auto json = encode(material, settings);
			json["name"] = name;
			serializer["materials"].emplace_back(json);
		}
	});
	jobs.emplace_back([&]{
		// store light information
		ext::json::reserve( serializer["lighting"], graph.lights.size() );
		for ( auto pair : graph.lights ) {
			auto& name = pair.first;
			auto& light = pair.second;
			auto json = encode(light, settings);
			json["name"] = name;
			serializer["lighting"].emplace_back(json);
		}
	});
	jobs.emplace_back([&]{
		// store animation information
		ext::json::reserve( serializer["animations"], graph.animations.size() );
		if ( !settings.combined ) {
			for ( auto pair : graph.animations ) {
				uf::stl::string f = "animation."+pair.first+".json";
				encode(pair.second, settings).writeToFile(directory+"/animation."+pair.first+".json", settings);
				serializer["animations"].emplace_back("animation."+pair.first+".json" + (settings.compress ? ".gz" : ""));
			}
		} else {
			for ( auto pair : graph.animations ) serializer["animations"][pair.first] = encode(pair.second, settings);
		}
	});
	jobs.emplace_back([&]{
		// store skin information
		ext::json::reserve( serializer["skins"], graph.skins.size() );
		for ( auto& skin : graph.skins ) serializer["skins"].emplace_back( encode(skin, settings) );
	});
	jobs.emplace_back([&]{
		// store draw command information
		ext::json::reserve( serializer["drawCommands"], graph.drawCommands.size() );
		for ( auto& drawCommand : graph.drawCommands ) serializer["drawCommands"].emplace_back( encode(drawCommand, settings) );
	});
	jobs.emplace_back([&]{
		// store instance information
		ext::json::reserve( serializer["instances"], graph.instances.size() );
		for ( auto& instance : graph.instances ) serializer["instances"].emplace_back( encode(instance, settings) );
	});
	jobs.emplace_back([&]{
		// store mesh information
		ext::json::reserve( serializer["meshes"], graph.meshes.size() );
		if ( !settings.combined ) {
			::EncodingSettings s = settings;
			for ( size_t i = 0; i < graph.meshes.size(); ++i ) {
				auto& name = graph.meshes[i];
				auto& mesh = uf::graph::storage.meshes[name];
				if ( !s.encodeBuffers ) {
					s.filename = directory+"/mesh."+std::to_string(i)+".json";
					encode(mesh, s).writeToFile(s.filename, settings);
					uf::Serializer json;
					json["name"] = name;
					json["filename"] = uf::io::filename(s.filename + (settings.compress ? ".gz" : ""));
					serializer["meshes"].emplace_back( json );
				} else {
					s.filename = directory+"/mesh."+std::to_string(i);
					auto json = encode(mesh, s);
					json["name"] = name;
					serializer["meshes"].emplace_back(json);
				}
			}
		} else {
			for ( auto& name : graph.meshes ) {
				auto& mesh = uf::graph::storage.meshes[name];
				auto json = encode(mesh, settings);
				json["name"] = name;
				serializer["meshes"].emplace_back(json);
			}
		}
	});
	jobs.emplace_back([&]{
		// store node information
		ext::json::reserve( serializer["nodes"], graph.nodes.size() );
		for ( auto& node : graph.nodes ) serializer["nodes"].emplace_back( encode(node, settings) );
		serializer["root"] = encode(graph.root, settings);
	});
#if UF_GRAPH_LOAD_MULTITHREAD
	if ( !jobs.empty() ) uf::thread::batchWorkers( jobs );
#else
	for ( auto& job : jobs ) return job();
#endif

	if ( !settings.combined ) target = directory + "/graph.json";
	serializer.writeToFile( target, settings );
}

uf::stl::string uf::graph::print( const pod::Graph& graph ) {
	uf::stl::stringstream ss;
#if 0
	ss << "Graph Data:"
		"\n\tImages: " << graph.images.size() << ""
		"\n\tTextures: " << graph.textures.size() << ""
		"\n\tMaterials: " << graph.materials.size() << ""
		"\n\tLights: " << graph.lights.size() << ""
		"\n\tMeshes: " << graph.meshes.size() << ""
		"\n\tAnimations: " << graph.animations.size() << ""
		"\n\tNodes: " << graph.nodes.size() << ""
		"\n";
	ss << "Graph Tree: \n";
	std::function<void(const pod::Node&, size_t)> print = [&]( const pod::Node& node, size_t indent ) {
		for ( size_t i = 0; i < indent; ++i ) ss << "\t";
		ss << "Node[" << node.index << "] " << node.name << ":\n";
		for ( auto index : node.children ) print( graph.nodes[index], indent + 1 );
	};
	print( graph.root, 1 );
#endif
	return ss.str();
}
uf::Serializer uf::graph::stats( const pod::Graph& graph ) {
	ext::json::Value json;
#if 0
	size_t memoryTextures = sizeof(pod::Texture::Storage) * graph.textures.size();
	size_t memoryMaterials = sizeof(pod::Material::Storage) * graph.materials.size();
	size_t memoryLights = sizeof(pod::Light) * graph.lights.size();
	size_t memoryImages = 0;
	size_t memoryMeshes = 0;
	size_t memoryAnimations = 0;
	size_t memoryNodes = 0;
	size_t memoryStrings = 0;
	size_t stringsCount = 0;

	for ( auto& texture : graph.textures ) {
		memoryStrings += sizeof(char) * texture.name.length();
		++memoryStrings;
	}
	for ( auto& material : graph.materials ) {
		memoryStrings += sizeof(char) * material.name.length();
		++memoryStrings;
	}
	for ( auto& light : graph.lights ) {
		memoryStrings += sizeof(char) * light.name.length();
		++memoryStrings;
	}

	for ( auto& image : graph.images ) memoryImages += image.getPixels().size();
	for ( auto& mesh : graph.meshes ) {
	//	memoryMeshes += sizeof(uf::Mesh::vertex_t) * mesh.vertices.size();
	//	memoryMeshes += sizeof(uf::Mesh::index_t) * mesh.indices.size();
		memoryMeshes += mesh.attributes.vertex.size * mesh.attributes.vertex.length;
		memoryMeshes += mesh.attributes.index.size * mesh.attributes.index.length;
	}
	for ( auto pair : graph.animations ) {
		memoryAnimations += sizeof(float) * 3;
		for ( auto& sampler : pair.second.samplers ) {
			memoryAnimations += sizeof(float) * 1 * sampler.inputs.size();
			memoryAnimations += sizeof(float) * 4 * sampler.outputs.size();
			memoryStrings += sizeof(char) * sampler.interpolator.length();
			++stringsCount;
		}
		for ( auto& channel : pair.second.channels ) {
			memoryAnimations += sizeof(uint32_t) * 2;
			memoryStrings += sizeof(char) * channel.path.length();
			++stringsCount;
		}
	}
	for ( auto& node : graph.nodes ) {
		memoryNodes += sizeof(int32_t) * 4;
		memoryNodes += sizeof(int32_t) * node.children.size();
		memoryStrings += sizeof(char) * node.name.length();
		++stringsCount;
	}

	json["strings"]["size"] = stringsCount; json["strings"]["bytes"] = memoryStrings;
	json["images"]["size"] = graph.images.size(); json["images"]["bytes"] = memoryImages;
	json["textures"]["size"] = graph.textures.size(); json["textures"]["bytes"] = memoryTextures;
	json["materials"]["size"] = graph.materials.size(); json["materials"]["bytes"] = memoryMaterials;
	json["lights"]["size"] = graph.lights.size(); json["lights"]["bytes"] = memoryLights;
	json["meshes"]["size"] = graph.meshes.size(); json["meshes"]["bytes"] = memoryMeshes;
	json["animations"]["size"] = graph.animations.size(); json["animations"]["bytes"] = memoryAnimations;
	json["nodes"]["size"] = graph.nodes.size(); json["nodes"]["bytes"] = memoryNodes;
	json["bytes"] = (memoryTextures + memoryMaterials + memoryLights + memoryImages + memoryMeshes + memoryAnimations + memoryNodes + memoryStrings);
/*
	std::cout << "Graph stats: " << 
		"\n\tNames: Bytes: " << memoryStrings << ""
		"\n\tImages: " << graph.images.size() << " | Bytes: " << memoryImages << ""
		"\n\tTextures: " << graph.textures.size() << " | Bytes: " << memoryTextures << ""
		"\n\tMaterials: " << graph.materials.size() << " | Bytes: " << memoryMaterials << ""
		"\n\tLights: " << graph.lights.size() << " | Bytes: " << memoryLights << ""
		"\n\tMeshes: " << graph.meshes.size() << " | Bytes: " << memoryMeshes << ""
		"\n\tAnimations: " << graph.animations.size() << " | Bytes: " << memoryAnimations << ""
		"\n\tNodes: " << graph.nodes.size() << " | Bytes: " << memoryNodes << ""
		"\n\tTotal: " << (memoryTextures + memoryMaterials + memoryLights + memoryImages + memoryMeshes + memoryAnimations + memoryNodes + memoryStrings) << std::endl;
*/
#endif
	return json;
}