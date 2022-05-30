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

namespace {
	bool newGraphAdded = true;
}

pod::Graph::Storage uf::graph::storage;

UF_VERTEX_DESCRIPTOR(uf::graph::mesh::Base,
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base, R32G32_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base, R8G8B8A8_UNORM, color)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base, R32G32_SFLOAT, st)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base, R32G32B32_SFLOAT, normal)
);
UF_VERTEX_DESCRIPTOR(uf::graph::mesh::Skinned,
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R32G32_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R8G8B8A8_UNORM, color)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R32G32_SFLOAT, st)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R32G32B32_SFLOAT, normal)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R32G32B32_SFLOAT, tangent)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R16G16B16A16_UINT, joints)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R32G32B32A32_SFLOAT, weights)
);

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

void uf::graph::initializeGraphics( pod::Graph& graph, uf::Object& entity ) {
		auto& scene = uf::scene::getCurrentScene();
		auto& sceneTextures = scene.getComponent<pod::SceneTextures>();
		auto& sceneMetadataJson = scene.getComponent<uf::Serializer>();
		
		auto& graphic = entity.getComponent<uf::Graphic>();

		graphic.device = &uf::renderer::device;
		graphic.material.device = &uf::renderer::device;
		graphic.descriptor.frontFace = uf::renderer::enums::Face::CCW;
		graphic.descriptor.cullMode = !(graph.metadata["flags"]["INVERT"].as<bool>()) ? uf::renderer::enums::CullMode::BACK : uf::renderer::enums::CullMode::FRONT;
		if ( graph.metadata["cull mode"].is<uf::stl::string>() ) {
			const auto mode = graph.metadata["cull mode"].as<uf::stl::string>();
			if ( mode == "back" ) graphic.descriptor.cullMode = uf::renderer::enums::CullMode::BACK;
			else if ( mode == "front" ) graphic.descriptor.cullMode = uf::renderer::enums::CullMode::FRONT;
			else if ( mode == "none" ) graphic.descriptor.cullMode = uf::renderer::enums::CullMode::NONE;
			else if ( mode == "both" ) graphic.descriptor.cullMode = uf::renderer::enums::CullMode::BOTH;
		}
		{
		//	for ( auto& s : graph.samplers ) graphic.material.samplers.emplace_back( uf::graph::storage.samplers.map[s] );
		//	for ( auto pair : uf::graph::storage.samplers.map ) graphic.material.samplers.emplace_back( pair.second );
		//	for ( auto& key : uf::graph::storage.samplers.keys ) graphic.material.samplers.emplace_back( uf::graph::storage.samplers.map[key] );

		//	for ( auto& i : graph.images ) graphic.material.textures.emplace_back().aliasTexture( uf::graph::storage.texture2Ds.map[i] );
		//	for ( auto pair : uf::graph::storage.texture2Ds.map ) graphic.material.textures.emplace_back().aliasTexture( pair.second );
		#if 1
			for ( auto& key : uf::graph::storage.texture2Ds.keys ) graphic.material.textures.emplace_back().aliasTexture( uf::graph::storage.texture2Ds.map[key] );
		#else
			uf::stl::map<size_t, uf::stl::string> texture2DOrderedMap; // texture2DOrderedMap.reserve( uf::graph::storage.texture2Ds.keys.size() );
			for ( auto key : uf::graph::storage.texture2Ds.keys ) {
				texture2DOrderedMap[uf::graph::storage.texture2Ds.indices[key]] = key;
			}
			for ( auto pair : texture2DOrderedMap ) graphic.material.textures.emplace_back().aliasTexture( uf::graph::storage.texture2Ds.map[pair.second] );
		#endif

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
		
		uf::stl::string root = uf::io::directory( graph.name );
		size_t texture2Ds = 0;
		size_t texture3Ds = 0;
		for ( auto& texture : graphic.material.textures ) {
			if ( texture.width > 1 && texture.height > 1 && texture.depth == 1 && texture.layers == 1 ) ++texture2Ds;
			else if ( texture.width > 1 && texture.height > 1 && texture.depth > 1 && texture.layers == 1 ) ++texture3Ds;
		}

		// standard pipeline
		uf::stl::string vertexShaderFilename = graph.metadata["shaders"]["vertex"].as<uf::stl::string>("/graph/base.vert.spv"); {
			std::pair<bool, uf::stl::string> settings[] = {
				{ graph.metadata["flags"]["SKINNED"].as<bool>(), "skinned.vert" },
				{ !graph.metadata["flags"]["SEPARATE"].as<bool>(), "instanced.vert" },
			};
			FOR_ARRAY(settings) if ( settings[i].first ) vertexShaderFilename = uf::string::replace( vertexShaderFilename, "vert", settings[i].second );
			vertexShaderFilename = entity.grabURI( vertexShaderFilename, root );
		}
		uf::stl::string geometryShaderFilename = graph.metadata["shaders"]["geometry"].as<uf::stl::string>("");
		uf::stl::string fragmentShaderFilename = graph.metadata["shaders"]["fragment"].as<uf::stl::string>("/graph/base.frag.spv"); {
			std::pair<bool, uf::stl::string> settings[] = {
				{ uf::renderer::settings::experimental::deferredSampling, "deferredSampling.frag" },
			};
			FOR_ARRAY(settings) if ( settings[i].first ) fragmentShaderFilename = uf::string::replace( fragmentShaderFilename, "frag", settings[i].second );
			fragmentShaderFilename = entity.grabURI( fragmentShaderFilename, root );
		}
		{
			graphic.material.metadata.autoInitializeUniforms = false;
			graphic.material.attachShader(vertexShaderFilename, uf::renderer::enums::Shader::VERTEX);
			graphic.material.attachShader(fragmentShaderFilename, uf::renderer::enums::Shader::FRAGMENT);
			graphic.material.metadata.autoInitializeUniforms = true;
		}

		uf::renderer::Buffer* indirect = NULL;
		for ( auto& buffer : graphic.buffers ) if ( !indirect && buffer.usage & uf::renderer::enums::Buffer::INDIRECT ) indirect = &buffer;

		{
			auto& shader = graphic.material.getShader("vertex");

			shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.camera );
	//	//	shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.drawCommands );
		#if UF_USE_VULKAN
			shader.buffers.emplace_back().aliasBuffer( *indirect );
		#endif
			shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.instance );
			shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.joint );
		#if UF_USE_VULKAN
			uint32_t maxPasses = 6;
			uint32_t* specializationConstants = (uint32_t*) (void*) shader.specializationConstants;
			for ( auto pair : shader.metadata.definitions.specializationConstants ) {
				auto& sc = pair.second;
				if ( sc.name == "PASSES" ) sc.value.ui = (specializationConstants[sc.index] = maxPasses);
			}
		#endif
		}
		{
			auto& shader = graphic.material.getShader("fragment");

			shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.instance );
			shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.material );
			shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.texture );
			shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.light );
		
		#if UF_USE_VULKAN
			uint32_t maxTextures = graph.textures.size(); // texture2Ds;

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
		#endif
		}
	#if UF_USE_VULKAN
		// culling pipeline
		if ( uf::renderer::settings::experimental::culling ) {
			uf::renderer::Buffer* indirect = NULL;
			for ( auto& buffer : graphic.buffers ) if ( !indirect && buffer.usage & uf::renderer::enums::Buffer::INDIRECT ) indirect = &buffer;
			UF_ASSERT( indirect );
			if ( indirect ) {
				uf::stl::string compShaderFilename = graph.metadata["shaders"]["vertex"].as<uf::stl::string>("/graph/cull.comp.spv");
				{
					graphic.material.metadata.autoInitializeUniforms = false;
					compShaderFilename = entity.grabURI( compShaderFilename, root );
					graphic.material.attachShader(compShaderFilename, uf::renderer::enums::Shader::COMPUTE, "culling");
					graphic.material.metadata.autoInitializeUniforms = true;
				}
				graphic.descriptor.inputs.dispatch = { graphic.descriptor.inputs.indirect.count, 1, 1 };

				auto& shader = graphic.material.getShader("compute", "culling");

				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.camera );
			#if UF_USE_VULKAN
				shader.buffers.emplace_back().aliasBuffer( *indirect );
			#endif
				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.instance );
			}
		}
		if ( geometryShaderFilename != "" && uf::renderer::device.enabledFeatures.geometryShader ) {
			geometryShaderFilename = entity.grabURI( geometryShaderFilename, root );
			graphic.material.attachShader(geometryShaderFilename, uf::renderer::enums::Shader::GEOMETRY);
		}
		// depth only pipeline
		{
			graphic.material.metadata.autoInitializeUniforms = false;
			uf::stl::string fragmentShaderFilename = graph.metadata["shaders"]["vertex"].as<uf::stl::string>("/graph/depth.frag.spv");
			fragmentShaderFilename = entity.grabURI( fragmentShaderFilename, root );
			graphic.material.attachShader(fragmentShaderFilename, uf::renderer::enums::Shader::FRAGMENT, "depth");
			graphic.material.metadata.autoInitializeUniforms = true;
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

				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.instance );
				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.material );
				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.texture );
				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.light );
			}
		}
		// vxgi pipeline
		if ( uf::renderer::settings::experimental::vxgi ) {
			uf::stl::string vertexShaderFilename = graph.metadata["shaders"]["vertex"].as<uf::stl::string>("/graph/base.vert.spv");
			uf::stl::string geometryShaderFilename = graph.metadata["shaders"]["geometry"].as<uf::stl::string>("/graph/voxelize.geom.spv");
			uf::stl::string fragmentShaderFilename = graph.metadata["shaders"]["fragment"].as<uf::stl::string>("/graph/voxelize.frag.spv");

			{
				fragmentShaderFilename = entity.grabURI( fragmentShaderFilename, root );
				graphic.material.metadata.autoInitializeUniforms = false;
				graphic.material.attachShader(fragmentShaderFilename, uf::renderer::enums::Shader::FRAGMENT, "vxgi");
				graphic.material.metadata.autoInitializeUniforms = true;
			}
			if ( geometryShaderFilename != "" && uf::renderer::device.enabledFeatures.geometryShader ) {
				geometryShaderFilename = entity.grabURI( geometryShaderFilename, root );
				graphic.material.attachShader(geometryShaderFilename, uf::renderer::enums::Shader::GEOMETRY, "vxgi");
			}
			{
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
				
				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.instance );
				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.material );
				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.texture );
				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.light );
			}
		}
		// baking pipeline
		if ( graph.metadata["baking"]["enabled"].as<bool>() ) {
			{
				graphic.material.metadata.autoInitializeUniforms = false;
				uf::stl::string vertexShaderFilename = uf::io::resolveURI("/graph/baking/bake.vert.spv");
				uf::stl::string geometryShaderFilename = uf::io::resolveURI("/graph/baking/bake.geom.spv");
				uf::stl::string fragmentShaderFilename = uf::io::resolveURI("/graph/baking/bake.frag.spv");
				graphic.material.attachShader(vertexShaderFilename, uf::renderer::enums::Shader::VERTEX, "baking");
				graphic.material.attachShader(geometryShaderFilename, uf::renderer::enums::Shader::GEOMETRY, "baking");
				graphic.material.attachShader(fragmentShaderFilename, uf::renderer::enums::Shader::FRAGMENT, "baking");
				graphic.material.metadata.autoInitializeUniforms = true;
			}
			{
				uint32_t maxPasses = 6;

				auto& shader = graphic.material.getShader("vertex", "baking");
				uint32_t* specializationConstants = (uint32_t*) (void*) shader.specializationConstants;
				for ( auto pair : shader.metadata.definitions.specializationConstants ) {
					auto& sc = pair.second;
					if ( sc.name == "PASSES" ) sc.value.ui = (specializationConstants[sc.index] = maxPasses);
				}

			//	shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.camera );
			//	shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.drawCommands );
			#if UF_USE_VULKAN
				shader.buffers.emplace_back().aliasBuffer( *indirect );
			#endif
				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.instance );
				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.joint );
			}

			{
				size_t maxTextures = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["textures"]["max"]["2D"].as<size_t>(512);
				size_t maxCubemaps = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["textures"]["max"]["cube"].as<size_t>(128);
				size_t maxTextures3D = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["textures"]["max"]["3D"].as<size_t>(128);

				auto& shader = graphic.material.getShader("fragment", "baking");
				uint32_t* specializationConstants = (uint32_t*) (void*) shader.specializationConstants;
				for ( auto pair : shader.metadata.definitions.specializationConstants ) {
					auto& sc = pair.second;
					if ( sc.name == "TEXTURES" ) sc.value.ui = (specializationConstants[sc.index] = maxTextures);
					else if ( sc.name == "CUBEMAPS" ) sc.value.ui = (specializationConstants[sc.index] = maxCubemaps);
				}
				for ( auto pair : shader.metadata.definitions.textures ) {
					auto& tx = pair.second;
					for ( auto& layout : shader.descriptorSetLayoutBindings ) {
						if ( layout.binding != tx.binding ) continue;
						if ( tx.name == "samplerTextures" ) layout.descriptorCount = maxTextures;
						else if ( tx.name == "samplerCubemaps" ) layout.descriptorCount = maxCubemaps;
					}
				}

				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.instance );
				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.material );
				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.texture );
				shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.light );
			}
		}
	#endif

		uf::instantiator::bind( "GraphBehavior", entity );
		uf::instantiator::unbind( "RenderBehavior", entity );
	}

void uf::graph::process( pod::Graph& graph ) {
	// copy our local storage to global storage

	//
	if ( !graph.root.entity ) graph.root.entity = new uf::Object;

	//
	uf::stl::unordered_map<uf::stl::string, bool> isSrgb;

	// process lightmap
#if UF_USE_OPENGL
	#define UF_GRAPH_DEFAULT_LIGHTMAP "./lightmap.%i.min.dtex"
#else
	#define UF_GRAPH_DEFAULT_LIGHTMAP "./lightmap.%i.png"
#endif
	{
		uf::stl::unordered_map<size_t, uf::stl::string> filenames;
		uf::stl::unordered_map<size_t, size_t> lightmapIDs;
		uint32_t lightmapCount = 0;

		for ( auto& name : graph.instances ) {
			auto& instance = uf::graph::storage.instances[name];
			filenames[instance.auxID] = uf::string::replace(UF_GRAPH_DEFAULT_LIGHTMAP, "%i", std::to_string(instance.auxID));

			lightmapCount = std::max( lightmapCount, instance.auxID );
		}
		for ( auto& name : graph.primitives ) {
			auto& primitives = uf::graph::storage.primitives[name];
			for ( auto& primitive : primitives ) {
				filenames[primitive.instance.auxID] = uf::string::replace(UF_GRAPH_DEFAULT_LIGHTMAP, "%i", std::to_string(primitive.instance.auxID));

				lightmapCount = std::max( lightmapCount, primitive.instance.auxID );
			}
		}
		graph.metadata["baking"]["layers"] = lightmapCount;
		
		if ( graph.metadata["lightmap"].as<bool>() ) for ( auto& pair : filenames ) {
			auto i = pair.first;
			auto f = uf::io::sanitize( pair.second, uf::io::directory( graph.name ) );

			if ( !uf::io::exists( f ) ) {
				UF_MSG_ERROR( "lightmap does not exist: " << f )
				continue;
			}
			

			auto textureID = graph.textures.size();
			auto imageID = graph.images.size();

			auto& texture = /*graph.storage*/uf::graph::storage.textures[graph.textures.emplace_back(f)];
			auto& image = /*graph.storage*/uf::graph::storage.images[graph.images.emplace_back(f)];
			image.open( f, false );

			texture.index = imageID;

			lightmapIDs[i] = textureID;

			graph.metadata["lightmaps"][i] = f;
			graph.metadata["baking"]["enabled"] = false;

			isSrgb[f] = false;
		}
				
		for ( auto& name : graph.instances ) {
			auto& instance = uf::graph::storage.instances[name];
			if ( lightmapIDs.count( instance.auxID ) == 0 ) continue;
			instance.lightmapID = lightmapIDs[instance.auxID];
		}
		for ( auto& name : graph.primitives ) {
			auto& primitives = uf::graph::storage.primitives[name];
			for ( auto& primitive : primitives ) {
				if ( lightmapIDs.count( primitive.instance.auxID ) == 0 ) continue;
				primitive.instance.lightmapID = lightmapIDs[primitive.instance.auxID];
			}
		}
	}
	// add atlas


	// setup textures
	uf::graph::storage.texture2Ds.reserve( uf::graph::storage.images.map.size() );

	// not having this block will cause our images and texture2Ds to be ordered out of sync
	for ( auto& key : graph.images ) {
		uf::graph::storage.images[key];
		uf::graph::storage.texture2Ds[key];
	}

	// figure out what texture is what exactly
	for ( auto& key : graph.materials ) {
		auto& material = uf::graph::storage.materials[key];
		auto ID = material.indexAlbedo;

		if ( !(0 <= ID && ID < graph.textures.size()) ) continue;

		auto texName = graph.textures[ID];
		isSrgb[texName] = true;
	}

	for ( auto& key : graph.images ) {
		auto& image = uf::graph::storage.images[key];
		auto& texture = uf::graph::storage.texture2Ds[key];
		if ( !texture.generated() ) {
		//	bool isLightmap = graph.metadata["lightmapped"].as<uf::stl::string>() == key;
		//	auto filter = graph.metadata["filter"].as<uf::stl::string>() == "NEAREST" && !isLightmap ? uf::renderer::enums::Filter::NEAREST : uf::renderer::enums::Filter::LINEAR;
		//	auto filter = uf::renderer::enums::Filter::LINEAR;

			auto filter = graph.metadata["filter"].as<uf::stl::string>() == "NEAREST" ? uf::renderer::enums::Filter::NEAREST : uf::renderer::enums::Filter::LINEAR;
			texture.sampler.descriptor.filter.min = filter;
			texture.sampler.descriptor.filter.mag = filter;
			texture.srgb = isSrgb.count(key) == 0 ? false : isSrgb[key];

			texture.loadFromImage( image );
		#if UF_ENV_DREAMCAST
			image.clear();
		#endif
		}
	}	

	
	// process nodes
	for ( auto index : graph.root.children ) process( graph, index, *graph.root.entity );

	// remap textures->images IDs
	for ( auto& name : graph.textures ) {
		auto& texture = uf::graph::storage.textures[name];
		auto& keys = uf::graph::storage.images.keys;
		auto& indices = uf::graph::storage.images.indices;

		if ( !(0 <= texture.index && texture.index < graph.images.size()) ) continue;

		auto& needle = graph.images[texture.index];
	#if 1
		texture.index = indices[needle];
	#elif 1
		for ( size_t i = 0; i < keys.size(); ++i ) {
			if ( keys[i] != needle ) continue;
			texture.index = i;
			break;
		}
	#else
		auto it = std::find( keys.begin(), keys.end(), needle );
		UF_ASSERT( it != keys.end() );
		texture.index = it - keys.begin();
	#endif
	}
	// remap materials->texture IDs
	for ( auto& name : graph.materials ) {
		auto& material = uf::graph::storage.materials[name];
		auto& keys = uf::graph::storage.textures.keys;
		auto& indices = uf::graph::storage.textures.indices;
		int32_t* IDs[] = { &material.indexAlbedo, &material.indexNormal, &material.indexEmissive, &material.indexOcclusion, &material.indexMetallicRoughness };
		for ( auto* pointer : IDs ) {
			auto& ID = *pointer;
			if ( !(0 <= ID && ID < graph.textures.size()) ) continue;
			auto& needle = graph.textures[ID];
		#if 1
			ID = indices[needle];
		#elif 1
			for ( size_t i = 0; i < keys.size(); ++i ) {
				if ( keys[i] != needle ) continue;
				ID = i;
				break;
			}
		#else
			if ( !(0 <= ID && ID < graph.textures.size()) ) continue;
			auto it = std::find( keys.begin(), keys.end(), needle );
			UF_ASSERT( it != keys.end() );
			ID = it - keys.begin();
		#endif
		}
	}
	// remap instance variables
	for ( auto& name : graph.instances ) {
		auto& instance = uf::graph::storage.instances[name];
		
		if ( 0 <= instance.materialID && instance.materialID < graph.materials.size() ) {
			auto& keys = /*graph.storage*/uf::graph::storage.materials.keys;
			auto& indices = /*graph.storage*/uf::graph::storage.materials.indices;
			
			if ( !(0 <= instance.materialID && instance.materialID < graph.materials.size()) ) continue;

			auto& needle = graph.materials[instance.materialID];
		#if 1
			instance.materialID = indices[needle];
		#elif 1
			for ( size_t i = 0; i < keys.size(); ++i ) {
				if ( keys[i] != needle ) continue;
				instance.materialID = i;
				break;
			}
		#else
			auto it = std::find( keys.begin(), keys.end(), needle );
			UF_ASSERT( it != keys.end() );
			instance.materialID = it - keys.begin();
		#endif
		}
		if ( 0 <= instance.lightmapID && instance.lightmapID < graph.textures.size() ) {
			auto& keys = /*graph.storage*/uf::graph::storage.textures.keys;
			auto& indices = /*graph.storage*/uf::graph::storage.textures.indices;

			if ( !(0 <= instance.lightmapID && instance.lightmapID < graph.textures.size()) ) continue;

			auto& needle = graph.textures[instance.lightmapID];
		#if 1
			instance.lightmapID = indices[needle];
		#elif 1
			for ( size_t i = 0; i < keys.size(); ++i ) {
				if ( keys[i] != needle ) continue;
				instance.lightmapID = i;
				break;
			}
		#else
			auto it = std::find( keys.begin(), keys.end(), needle );
			UF_ASSERT( it != keys.end() );
			instance.lightmapID = it - keys.begin();
		#endif
		}
	#if 0
		// i genuinely dont remember what this is used for

		if ( 0 <= instance.imageID && instance.imageID < graph.images.size() ) {
			auto& keys = /*graph.storage*/uf::graph::storage.images.keys;
			auto it = std::find( keys.begin(), keys.end(), graph.images[instance.imageID] );
			UF_ASSERT( it != keys.end() );
			instance.imageID = it - keys.begin();
		}
	#endif
		// remap a skinID as an actual jointID
		if ( 0 <= instance.jointID && instance.jointID < graph.skins.size() ) {
			auto& name = graph.skins[instance.jointID];
			instance.jointID = 0;
			for ( auto key : uf::graph::storage.joints.keys ) {
				if ( key == name ) break;
				auto& joints = uf::graph::storage.joints[key];
				instance.jointID += joints.size();
			}
		}
	}

	uf::graph::reload();

	// setup combined mesh if requested
	if ( !(graph.metadata["flags"]["SEPARATE"].as<bool>()) ) {
		graph.root.mesh = graph.meshes.size();
		auto keyName = graph.name + "/" + graph.root.name;
		auto& mesh = uf::graph::storage.meshes[graph.meshes.emplace_back(keyName)];
		mesh.bindIndirect<pod::DrawCommand>();
		mesh.bind<uf::graph::mesh::Skinned, uint32_t>();

		uf::stl::vector<pod::DrawCommand> drawCommands;
		
		size_t counts = 0;
		for ( auto& name : graph.meshes ) {
			if ( name == keyName ) continue;

			auto& m = uf::graph::storage.meshes.map[name];
			m.updateDescriptor();

			mesh.insertVertices( m );
			mesh.insertIndices( m );
			mesh.insertInstances( m );

		//	mesh.insertIndirects( m );
			pod::DrawCommand* drawCommand = (pod::DrawCommand*) m.getBuffer( m.indirect ).data();
			for ( size_t i = 0; i < m.indirect.count; ++i ) drawCommands.emplace_back( drawCommand[i] );
		}

		// fix up draw command for combined mesh
		{
			size_t totalIndices = 0;
			size_t totalVertices = 0;
			for ( auto& drawCommand : drawCommands ) {
				drawCommand.indexID = totalIndices;
				drawCommand.vertexID = totalVertices;

				totalIndices += drawCommand.indices;
				totalVertices += drawCommand.vertices;
			}
			
			mesh.insertIndirects( drawCommands );
		}

		// slice mesh
		{
		//	auto slices = uf::meshgrid::partition( mesh, 1 );
		//	uf::meshgrid::print( slices, mesh.index.count );
		}

		{
			auto& graphic = graph.root.entity->getComponent<uf::Graphic>();
			graphic.initialize();
			graphic.initializeMesh( mesh );
			graphic.process = true;

			uf::graph::initializeGraphics( graph, *graph.root.entity );
		}
	}
}
void uf::graph::process( pod::Graph& graph, int32_t index, uf::Object& parent ) {
	auto& node = graph.nodes[index];
	// ignore pesky light_Orientation nodes
	if ( uf::string::split( node.name, "_" ).back() == "Orientation" ) return;
	// for dreamcast, ignore lights if we're baked
	if ( graph.metadata["lightmapped"].as<bool>() && graph.metadata["lights"]["disable if lightmapped"].as<bool>(true) ) if ( graph.lights.count(node.name) > 0 ) return;


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
	ext::json::Value tag = ext::json::null();
/*
	if ( ext::json::isObject( graph.metadata["tags"][node.name] ) ) {
		tag = graph.metadata["tags"][node.name];
	}
*/
	ext::json::forEach( graph.metadata["tags"], [&]( const uf::stl::string& key, ext::json::Value& value ) {
		if ( uf::string::isRegex( key ) ) {
			if ( uf::string::matches( node.name, key ).empty() ) return;
		} else if ( node.name != key ) return;
		tag = value;
	});
	if ( ext::json::isObject( tag ) ) {
		if ( tag["ignore"].as<bool>() ) return;

		if ( tag["action"].as<uf::stl::string>() == "load" ) {
			if ( tag["filename"].is<uf::stl::string>() ) {
				uf::stl::string filename = uf::io::resolveURI( tag["filename"].as<uf::stl::string>(), graph.metadata["root"].as<uf::stl::string>() );
				entity.load(filename);
			} else if ( ext::json::isObject( tag["payload"] ) ) {
				uf::Serializer json = tag["payload"];
				json["root"] = graph.metadata["root"];
				entity.load(json);
			}
		} else if ( tag["action"].as<uf::stl::string>() == "attach" ) {
			uf::stl::string filename = uf::io::resolveURI( tag["filename"].as<uf::stl::string>(), graph.metadata["root"].as<uf::stl::string>() );
			auto& child = entity.loadChild( filename, false );
			auto& childTransform = child.getComponent<pod::Transform<>>();
			auto flatten = uf::transform::flatten( node.transform );
			if ( !tag["preserve position"].as<bool>() ) childTransform.position = flatten.position;
			if ( !tag["preserve orientation"].as<bool>() ) childTransform.orientation = flatten.orientation;
		}
		if ( tag["static"].is<bool>() ) {
			metadata.system.ignoreGraph = tag["static"].as<bool>();
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
		//	{
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
		if ( tag["transform"]["offset"].as<bool>() ) {
			auto parsed = uf::transform::decode( tag["transform"], pod::Transform<>{} );
			transform.position += parsed.position;
			transform.orientation = uf::quaternion::multiply( transform.orientation, parsed.orientation );
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
			instance.jointID = graph.metadata["flags"]["SKINNED"].as<bool>() ? 0 : -1;

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
			
			uf::graph::initializeGraphics( graph, entity );
		}
		
		{
			auto phyziks = tag["physics"];
			if ( !ext::json::isObject( phyziks ) ) phyziks = metadataJson["system"]["physics"];
			else metadataJson["system"]["physics"] = phyziks;
			
			if ( ext::json::isObject( phyziks ) ) {
				uf::stl::string type = phyziks["type"].as<uf::stl::string>();		

				if ( type == "mesh" ) {
					auto& collider = entity.getComponent<pod::PhysicsState>();
					collider.stats.mass = phyziks["mass"].as(collider.stats.mass);
					collider.stats.friction = phyziks["friction"].as(collider.stats.friction);
					collider.stats.restitution = phyziks["restitution"].as(collider.stats.restitution);
					collider.stats.inertia = uf::vector::decode( phyziks["inertia"], collider.stats.inertia );
					collider.stats.gravity = uf::vector::decode( phyziks["gravity"], collider.stats.gravity );

					uf::physics::impl::create( entity.as<uf::Object>(), mesh, !phyziks["static"].as<bool>(true) );
				} else {
					auto min = uf::matrix::multiply<float>( model, bounds.min, 1.0f );
					auto max = uf::matrix::multiply<float>( model, bounds.max, 1.0f );

					pod::Vector3f center = (max + min) * 0.5f;
					pod::Vector3f corner = uf::vector::abs(max - min) * 0.5f;
				//	corner.x = abs(corner.x);
				//	corner.y = abs(corner.y);
				//	corner.z = abs(corner.z);
					
					metadataJson["system"]["physics"]["center"] = uf::vector::encode( center );
					metadataJson["system"]["physics"]["corner"] = uf::vector::encode( corner );
				}
			}
		}
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
	if ( graph.metadata["baking"]["enabled"].as<bool>() ) {
		auto& metadataJson = graph.root.entity->getComponent<uf::Serializer>();
		metadataJson["baking"] = graph.metadata["baking"];
		metadataJson["baking"]["root"] = uf::io::directory( graph.name );
		uf::instantiator::bind( "BakingBehavior", *graph.root.entity );
	}

	graph.root.entity->initialize();
	graph.root.entity->process([&]( uf::Entity* entity ) {
		if ( entity->getUid() == 0 ) entity->initialize();
	/*
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
	*/
	});

}
void uf::graph::animate( pod::Graph& graph, const uf::stl::string& _name, float speed, bool immediate ) {
	if ( !(graph.metadata["flags"]["SKINNED"].as<bool>()) ) return;
	const uf::stl::string name = /*graph.name + "/" +*/ _name;
//	UF_MSG_DEBUG( graph.name << " " << name );
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
	//	UF_MSG_DEBUG("ANIMATION: " << name << "\t" << animation->cur);
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
//	UF_MSG_DEBUG("OVERRIDED: " << graph.settings.animations.override.a << "\t" << -std::numeric_limits<float>::max());
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
	if ( node.skin >= 0 ) {
		pod::Matrix4f nodeMatrix = uf::graph::matrix( graph, node.index );
		pod::Matrix4f inverseTransform = uf::matrix::inverse( nodeMatrix );
		
		auto& name = graph.skins[node.skin];
		auto& skin = uf::graph::storage.skins[name];
		auto& joints = uf::graph::storage.joints[name];
		joints.resize( skin.joints.size() );
		for ( size_t i = 0; i < skin.joints.size(); ++i ) joints[i] = uf::matrix::identity();

		if ( graph.settings.animations.override.a >= 0 || !graph.sequence.empty() ) {
			for ( size_t i = 0; i < skin.joints.size(); ++i ) {
				joints[i] = inverseTransform * (uf::graph::matrix(graph, skin.joints[i]) * skin.inverseBindMatrices[i]);
			}
		}
	/*
		if ( node.entity && node.entity->hasComponent<uf::Graphic>() ) {
			auto& graphic = node.entity->getComponent<uf::Graphic>();
			if ( node.buffers.joint < graphic.buffers.size() ) {
				auto& buffer = graphic.buffers.at(node.buffers.joint);
				shader.updateBuffer( (const void*) joints.data(), joints.size() * sizeof(pod::Matrix4f), buffer );
			}
		}
	*/
	}
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

	uf::graph::storage.buffers.camera.destroy();
	uf::graph::storage.buffers.drawCommands.destroy();
	uf::graph::storage.buffers.instance.destroy();
	uf::graph::storage.buffers.joint.destroy();
	uf::graph::storage.buffers.material.destroy();
	uf::graph::storage.buffers.texture.destroy();
	uf::graph::storage.buffers.light.destroy();
}

void uf::graph::initialize() {
#if UF_ENV_DREAMCAST
	const size_t MAX_SIZE = 256;
#else
	const size_t MAX_SIZE = 1024;
#endif
	uf::graph::storage.buffers.camera.initialize( (const void*) nullptr, sizeof(pod::Camera::Viewports), uf::renderer::enums::Buffer::UNIFORM );
	uf::graph::storage.buffers.drawCommands.initialize( (const void*) nullptr, sizeof(pod::DrawCommand)  * MAX_SIZE, uf::renderer::enums::Buffer::STORAGE );
	uf::graph::storage.buffers.instance.initialize( (const void*) nullptr, sizeof(pod::Instance) * MAX_SIZE, uf::renderer::enums::Buffer::STORAGE );
	uf::graph::storage.buffers.joint.initialize( (const void*) nullptr, sizeof(pod::Matrix4f) * MAX_SIZE, uf::renderer::enums::Buffer::STORAGE );
	uf::graph::storage.buffers.material.initialize( (const void*) nullptr, sizeof(pod::Material) * MAX_SIZE, uf::renderer::enums::Buffer::STORAGE );
	uf::graph::storage.buffers.texture.initialize( (const void*) nullptr, sizeof(pod::Texture) * MAX_SIZE, uf::renderer::enums::Buffer::STORAGE );
	uf::graph::storage.buffers.light.initialize( (const void*) nullptr, sizeof(pod::Light) * MAX_SIZE, uf::renderer::enums::Buffer::STORAGE );
}
void uf::graph::tick() {
	uf::stl::vector<pod::Instance> instances; instances.reserve(uf::graph::storage.instances.map.size());
	uf::stl::vector<pod::Matrix4f> joints; joints.reserve(uf::graph::storage.joints.map.size());

	for ( auto key : uf::graph::storage.instances.keys ) instances.emplace_back( uf::graph::storage.instances.map[key] );

	for ( auto key : uf::graph::storage.joints.keys ) {
		auto& matrices = uf::graph::storage.joints[key];
		joints.reserve( joints.size() + matrices.size() );
		for ( auto& mat : matrices ) joints.emplace_back( mat );
	}

	uf::graph::storage.buffers.instance.update( (const void*) instances.data(), instances.size() * sizeof(pod::Instance) );
	uf::graph::storage.buffers.joint.update( (const void*) joints.data(), joints.size() * sizeof(pod::Matrix4f) );

	if ( ::newGraphAdded ) {
		uf::stl::vector<pod::DrawCommand> drawCommands; drawCommands.reserve(uf::graph::storage.drawCommands.map.size());
		uf::stl::vector<pod::Material> materials; materials.reserve(uf::graph::storage.materials.map.size());
		uf::stl::vector<pod::Texture> textures; textures.reserve(uf::graph::storage.textures.map.size());

		for ( auto key : uf::graph::storage.drawCommands.keys ) drawCommands.insert( drawCommands.end(), uf::graph::storage.drawCommands.map[key].begin(), uf::graph::storage.drawCommands.map[key].end() );
	#if 1
		for ( auto key : uf::graph::storage.materials.keys ) materials.emplace_back( uf::graph::storage.materials.map[key] );
		for ( auto key : uf::graph::storage.textures.keys ) textures.emplace_back( uf::graph::storage.textures.map[key] );
	#else
		uf::stl::map<size_t, uf::stl::string> materialOrderedMap; // materialOrderedMap.reserve( uf::graph::storage.materials.keys.size() );
		for ( auto key : uf::graph::storage.materials.keys ) {
			materialOrderedMap[uf::graph::storage.materials.indices[key]] = key;
		}
		for ( auto pair : materialOrderedMap ) materials.emplace_back( uf::graph::storage.materials.map[pair.second] );

		uf::stl::map<size_t, uf::stl::string> textureOrderedMap; // textureOrderedMap.reserve( uf::graph::storage.textures.keys.size() );
		for ( auto key : uf::graph::storage.textures.keys ) {
			textureOrderedMap[uf::graph::storage.textures.indices[key]] = key;
		}
		for ( auto pair : textureOrderedMap ) textures.emplace_back( uf::graph::storage.textures.map[pair.second] );
	#endif
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
void uf::graph::reload() {
	::newGraphAdded = true;
}