#include <uf/engine/graph/graph.h>
#include <uf/ext/bullet/bullet.h>
#include <uf/ext/gltf/gltf.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/mesh/grid.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/string/base64.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>
#include <uf/ext/xatlas/xatlas.h>

#if UF_ENV_DREAMCAST
	#define UF_GRAPH_LOAD_MULTITHREAD 0
#else
	#define UF_GRAPH_LOAD_MULTITHREAD 1
#endif

namespace {
	bool newGraphAdded = true;
}

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
		uf::stl::string vertexShaderFilename = graph.metadata["shaders"]["vertex"].as<uf::stl::string>("/graph/base.vert.spv");
		uf::stl::string geometryShaderFilename = graph.metadata["shaders"]["geometry"].as<uf::stl::string>("");
		uf::stl::string fragmentShaderFilename = graph.metadata["shaders"]["fragment"].as<uf::stl::string>("/graph/base.frag.spv");
		{
			graphic.material.metadata.autoInitializeUniforms = false;
			if ( uf::renderer::settings::experimental::deferredSampling ) {
				fragmentShaderFilename = uf::string::replace( fragmentShaderFilename, "frag", "deferredSampling.frag" );
			}
			{
				if ( !graph.metadata["flags"]["SEPARATE"].as<bool>() ) {
					vertexShaderFilename = graph.metadata["flags"]["SKINNED"].as<bool>() ? "/graph/skinned.instanced.vert.spv" : "/graph/instanced.vert.spv";
				} else if ( graph.metadata["flags"]["SKINNED"].as<bool>() ) vertexShaderFilename = "/graph/skinned.vert.spv";
				vertexShaderFilename = entity.grabURI( vertexShaderFilename, root );
				graphic.material.attachShader(vertexShaderFilename, uf::renderer::enums::Shader::VERTEX);
			}
			{
				fragmentShaderFilename = entity.grabURI( fragmentShaderFilename, root );
				graphic.material.attachShader(fragmentShaderFilename, uf::renderer::enums::Shader::FRAGMENT);
			}
		#if UF_USE_VULKAN
			graphic.material.metadata.autoInitializeUniforms = true;
			if ( geometryShaderFilename != "" && uf::renderer::device.enabledFeatures.geometryShader ) {
				geometryShaderFilename = entity.grabURI( geometryShaderFilename, root );
				graphic.material.attachShader(geometryShaderFilename, uf::renderer::enums::Shader::GEOMETRY);
			}
			{
				uint32_t maxPasses = 6;

				auto& shader = graphic.material.getShader("vertex");
				uint32_t* specializationConstants = (uint32_t*) (void*) shader.specializationConstants;
				for ( auto pair : shader.metadata.definitions.specializationConstants ) {
					auto& sc = pair.second;
					if ( sc.name == "PASSES" ) sc.value.ui = (specializationConstants[sc.index] = maxPasses);
				}

				uf::renderer::Buffer* indirect = NULL;
				for ( auto& buffer : graphic.buffers ) if ( !indirect && buffer.usage & uf::renderer::enums::Buffer::INDIRECT ) indirect = &buffer;

				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.camera );
			//	shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.drawCommands );
				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.instance );
				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.joint );
			}
			{
				uint32_t maxTextures = graph.textures.size(); // texture2Ds;

				auto& shader = graphic.material.getShader("fragment");
				uint32_t* specializationConstants = (uint32_t*) (void*) shader.specializationConstants;
				for ( auto pair : shader.metadata.definitions.specializationConstants ) {
					auto& sc = pair.second;
					if ( sc.name == "TEXTURES" ) sc.value.ui = (specializationConstants[sc.index] = maxTextures);
				}
				for ( auto pair : shader.metadata.definitions.textures ) {
					auto& tx = pair.second;
					for ( auto& layout : shader.descriptorSetLayoutBindings ) {
						if ( layout.binding != tx.binding ) continue;
						if ( tx.name == "samplerTextures" ) layout.descriptorCount = maxTextures;
					}
				}

				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.material );
				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.texture );
				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.light );
			}
		#endif
		}
		// culling pipeline
		if ( uf::renderer::settings::experimental::culling ) {
			graphic.material.metadata.autoInitializeUniforms = false;
			uf::stl::string compShaderFilename = graph.metadata["shaders"]["vertex"].as<uf::stl::string>("/graph/cull.comp.spv");
			{
				compShaderFilename = entity.grabURI( compShaderFilename, root );
				graphic.material.attachShader(compShaderFilename, uf::renderer::enums::Shader::COMPUTE, "culling");
			}
			graphic.descriptor.inputs.dispatch = { graphic.descriptor.inputs.indirect.count, 1, 1 };

			auto& shader = graphic.material.getShader("compute", "culling");

			uf::renderer::Buffer* indirect = NULL;
			for ( auto& buffer : graphic.buffers ) if ( !indirect && buffer.usage & uf::renderer::enums::Buffer::INDIRECT ) indirect = &buffer;
			UF_ASSERT( indirect );

			shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.camera );
			shader.buffers.emplace_back().aliasBuffer( *indirect );
			shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.instance );
		}
		// depth only pipeline
		#if UF_USE_VULKAN
		{
			graphic.material.metadata.autoInitializeUniforms = false;
			uf::stl::string fragmentShaderFilename = graph.metadata["shaders"]["vertex"].as<uf::stl::string>("/graph/depth.frag.spv");
			{
				graphic.material.attachShader(vertexShaderFilename, uf::renderer::enums::Shader::VERTEX, "depth");
			}
			{
				fragmentShaderFilename = entity.grabURI( fragmentShaderFilename, root );
				graphic.material.attachShader(fragmentShaderFilename, uf::renderer::enums::Shader::FRAGMENT, "depth");
			}
			{
				uint32_t maxPasses = 6;

				auto& shader = graphic.material.getShader("vertex", "depth");
				uint32_t* specializationConstants = (uint32_t*) (void*) shader.specializationConstants;
				for ( auto pair : shader.metadata.definitions.specializationConstants ) {
					auto& sc = pair.second;
					if ( sc.name == "PASSES" ) sc.value.ui = (specializationConstants[sc.index] = maxPasses);
				}

				uf::renderer::Buffer* indirect = NULL;
				for ( auto& buffer : graphic.buffers ) if ( !indirect && buffer.usage & uf::renderer::enums::Buffer::INDIRECT ) indirect = &buffer;

				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.camera );
			//	shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.drawCommands );
				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.instance );
				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.joint );
			}
			{
				uint32_t maxTextures = graph.textures.size(); // texture2Ds;

				auto& shader = graphic.material.getShader("fragment", "depth");
				uint32_t* specializationConstants = (uint32_t*) (void*) shader.specializationConstants;
				for ( auto pair : shader.metadata.definitions.specializationConstants ) {
					auto& sc = pair.second;
					if ( sc.name == "TEXTURES" ) sc.value.ui = (specializationConstants[sc.index] = maxTextures);
				}
				for ( auto pair : shader.metadata.definitions.textures ) {
					auto& tx = pair.second;
					for ( auto& layout : shader.descriptorSetLayoutBindings ) {
						if ( layout.binding != tx.binding ) continue;
						if ( tx.name == "samplerTextures" ) layout.descriptorCount = maxTextures;
					}
				}

				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.material );
				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.texture );
				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.light );
			}
		}
		// vxgi pipeline
		if ( uf::renderer::settings::experimental::vxgi ) {
		//	graphic.material.metadata.json["shader"]["autoInitializeUniformBuffers"] = false;
			graphic.material.metadata.autoInitializeUniforms = false;
			uf::stl::string vertexShaderFilename = graph.metadata["shaders"]["vertex"].as<uf::stl::string>("/graph/base.vert.spv");
			uf::stl::string geometryShaderFilename = graph.metadata["shaders"]["geometry"].as<uf::stl::string>("/graph/voxelize.geom.spv");
			uf::stl::string fragmentShaderFilename = graph.metadata["shaders"]["fragment"].as<uf::stl::string>("/graph/voxelize.frag.spv");
			if ( uf::renderer::settings::experimental::deferredSampling ) {
			//	fragmentShaderFilename = uf::string::replace( fragmentShaderFilename, "frag", "deferredSampling.frag" );
			}
			{
				fragmentShaderFilename = entity.grabURI( fragmentShaderFilename, root );
				graphic.material.attachShader(fragmentShaderFilename, uf::renderer::enums::Shader::FRAGMENT, "vxgi");
			}
		//	graphic.material.metadata.json["shader"]["autoInitializeUniformBuffers"] = true;
			graphic.material.metadata.autoInitializeUniforms = true;
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
				uint32_t* specializationConstants = (uint32_t*) (void*) shader.specializationConstants;
				for ( auto pair : shader.metadata.definitions.specializationConstants ) {
					auto& sc = pair.second;
					if ( sc.name == "TEXTURES" ) sc.value.ui = (specializationConstants[sc.index] = maxTextures);
					else if ( sc.name == "CASCADES" ) sc.value.ui = (specializationConstants[sc.index] = maxCascades);
				}
				for ( auto pair : shader.metadata.definitions.textures ) {
					auto& tx = pair.second;
					for ( auto& layout : shader.descriptorSetLayoutBindings ) {
						if ( layout.binding != tx.binding ) continue;
						if ( tx.name == "samplerTextures" ) layout.descriptorCount = maxTextures;
						else if ( tx.name == "voxelId" ) layout.descriptorCount = maxCascades;
						else if ( tx.name == "voxelUv" ) layout.descriptorCount = maxCascades;
						else if ( tx.name == "voxelNormal" ) layout.descriptorCount = maxCascades;
						else if ( tx.name == "voxelRadiance" ) layout.descriptorCount = maxCascades;
					}
				}

			/*
				uint32_t* specializationConstants = (uint32_t*) (void*) shader.specializationConstants;
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
			*/
				
				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.material );
				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.texture );
				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.light );
			}
		}
		#endif
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
			for ( auto& i : graph.images ) graphic.material.textures.emplace_back().aliasTexture( uf::graph::storage.texture2Ds.map[i] );
			for ( auto& s : graph.samplers ) graphic.material.samplers.emplace_back( uf::graph::storage.samplers.map[s] );
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
	// copy our local storage to global storage
	{
		for ( auto& name : graph.storage.atlases.keys ) uf::graph::storage.atlases[name] = graph.storage.atlases[name];
		for ( auto& name : graph.storage.images.keys ) uf::graph::storage.images[name] = graph.storage.images[name];
		for ( auto& name : graph.storage.primitives.keys ) uf::graph::storage.primitives[name] = graph.storage.primitives[name];
		for ( auto& name : graph.storage.meshes.keys ) uf::graph::storage.meshes[name] = graph.storage.meshes[name];
		for ( auto& name : graph.storage.joints.keys ) uf::graph::storage.joints[name] = graph.storage.joints[name];
		for ( auto& name : graph.storage.materials.keys ) uf::graph::storage.materials[name] = graph.storage.materials[name];
		for ( auto& name : graph.storage.textures.keys ) uf::graph::storage.textures[name] = graph.storage.textures[name];
		for ( auto& name : graph.storage.samplers.keys ) uf::graph::storage.samplers[name] = graph.storage.samplers[name];
		for ( auto& name : graph.storage.skins.keys ) uf::graph::storage.skins[name] = graph.storage.skins[name];
		for ( auto& name : graph.storage.animations.keys ) uf::graph::storage.animations[name] = graph.storage.animations[name];
		for ( auto& name : graph.storage.entities.keys ) uf::graph::storage.entities[name] = graph.storage.entities[name];
	}
	//
	if ( !graph.root.entity ) graph.root.entity = new uf::Object;

	// process lightmap

	// add atlas

	// setup textures
	uf::graph::storage.texture2Ds.reserve( uf::graph::storage.images.map.size() );
	for ( auto& keyName : graph.images ) {
		auto& image = uf::graph::storage.images[keyName];
		auto& texture = uf::graph::storage.texture2Ds[keyName];
		if ( !texture.generated() ) texture.loadFromImage( image );
	}	

	
	// process nodes
	for ( auto index : graph.root.children ) process( graph, index, *graph.root.entity );

	// remap materials->texture IDs
	for ( auto& name : graph.materials ) {
		auto& material = uf::graph::storage.materials[name];
		auto& keys = uf::graph::storage.textures.keys;
		int32_t* IDs[] = { &material.indexAlbedo, &material.indexNormal, &material.indexEmissive, &material.indexOcclusion, &material.indexMetallicRoughness };
		for ( auto* pointer : IDs ) {
			auto& ID = *pointer;
			if ( !(0 <= ID && ID < graph.materials.size()) ) continue;
			auto it = std::find( keys.begin(), keys.end(), graph.textures[ID] );
			UF_ASSERT( it != keys.end() );
			ID = it - keys.begin();
		}
	}
	// remap textures->images IDs
	for ( auto& name : graph.textures ) {
		auto& texture = uf::graph::storage.textures[name];
		auto& keys = uf::graph::storage.images.keys;
		
		if ( !(0 <= texture.index && texture.index < graph.textures.size()) ) continue;
		auto it = std::find( keys.begin(), keys.end(), graph.images[texture.index] );
		UF_ASSERT( it != keys.end() );
		texture.index = it - keys.begin();
	}
	// remap instance materials
	for ( auto& name : graph.instances ) {
		auto& instance = uf::graph::storage.instances[name];
		
		if ( !(0 <= instance.materialID && instance.materialID < graph.materials.size()) ) continue;
		auto& keys = graph.storage.materials.keys;
		auto it = std::find( keys.begin(), keys.end(), graph.materials[instance.materialID] );
		UF_ASSERT( it != keys.end() );
		instance.materialID = it - keys.begin();
	}
	::newGraphAdded = true;

	// setup combined mesh if requested
	if ( !(graph.metadata["flags"]["SEPARATE"].as<bool>()) ) {
		graph.root.mesh = graph.meshes.size();
		auto keyName = graph.name + "/" + graph.root.name;
		auto& mesh = uf::graph::storage.meshes[graph.meshes.emplace_back(keyName)];
		for ( auto& name : graph.meshes ) {
			if ( name == keyName ) continue;

			auto& m = uf::graph::storage.meshes.map[name];
			mesh.bindIndirect<pod::DrawCommand>();
			mesh.bind<uf::graph::mesh::Skinned, uint32_t>();
			mesh.insert( m );
		}
		// fix up draw command for combined mesh
		{
			auto& attribute = mesh.indirect.attributes.front();
			auto& buffer = mesh.buffers[mesh.isInterleaved(mesh.indirect.interleaved) ? mesh.indirect.interleaved : attribute.buffer];
			pod::DrawCommand* drawCommands = (pod::DrawCommand*) buffer.data();

			size_t totalIndices = 0;
			size_t totalVertices = 0;
			for ( auto i = 0; i < mesh.indirect.count; ++i ) {
				auto& drawCommand = drawCommands[i];
				drawCommand.indexID = totalIndices;
				drawCommand.vertexID = totalVertices;

				totalIndices += drawCommand.indices;
				totalVertices += drawCommand.vertices;
			}
		}
		{
			auto& graphic = graph.root.entity->getComponent<uf::Graphic>();
			graphic.initialize();
			graphic.initializeMesh( mesh );
			graphic.process = true;

			initializeGraphics( graph, *graph.root.entity );
		}
	}
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

		//	if ( metadataLight["shadows"].as<bool>() ) metadataLight["radius"][1] = 0;
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
	auto& transform = entity.getComponent<pod::Transform<>>();
	{
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

	size_t objectID = uf::graph::storage.entities.keys.size();
	uf::graph::storage.entities[std::to_string(objectID)] = &entity;

	if ( 0 <= node.mesh && node.mesh < graph.meshes.size() ) {
		auto model = uf::transform::model( transform );
		auto& mesh = uf::graph::storage.meshes.map[graph.meshes[node.mesh]];
		auto& primitives = uf::graph::storage.primitives.map[graph.primitives[node.mesh]];

		pod::Instance::Bounds bounds;
		// setup instances
		for ( auto i = 0; i < primitives.size(); ++i ) {
			auto& primitive = primitives[i];

			size_t instanceID = uf::graph::storage.instances.keys.size();
			auto& instance = uf::graph::storage.instances[graph.instances.emplace_back(std::to_string(instanceID))];
			instance = primitive.instance;

			instance.model = model;
			instance.objectID = objectID;

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
		if ( (graph.metadata["flags"]["SEPARATE"].as<bool>()) && graph.metadata["flags"]["RENDER"].as<bool>() ) {
			uf::instantiator::bind("RenderBehavior", entity);

			auto& graphic = entity.getComponent<uf::Graphic>();
			graphic.initialize();
			graphic.initializeMesh( mesh );
			graphic.process = true;
			
			initializeGraphics( graph, entity );
		}
		
	#if 1
		if ( !ext::json::isNull( graph.metadata["tags"][node.name] ) ) {
			auto& info = graph.metadata["tags"][node.name];
			if ( ext::json::isObject( info["collision"] ) ) {
				uf::stl::string type = info["collision"]["type"].as<uf::stl::string>();
				float mass = info["collision"]["mass"].as<float>();
				bool dynamic = !info["collision"]["static"].as<bool>();
				bool recenter = info["collision"]["recenter"].as<bool>();

				auto min = uf::matrix::multiply<float>( model, bounds.min, 1.0f );
				auto max = uf::matrix::multiply<float>( model, bounds.max, 1.0f );

				pod::Vector3f center = (max + min) * 0.5f;
				pod::Vector3f corner = (max - min) * 0.5f;
		
			#if UF_USE_BULLET
				if ( type == "mesh" ) {
					auto& collider = ext::bullet::create( entity.as<uf::Object>(), mesh, dynamic );
					if ( recenter ) collider.transform.position = center;
				} else if ( type == "bounding box" ) {
					auto& collider = ext::bullet::create( entity.as<uf::Object>(), corner, mass );
					collider.shared = true;
					if ( recenter ) collider.transform.position = center - transform.position;
				} else if ( type == "capsule" ) {
					float radius = info["collision"]["radius"].as<float>();
					float height = info["collision"]["height"].as<float>();
					auto& collider = ext::bullet::create( entity.as<uf::Object>(), radius, height, mass );
					collider.shared = true;
					if ( recenter ) collider.transform.position = center - transform.position;
				}
			#endif
			}
		}
	#endif
	}
	//UF_MSG_DEBUG( "Loaded " << node.name );
	for ( auto index : node.children ) uf::graph::process( graph, index, entity );
}
void uf::graph::cleanup( pod::Graph& graph ) {
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
		pod::Animation& animation = uf::graph::storage.animations.map[name]; // graph.animations[name];
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
		//UF_MSG_DEBUG( "Initializing... " << uf::string::toString( entity->as<uf::Object>() ) );
		if ( !entity->hasComponent<uf::Graphic>() ) {
			if ( entity->getUid() == 0 ) entity->initialize();
			//UF_MSG_DEBUG( "Initialized " << uf::string::toString( entity->as<uf::Object>() ) );
			return;
		}
		initializeShaders( graph, entity->as<uf::Object>() );

		uf::instantiator::bind( "GraphBehavior", *entity );
		uf::instantiator::unbind( "RenderBehavior", *entity );
		if ( entity->getUid() == 0 ) entity->initialize();
		//UF_MSG_DEBUG( "Initialized " << uf::string::toString( entity->as<uf::Object>() ) );
	});
}
void uf::graph::animate( pod::Graph& graph, const uf::stl::string& name, float speed, bool immediate ) {
	if ( !(graph.metadata["flags"]["SKINNED"].as<bool>()) ) return;
//	if ( graph.animations.count( name ) > 0 ) {
	if ( uf::graph::storage.animations.map.count( name ) > 0 ) {
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
	// update our instances
	for ( auto& name : uf::graph::storage.instances.keys ) {
		auto& instance = uf::graph::storage.instances[name];
		auto& entity = *uf::graph::storage.entities[std::to_string(instance.objectID)];
		auto& transform = entity.getComponent<pod::Transform<>>();
		instance.model = uf::transform::model( transform );
	}

	// no skins
	if ( !(graph.metadata["flags"]["SKINNED"].as<bool>()) ) return;
	if ( graph.sequence.empty() ) goto UPDATE;
	if ( graph.settings.animations.override.a >= 0 ) goto OVERRIDE;
	{
		uf::stl::string name = graph.sequence.front();
		pod::Animation* animation = &uf::graph::storage.animations.map[name]; // &graph.animations[name];
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
				animation = &uf::graph::storage.animations.map[name]; // &graph.animations[name];
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

void uf::graph::initialize() {
	uf::graph::storage.buffers.camera.initialize( (const void*) nullptr, sizeof(pod::Camera::Viewports), uf::renderer::enums::Buffer::UNIFORM );
	uf::graph::storage.buffers.drawCommands.initialize( (const void*) nullptr, sizeof(pod::DrawCommand), uf::renderer::enums::Buffer::STORAGE );
	uf::graph::storage.buffers.instance.initialize( (const void*) nullptr, sizeof(pod::Instance) * 1024, uf::renderer::enums::Buffer::STORAGE );
	uf::graph::storage.buffers.joint.initialize( (const void*) nullptr, sizeof(pod::Matrix4f) * 1024, uf::renderer::enums::Buffer::STORAGE );
	uf::graph::storage.buffers.material.initialize( (const void*) nullptr, sizeof(pod::Material) * 1024, uf::renderer::enums::Buffer::STORAGE );
	uf::graph::storage.buffers.texture.initialize( (const void*) nullptr, sizeof(pod::Texture) * 1024, uf::renderer::enums::Buffer::STORAGE );
	uf::graph::storage.buffers.light.initialize( (const void*) nullptr, sizeof(pod::Light) * 1024, uf::renderer::enums::Buffer::STORAGE );
}
void uf::graph::tick() {
	uf::stl::vector<pod::Instance> instances; instances.reserve(uf::graph::storage.instances.map.size());
	uf::stl::vector<pod::Matrix4f> joints; joints.reserve(uf::graph::storage.joints.map.size());

	for ( auto key : uf::graph::storage.instances.keys ) {
		auto& instance = instances.emplace_back( uf::graph::storage.instances.map[key] );
	//	instance.bounds.min = uf::matrix::multiply<float>( instance.model, instance.bounds.min, 1.0f );
	//	instance.bounds.max = uf::matrix::multiply<float>( instance.model, instance.bounds.max, 1.0f );
	/*
		pod::Vector3f min = uf::matrix::multiply<float>( instance.model, instance.bounds.min, 1.0f );
		pod::Vector3f max = uf::matrix::multiply<float>( instance.model, instance.bounds.max, 1.0f );

		instance.bounds.min = (max + min) * 0.5f; // center
		instance.bounds.max = (max - min) * 0.5f; // extent
	*/
	}
	for ( auto key : uf::graph::storage.joints.keys ) joints.emplace_back( uf::graph::storage.joints.map[key] );

	uf::graph::storage.buffers.instance.update( (const void*) instances.data(), instances.size() * sizeof(pod::Instance) );
	uf::graph::storage.buffers.joint.update( (const void*) joints.data(), joints.size() * sizeof(pod::Matrix4f) );

	if ( ::newGraphAdded ) {
		uf::stl::vector<pod::DrawCommand> drawCommands; drawCommands.reserve(uf::graph::storage.drawCommands.map.size());
		uf::stl::vector<pod::Material> materials; materials.reserve(uf::graph::storage.materials.map.size());
		uf::stl::vector<pod::Texture> textures; textures.reserve(uf::graph::storage.textures.map.size());

		for ( auto key : uf::graph::storage.drawCommands.keys ) drawCommands.insert( drawCommands.end(), uf::graph::storage.drawCommands.map[key].begin(), uf::graph::storage.drawCommands.map[key].end() );
		for ( auto key : uf::graph::storage.materials.keys ) materials.emplace_back( uf::graph::storage.materials.map[key] );
		for ( auto key : uf::graph::storage.textures.keys ) textures.emplace_back( uf::graph::storage.textures.map[key] );

		uf::graph::storage.buffers.drawCommands.update( (const void*) drawCommands.data(), drawCommands.size() * sizeof(pod::DrawCommand) );
		uf::graph::storage.buffers.material.update( (const void*) materials.data(), materials.size() * sizeof(pod::Material) );
		uf::graph::storage.buffers.texture.update( (const void*) textures.data(), textures.size() * sizeof(pod::Texture) );
		::newGraphAdded = false;
	}
}
void uf::graph::render() {
	auto& scene = uf::scene::getCurrentScene();
	auto& controller = scene.getController();
	auto& camera = controller.getComponent<uf::Camera>();
	uf::graph::storage.buffers.camera.update( (const void*) &camera.data().viewport, sizeof(pod::Camera::Viewports) );
}
void uf::graph::destroy() {
	for ( auto pair : uf::graph::storage.texture2Ds.map ) pair.second.destroy();
	for ( auto& t : uf::graph::storage.shadow2Ds ) t.destroy();
	for ( auto& t : uf::graph::storage.shadowCubes ) t.destroy();

	for ( auto pair : uf::graph::storage.atlases.map ) pair.second.clear();
	for ( auto pair : uf::graph::storage.images.map ) pair.second.clear();
	for ( auto pair : uf::graph::storage.meshes.map ) pair.second.destroy();
}
namespace {
	uf::Image decodeImage( ext::json::Value& json, pod::Graph& graph ) {
		uf::Image image;

		if ( json["filename"].is<uf::stl::string>() ) {
			const uf::stl::string directory = uf::io::directory( graph.name );
			const uf::stl::string filename = uf::io::filename( json["filename"].as<uf::stl::string>() );
			const uf::stl::string name = directory + "/" + filename;
			image.open(name, false);
		} else {
			auto size = uf::vector::decode( json["size"], pod::Vector2ui{} );
			size_t bpp = json["bpp"].as<size_t>();
			size_t channels = json["channels"].as<size_t>();
			auto pixels = uf::base64::decode( json["data"].as<uf::stl::string>() );
			image.loadFromBuffer( &pixels[0], size, bpp, channels, true );
		}
		return image;
	}

	pod::Texture decodeTexture( ext::json::Value& json, pod::Graph& graph ) {
		pod::Texture texture;

		texture.index = json["index"].as(texture.index);
		texture.sampler = json["sampler"].as(texture.sampler);
		texture.remap = json["remap"].as(texture.remap);
		texture.blend = json["blend"].as(texture.blend);
		texture.lerp = uf::vector::decode( json["lerp"], pod::Vector4f{} );
		
		return texture;
	}

	uf::renderer::Sampler decodeSampler( ext::json::Value& json, pod::Graph& graph ) {
		uf::renderer::Sampler sampler;

		sampler.descriptor.filter.min = (uf::renderer::enums::Filter::type_t) json["min"].as<size_t>();
		sampler.descriptor.filter.mag = (uf::renderer::enums::Filter::type_t) json["mag"].as<size_t>();
		sampler.descriptor.addressMode.u = (uf::renderer::enums::AddressMode::type_t) json["u"].as<size_t>();
		sampler.descriptor.addressMode.v = (uf::renderer::enums::AddressMode::type_t) json["v"].as<size_t>();
		sampler.descriptor.addressMode.w = sampler.descriptor.addressMode.v;
		
		return sampler;
	}

	pod::Material decodeMaterial( ext::json::Value& json, pod::Graph& graph ) {
		pod::Material material;

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
		
		return material;
	}

	pod::Light decodeLight( ext::json::Value& json, pod::Graph& graph ) {
		pod::Light light;
		light.color = uf::vector::decode( json["color"], light.color );
		light.intensity = json["intensity"].as(light.intensity);
		light.range = json["range"].as(light.range);
		return light;
	}

	pod::Animation decodeAnimation( ext::json::Value& json, pod::Graph& graph ) {
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

	pod::Skin decodeSkin( ext::json::Value& json, pod::Graph& graph ) {
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

	pod::Instance decodeInstance( ext::json::Value& json, pod::Graph& graph ) {
		pod::Instance instance;
		instance.model = uf::matrix::decode( json["model"], instance.model );
		instance.color = uf::vector::decode( json["color"], instance.color );
		instance.materialID = json["materialID"].as( instance.materialID );
		instance.primitiveID = json["primitiveID"].as( instance.primitiveID );
		instance.meshID = json["meshID"].as( instance.meshID );
		instance.objectID = json["objectID"].as( instance.objectID );
		instance.bounds.min = uf::vector::decode( json["bounds"]["min"], instance.bounds.min );
		instance.bounds.max = uf::vector::decode( json["bounds"]["max"], instance.bounds.max );
		return instance;
	}

	pod::DrawCommand decodeDrawCommand( ext::json::Value& json, pod::Graph& graph ) {
		pod::DrawCommand drawCommand;
		drawCommand.indices = json["indices"].as( drawCommand.indices );
		drawCommand.instances = json["instances"].as( drawCommand.instances );
		drawCommand.indexID = json["indexID"].as( drawCommand.indexID );
		drawCommand.vertexID = json["vertexID"].as( drawCommand.vertexID );
		drawCommand.instanceID = json["instanceID"].as( drawCommand.instanceID );
	//	drawCommand.padding1 = json["padding1"].as( drawCommand.padding1 );
	//	drawCommand.padding2 = json["padding2"].as( drawCommand.padding2 );
		drawCommand.vertices = json["vertices"].as( drawCommand.vertices );
		return drawCommand;
	}

	pod::Primitive decodePrimitive( ext::json::Value& json, pod::Graph& graph ) {
		pod::Primitive prim;
		prim.instance = decodeInstance( json["instance"], graph );
		prim.drawCommand = decodeDrawCommand( json["drawCommand"], graph );
		return prim;
	}

	uf::stl::vector<pod::Primitive> decodePrimitives( ext::json::Value& json, pod::Graph& graph ) {
		uf::stl::vector<pod::Primitive> primitives;
		auto name = json["name"].as<uf::stl::string>();
		ext::json::forEach( json["primitives"], [&]( ext::json::Value& value ){
			primitives.emplace_back( decodePrimitive( value, graph ) );
		});
		return primitives;
	}
	uf::stl::vector<pod::DrawCommand> decodeDrawCommands( ext::json::Value& json, pod::Graph& graph ) {
		uf::stl::vector<pod::DrawCommand> drawCommands;
		auto name = json["name"].as<uf::stl::string>();
		ext::json::forEach( json["drawCommands"], [&]( ext::json::Value& value ){
			drawCommands.emplace_back( decodeDrawCommand( value, graph ) );
		});
		return drawCommands;
	}

	uf::Mesh decodeMesh( ext::json::Value& json, pod::Graph& graph ) {
		uf::Mesh mesh;

		mesh.vertex.attributes.reserve( json["inputs"]["vertex"]["attributes"].size() );
		mesh.vertex.count = json["inputs"]["vertex"]["count"].as( mesh.vertex.count );
		mesh.vertex.first = json["inputs"]["vertex"]["first"].as( mesh.vertex.first );
		mesh.vertex.stride = json["inputs"]["vertex"]["stride"].as( mesh.vertex.stride );
		mesh.vertex.offset = json["inputs"]["vertex"]["stride"].as( mesh.vertex.offset );
		mesh.vertex.interleaved = json["inputs"]["vertex"]["interleaved"].as( mesh.vertex.interleaved );
		ext::json::forEach( json["inputs"]["vertex"]["attributes"], [&]( ext::json::Value& value ){
			auto& attribute = mesh.vertex.attributes.emplace_back();

			attribute.descriptor.offset = value["descriptor"]["offset"].as(attribute.descriptor.offset);
			attribute.descriptor.size = value["descriptor"]["size"].as(attribute.descriptor.size);
			attribute.descriptor.format = (uf::renderer::enums::Format::type_t) value["descriptor"]["format"].as<size_t>(attribute.descriptor.format);
			attribute.descriptor.name = value["descriptor"]["name"].as(attribute.descriptor.name);
			attribute.descriptor.type = (uf::renderer::enums::Type::type_t) value["descriptor"]["type"].as(attribute.descriptor.type);
			attribute.descriptor.components = value["descriptor"]["components"].as(attribute.descriptor.components);
			attribute.buffer = value["buffer"].as(attribute.buffer);
			attribute.offset = value["offset"].as(attribute.offset);
			attribute.stride = value["stride"].as(attribute.stride);
		});
		mesh.index.attributes.reserve( json["inputs"]["index"]["attributes"].size() );
		mesh.index.count = json["inputs"]["index"]["count"].as( mesh.index.count );
		mesh.index.first = json["inputs"]["index"]["first"].as( mesh.index.first );
		mesh.index.stride = json["inputs"]["index"]["stride"].as( mesh.index.stride );
		mesh.index.offset = json["inputs"]["index"]["stride"].as( mesh.index.offset );
		mesh.index.interleaved = json["inputs"]["index"]["interleaved"].as( mesh.index.interleaved );
		ext::json::forEach( json["inputs"]["index"]["attributes"], [&]( ext::json::Value& value ){
			auto& attribute = mesh.index.attributes.emplace_back();

			attribute.descriptor.offset = value["descriptor"]["offset"].as(attribute.descriptor.offset);
			attribute.descriptor.size = value["descriptor"]["size"].as(attribute.descriptor.size);
			attribute.descriptor.format = (uf::renderer::enums::Format::type_t) value["descriptor"]["format"].as<size_t>(attribute.descriptor.format);
			attribute.descriptor.name = value["descriptor"]["name"].as(attribute.descriptor.name);
			attribute.descriptor.type = (uf::renderer::enums::Type::type_t) value["descriptor"]["type"].as(attribute.descriptor.type);
			attribute.descriptor.components = value["descriptor"]["components"].as(attribute.descriptor.components);
			attribute.buffer = value["buffer"].as(attribute.buffer);
			attribute.offset = value["offset"].as(attribute.offset);
			attribute.stride = value["stride"].as(attribute.stride);
		});
		mesh.instance.attributes.reserve( json["inputs"]["instance"]["attributes"].size() );
		mesh.instance.count = json["inputs"]["instance"]["count"].as( mesh.instance.count );
		mesh.instance.first = json["inputs"]["instance"]["first"].as( mesh.instance.first );
		mesh.instance.stride = json["inputs"]["instance"]["stride"].as( mesh.instance.stride );
		mesh.instance.offset = json["inputs"]["instance"]["stride"].as( mesh.instance.offset );
		mesh.instance.interleaved = json["inputs"]["instance"]["interleaved"].as( mesh.instance.interleaved );
		ext::json::forEach( json["inputs"]["instance"]["attributes"], [&]( ext::json::Value& value ){
			auto& attribute = mesh.instance.attributes.emplace_back();

			attribute.descriptor.offset = value["descriptor"]["offset"].as(attribute.descriptor.offset);
			attribute.descriptor.size = value["descriptor"]["size"].as(attribute.descriptor.size);
			attribute.descriptor.format = (uf::renderer::enums::Format::type_t) value["descriptor"]["format"].as<size_t>(attribute.descriptor.format);
			attribute.descriptor.name = value["descriptor"]["name"].as(attribute.descriptor.name);
			attribute.descriptor.type = (uf::renderer::enums::Type::type_t) value["descriptor"]["type"].as(attribute.descriptor.type);
			attribute.descriptor.components = value["descriptor"]["components"].as(attribute.descriptor.components);
			attribute.buffer = value["buffer"].as(attribute.buffer);
			attribute.offset = value["offset"].as(attribute.offset);
			attribute.stride = value["stride"].as(attribute.stride);
		});
		mesh.indirect.attributes.reserve( json["inputs"]["indirect"]["attributes"].size() );
		mesh.indirect.count = json["inputs"]["indirect"]["count"].as( mesh.indirect.count );
		mesh.indirect.first = json["inputs"]["indirect"]["first"].as( mesh.indirect.first );
		mesh.indirect.stride = json["inputs"]["indirect"]["stride"].as( mesh.indirect.stride );
		mesh.indirect.offset = json["inputs"]["indirect"]["stride"].as( mesh.indirect.offset );
		mesh.indirect.interleaved = json["inputs"]["indirect"]["interleaved"].as( mesh.indirect.interleaved );
		ext::json::forEach( json["inputs"]["indirect"]["attributes"], [&]( ext::json::Value& value ){
			auto& attribute = mesh.indirect.attributes.emplace_back();

			attribute.descriptor.offset = value["descriptor"]["offset"].as(attribute.descriptor.offset);
			attribute.descriptor.size = value["descriptor"]["size"].as(attribute.descriptor.size);
			attribute.descriptor.format = (uf::renderer::enums::Format::type_t) value["descriptor"]["format"].as<size_t>(attribute.descriptor.format);
			attribute.descriptor.name = value["descriptor"]["name"].as(attribute.descriptor.name);
			attribute.descriptor.type = (uf::renderer::enums::Type::type_t) value["descriptor"]["type"].as(attribute.descriptor.type);
			attribute.descriptor.components = value["descriptor"]["components"].as(attribute.descriptor.components);
			attribute.buffer = value["buffer"].as(attribute.buffer);
			attribute.offset = value["offset"].as(attribute.offset);
			attribute.stride = value["stride"].as(attribute.stride);
		});

		mesh.buffers.reserve( json["buffers"].size() );
		ext::json::forEach( json["buffers"], [&]( ext::json::Value& value ){
			const uf::stl::string filename = value.as<uf::stl::string>();
			const uf::stl::string directory = uf::io::directory( graph.name );
			auto& buffer = mesh.buffers.emplace_back(uf::io::readAsBuffer( directory + "/" + filename ));
		});
		mesh.updateDescriptor();
		return mesh;
	}

	pod::Node decodeNode( ext::json::Value& json, pod::Graph& graph ) {
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
		// load images
		UF_DEBUG_TIMER_MULTITRACE("Reading instances...");
		graph.instances.reserve( serializer["instances"].size() );
		ext::json::forEach( serializer["instances"], [&]( ext::json::Value& value ){
			auto name = value["name"].as<uf::stl::string>();
			graph.storage.instances[name] = decodeInstance( value, graph );
			graph.instances.emplace_back(name);
		});
	});
	jobs.emplace_back([&]{
		// load images
		UF_DEBUG_TIMER_MULTITRACE("Reading primitives...");
		graph.primitives.reserve( serializer["primitives"].size() );
		ext::json::forEach( serializer["primitives"], [&]( ext::json::Value& value ){
			auto name = value["name"].as<uf::stl::string>();
			graph.storage.primitives[name] = decodePrimitives( value, graph );
			graph.primitives.emplace_back(name);
		});
	});
	jobs.emplace_back([&]{
		// load images
		UF_DEBUG_TIMER_MULTITRACE("Reading drawCommands...");
		graph.drawCommands.reserve( serializer["drawCommands"].size() );
		ext::json::forEach( serializer["drawCommands"], [&]( ext::json::Value& value ){
			auto name = value["name"].as<uf::stl::string>();
			graph.storage.drawCommands[name] = decodeDrawCommands( value, graph );
			graph.drawCommands.emplace_back(name);
		});
	});
	jobs.emplace_back([&]{
		// load mesh information
		UF_DEBUG_TIMER_MULTITRACE("Reading meshes..."); 
		graph.meshes.reserve( serializer["meshes"].size() );
		ext::json::forEach( serializer["meshes"], [&]( ext::json::Value& value ){
			auto name = value["name"].as<uf::stl::string>();
			graph.storage.meshes[name] = decodeMesh( value, graph );
			graph.meshes.emplace_back(name);
		});
	});
	jobs.emplace_back([&]{
		// load images
		UF_DEBUG_TIMER_MULTITRACE("Reading images...");
		graph.images.reserve( serializer["images"].size() );
		ext::json::forEach( serializer["images"], [&]( ext::json::Value& value ){
			auto name = value["name"].as<uf::stl::string>();
			graph.storage.images[name] = decodeImage( value, graph );
			graph.images.emplace_back(name);
		});
	#if 0 && !UF_ENV_DREAMCAST
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
	#endif
	});
	jobs.emplace_back([&]{
		// load texture information
		UF_DEBUG_TIMER_MULTITRACE("Reading texture information...");
		graph.textures.reserve( serializer["textures"].size() );
		ext::json::forEach( serializer["textures"], [&]( ext::json::Value& value ){
			auto name = value["name"].as<uf::stl::string>();
			graph.storage.textures[name] = decodeTexture( value, graph );
			graph.textures.emplace_back(name);
		});
	});
	jobs.emplace_back([&]{
		// load sampler information
		UF_DEBUG_TIMER_MULTITRACE("Reading sampler information...");
		graph.samplers.reserve( serializer["samplers"].size() );
		ext::json::forEach( serializer["samplers"], [&]( ext::json::Value& value ){
			auto name = value["name"].as<uf::stl::string>();
			graph.storage.samplers[name] = decodeSampler( value, graph );
			graph.samplers.emplace_back(name);
		});
	});
	jobs.emplace_back([&]{
		// load material information
		UF_DEBUG_TIMER_MULTITRACE("Reading material information...");
		graph.materials.reserve( serializer["materials"].size() );
		ext::json::forEach( serializer["materials"], [&]( ext::json::Value& value ){
			auto name = value["name"].as<uf::stl::string>();
			graph.storage.materials[name] = decodeMaterial( value, graph );
			graph.materials.emplace_back(name);
		});
	});
	jobs.emplace_back([&]{
		// load light information
		UF_DEBUG_TIMER_MULTITRACE("Reading lighting information...");
		graph.lights.reserve( serializer["lights"].size() );
		ext::json::forEach( serializer["lights"], [&]( ext::json::Value& value ){
			auto name = value["name"].as<uf::stl::string>();
			graph.lights[name] = decodeLight( value, graph );
		});
	});
	jobs.emplace_back([&]{
		// load animation information
		UF_DEBUG_TIMER_MULTITRACE("Reading animation information...");
		graph.storage.animations.map.reserve( serializer["animations"].size() );
		ext::json::forEach( serializer["animations"], [&]( ext::json::Value& value ){
			auto name = value["name"].as<uf::stl::string>();
			if ( value["filename"].is<uf::stl::string>() ) {
				uf::Serializer json;
				json.readFromFile( directory + "/" + value["filename"].as<uf::stl::string>() );
				graph.storage.animations[name] = decodeAnimation( json, graph );
			} else {
				graph.storage.animations[name] = decodeAnimation( value, graph );
			}
			graph.animations.emplace_back(name);
		});
	});
	jobs.emplace_back([&]{
		// load skin information
		UF_DEBUG_TIMER_MULTITRACE("Reading skinning information...");
		graph.skins.reserve( serializer["skins"].size() );
		ext::json::forEach( serializer["skins"], [&]( ext::json::Value& value ){
			auto name = value["name"].as<uf::stl::string>();
			graph.storage.skins[name] = decodeSkin( value, graph );
			graph.skins.emplace_back(name);
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
	uf::Serializer encode( const pod::Instance& instance, const EncodingSettings& settings ) {
		uf::Serializer json;
		json["model"] = uf::matrix::encode( instance.model, settings );
		json["color"] = uf::vector::encode( instance.color, settings );
		json["materialID"] = instance.materialID;
		json["primitiveID"] = instance.primitiveID;
		json["meshID"] = instance.meshID;
		json["objectID"] = instance.objectID;

		json["bounds"]["min"] = uf::vector::encode( instance.bounds.min, settings );
		json["bounds"]["max"] = uf::vector::encode( instance.bounds.max, settings );

		return json;
	}
	uf::Serializer encode( const pod::DrawCommand& drawCommand, const EncodingSettings& settings ) {
		uf::Serializer json;
		json["indices"] = drawCommand.indices;
		json["instances"] = drawCommand.instances;
		json["indexID"] = drawCommand.indexID;
		json["vertexID"] = drawCommand.vertexID;
		json["instanceID"] = drawCommand.instanceID;
	//	json["padding1"] = drawCommand.padding1;
	//	json["padding2"] = drawCommand.padding2;
		json["vertices"] = drawCommand.vertices;
		return json;
	}
	uf::Serializer encode( const pod::Primitive& primitive, const EncodingSettings& settings ) {
		uf::Serializer json;
		json["drawCommand"] = encode( primitive.drawCommand, settings );
		json["instance"] = encode( primitive.instance, settings );
		return json;
	}
	uf::Serializer encode( const uf::Mesh& mesh, const EncodingSettings& settings ) {
		uf::Serializer json;

		json["inputs"]["vertex"]["count"] = mesh.vertex.count;
		json["inputs"]["vertex"]["first"] = mesh.vertex.first;
		json["inputs"]["vertex"]["stride"] = mesh.vertex.stride;
		json["inputs"]["vertex"]["offset"] = mesh.vertex.offset;
		json["inputs"]["vertex"]["interleaved"] = mesh.vertex.interleaved;
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
			a["offset"] = attribute.offset;
			a["stride"] = attribute.stride;
		}

		json["inputs"]["index"]["count"] = mesh.index.count;
		json["inputs"]["index"]["first"] = mesh.index.first;
		json["inputs"]["index"]["stride"] = mesh.index.stride;
		json["inputs"]["index"]["offset"] = mesh.index.offset;
		json["inputs"]["index"]["interleaved"] = mesh.index.interleaved;
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
			a["offset"] = attribute.offset;
			a["stride"] = attribute.stride;
		}

		json["inputs"]["instance"]["count"] = mesh.instance.count;
		json["inputs"]["instance"]["first"] = mesh.instance.first;
		json["inputs"]["instance"]["stride"] = mesh.instance.stride;
		json["inputs"]["instance"]["offset"] = mesh.instance.offset;
		json["inputs"]["instance"]["interleaved"] = mesh.instance.interleaved;
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
			a["offset"] = attribute.offset;
			a["stride"] = attribute.stride;
		}

		json["inputs"]["indirect"]["count"] = mesh.indirect.count;
		json["inputs"]["indirect"]["first"] = mesh.indirect.first;
		json["inputs"]["indirect"]["stride"] = mesh.indirect.stride;
		json["inputs"]["indirect"]["offset"] = mesh.indirect.offset;
		json["inputs"]["indirect"]["interleaved"] = mesh.indirect.interleaved;
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
			a["offset"] = attribute.offset;
			a["stride"] = attribute.stride;
		}

		ext::json::reserve( json["buffers"], mesh.buffers.size() );
		for ( auto i = 0; i < mesh.buffers.size(); ++i ) {
			const uf::stl::string filename = settings.filename + ".buffer." + std::to_string(i) + "." + ( settings.compress ? "gz" : "bin" );
			uf::io::write( filename, mesh.buffers[i] );
			json["buffers"].emplace_back(uf::io::filename( filename ));
		}
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
	if ( !true && settings.unwrap ) {
		pod::Graph& g = const_cast<pod::Graph&>(graph);
		auto size = ext::xatlas::unwrap( g );
		serializer["wrapped"] = uf::vector::encode( size );
	}

	pod::Thread::container_t jobs;
	jobs.emplace_back([&]{
		ext::json::reserve( serializer["instances"], graph.instances.size() );
		for ( size_t i = 0; i < graph.instances.size(); ++i ) {
			auto& name = graph.instances[i];
			auto& instance = graph.storage.instances.map.at(name);
			uf::Serializer json = encode( instance, settings );
			json["name"] = name;
			serializer["instances"].emplace_back( json );
		}
	});
	jobs.emplace_back([&]{
		ext::json::reserve( serializer["primitives"], graph.primitives.size() );
		for ( size_t i = 0; i < graph.primitives.size(); ++i ) {
			auto& name = graph.primitives[i];
			auto& primitives = graph.storage.primitives.map.at(name);
			auto& json = serializer["primitives"].emplace_back();
			json["name"] = name;
		//	ext::json::reserve( json["primitives"], primitives.size() );
			for ( auto& primitive : primitives ) {
				json["primitives"].emplace_back( encode( primitive, settings ) );
			}
		}
	});
	jobs.emplace_back([&]{
		ext::json::reserve( serializer["drawCommands"], graph.drawCommands.size() );
		for ( size_t i = 0; i < graph.drawCommands.size(); ++i ) {
			auto& name = graph.drawCommands[i];
			auto& drawCommands = graph.storage.drawCommands.map.at(name);
			auto& json = serializer["drawCommands"].emplace_back();
			json["name"] = name;
		//	ext::json::reserve( json["drawCommands"], drawCommands.size() );
			for ( auto& drawCommand : drawCommands ) {
				json["drawCommands"].emplace_back( encode( drawCommand, settings ) );
			}
		}
	});
	jobs.emplace_back([&]{
		// store mesh information
		ext::json::reserve( serializer["meshes"], graph.meshes.size() );
		if ( !settings.combined ) {
			::EncodingSettings s = settings;
			for ( size_t i = 0; i < graph.meshes.size(); ++i ) {
				auto& name = graph.meshes[i];
				auto& mesh = graph.storage.meshes.map.at(name);
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
				auto& mesh = graph.storage.meshes.map.at(name);
				auto json = encode(mesh, settings);
				json["name"] = name;
				serializer["meshes"].emplace_back(json);
			}
		}
	});
#if 0
	jobs.emplace_back([&]{
		if ( uf::graphs::storage.atlases[graph.atlas].generated() ) {
			auto atlasName = filename + "/" + "atlas";
			auto& atlas = graph.storage.atlases[atlasName];
			auto& image = atlas.getAtlas();
			if ( !settings.combined ) {
				image.save(directory + "/atlas.png");
				serializer["atlas"] = "atlas.png";
			} else {
				serializer["atlas"] = encode(image, settings);
			}
		}
	});
#endif
	jobs.emplace_back([&]{
		ext::json::reserve( serializer["images"], graph.images.size() );
		if ( !settings.combined ) {
			for ( size_t i = 0; i < graph.images.size(); ++i ) {
				auto& name = graph.images[i];
				auto& image = graph.storage.images.map.at(name);
				uf::stl::string f = "image."+std::to_string(i)+".png";

				image.save(directory + "/" + f);
				uf::Serializer json;
				json["name"] = name;
				json["filename"] = f;
				serializer["images"].emplace_back( json );
			}
		} else {
			for ( auto& name : graph.images ) {
				auto& image = graph.storage.images.map.at(name);
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
			auto& texture = graph.storage.textures.map.at(name);
			auto json = encode(texture, settings);
			json["name"] = name;
			serializer["textures"].emplace_back(json);
		}
	});
	jobs.emplace_back([&]{
		// store sampler information
		ext::json::reserve( serializer["samplers"], graph.samplers.size() );
		for ( auto& name : graph.samplers ) {
			auto& sampler = graph.storage.samplers.map.at(name);
			auto json = encode(sampler, settings);
			json["name"] = name;
			serializer["samplers"].emplace_back(json);
		}
	});
	jobs.emplace_back([&]{
		// store material information
		ext::json::reserve( serializer["materials"], graph.materials.size() );
		for ( auto& name : graph.materials ) {
			auto& material = graph.storage.materials.map.at(name);
			auto json = encode(material, settings);
			json["name"] = name;
			serializer["materials"].emplace_back(json);
		}
	});
	jobs.emplace_back([&]{
		// store light information
		ext::json::reserve( serializer["lights"], graph.lights.size() );
		for ( auto pair : graph.lights ) {
			auto& name = pair.first;
			auto& light = pair.second;
			auto json = encode(light, settings);
			json["name"] = name;
			serializer["lights"].emplace_back(json);
		}
	});
	jobs.emplace_back([&]{
		// store animation information
		ext::json::reserve( serializer["animations"], graph.animations.size() );
		if ( !settings.combined ) {
			for ( auto& name : graph.animations ) {
				uf::stl::string f = "animation."+name+".json";
				auto& animation = graph.storage.animations.map.at(name);
				encode(animation, settings).writeToFile(directory+"/animation."+name+".json", settings);
				serializer["animations"].emplace_back("animation."+name+".json" + (settings.compress ? ".gz" : ""));
			}
		} else {
			for ( auto& name : graph.animations ) {
				auto& animation = graph.storage.animations.map.at(name);
				serializer["animations"][name] = encode(animation, settings);
			}
		}
	});
	jobs.emplace_back([&]{
		// store skin information
		ext::json::reserve( serializer["skins"], graph.skins.size() );
		for ( auto& name : graph.skins ) {
			auto& skin = graph.storage.skins.map.at(name);
			serializer["skins"].emplace_back( encode(skin, settings) );
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