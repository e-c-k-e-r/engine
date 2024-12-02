#include <uf/engine/graph/graph.h>
#include <uf/ext/gltf/gltf.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/mesh/grid.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/string/base64.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/camera/camera.h>
#include <uf/ext/xatlas/xatlas.h>


namespace {
	size_t process( uf::Object& object, pod::Graph& graph, pod::Node& parent ) {
		auto& scene = uf::scene::getCurrentScene();
		auto& storage = uf::graph::globalStorage ? uf::graph::storage : scene.getComponent<pod::Graph::Storage>();

		// grab relevant IDs
		size_t nodeID = graph.nodes.size();
		size_t instanceID = storage.instances.keys.size(); // graph.instances.size();
		size_t primitiveID = graph.primitives.size();
		size_t drawCommandID = graph.drawCommands.size();
		size_t meshID = graph.meshes.size();
		size_t objectID = storage.entities.keys.size();

		uf::stl::string keyName = graph.name + "[" + std::to_string(objectID) + "]";
		// create node
		auto& node = graph.nodes.emplace_back();
		node.index = nodeID;
		node.parent = parent.index;
		node.entity = &object;
		node.transform.reference = &object.getComponent<pod::Transform<>>();
		storage.entities[std::to_string(objectID)] = &object;
		// grab relevant data
		if ( object.hasComponent<uf::Graphic>() ) {
			auto& graphic = object.getComponent<uf::Graphic>();
			auto& textures = graphic.material.textures;

			// assume we'll only use one material ID for now
			size_t sub = 0;
			size_t materialID = graph.materials.size();
			for ( auto& t : textures ) {
				size_t textureID = graph.textures.size();
				size_t texture2DID = graph.texture2Ds.size();

				uf::stl::string subName = keyName + "[" + std::to_string(sub++) + "]";
				auto& material = storage.materials[graph.materials.emplace_back(subName)];
				auto& texture = storage.textures[graph.textures.emplace_back(subName)];
				auto& texture2D = storage.texture2Ds[graph.texture2Ds.emplace_back(subName)];

				material.indexAlbedo = textureID;
				material.colorBase = {1,1,1,1};
				texture.index = texture2DID;
				texture2D.aliasTexture(t);
			}
			// graphic.material.textures.clear();

			if ( object.hasComponent<uf::Mesh>() ) {
				node.mesh = meshID;
				// import
				auto& instance = storage.instances[graph.instances.emplace_back(keyName)];
				auto& drawCommands = storage.drawCommands[graph.drawCommands.emplace_back(keyName)];
				auto& drawCommand = drawCommands.emplace_back();
				auto& primitives = storage.primitives[graph.primitives.emplace_back(keyName)];
				auto& primitive = primitives.emplace_back();
				auto& mesh = (storage.meshes[graph.meshes.emplace_back(keyName)] = object.getComponent<uf::Mesh>());
		
				pod::Vector3f boundsMin = {  std::numeric_limits<float>::max(),  std::numeric_limits<float>::max(),  std::numeric_limits<float>::max() };
				pod::Vector3f boundsMax = { -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max() };

				for ( auto& attribute : mesh.vertex.attributes ) {
					if ( attribute.descriptor.name != "position" ) continue;
					for ( size_t i = 0; i < mesh.vertex.count; ++i ) {
						auto& position = *(const pod::Vector3f*) ( attribute.pointer + attribute.stride * (mesh.vertex.first + i));
						boundsMin = uf::vector::min( boundsMin, position );
						boundsMax = uf::vector::max( boundsMax, position );
					}
				}

				instance.materialID = materialID;
				instance.primitiveID = primitiveID;
				instance.meshID = meshID;
				instance.objectID = objectID;
				instance.bounds.min = boundsMin;
				instance.bounds.max = boundsMax;

				drawCommand.indices = mesh.index.count;
				drawCommand.instances = 1;
				drawCommand.indexID = 0;
				drawCommand.vertexID = 0;
				drawCommand.instanceID = instanceID;
				drawCommand.vertices = mesh.vertex.count;

				primitive.instance = instance;
				primitive.drawCommand = drawCommand;

				mesh.insertIndirects(drawCommands);
				mesh.updateDescriptor();

				uf::graph::initializeGraphics( graph, object, mesh );
			}
		}


		// recurse
		for ( auto* child : object.getChildren() ) {
			node.children.emplace_back( process( child->as<uf::Object>(), graph, node ) );
		}

		return nodeID;
	}
}

pod::Graph& uf::graph::convert( uf::Object& object, bool process ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& storage = uf::graph::globalStorage ? uf::graph::storage : scene.getComponent<pod::Graph::Storage>();

	auto& graph = object.getComponent<pod::Graph>();

	graph.name = object.getName();
	graph.metadata = object.getComponent<uf::Serializer>()["graph"];
	graph.root.entity = &object;

	::process( object, graph, graph.root );

	// process nodes
	if ( process ) {
		for ( auto index : graph.root.children ) uf::graph::process( graph, index, *graph.root.entity );
	}

	// remap textures->images IDs
	for ( auto& name : graph.textures ) {
		auto& texture = storage.textures[name];
		auto& keys = storage.images.keys;
		auto& indices = storage.images.indices;

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
		auto& material = storage.materials[name];
		auto& keys = storage.textures.keys;
		auto& indices = storage.textures.indices;
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
		auto& instance = storage.instances[name];
		
		if ( 0 <= instance.materialID && instance.materialID < graph.materials.size() ) {
			auto& keys = /*graph.storage*/storage.materials.keys;
			auto& indices = /*graph.storage*/storage.materials.indices;
			
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
			auto& keys = /*graph.storage*/storage.textures.keys;
			auto& indices = /*graph.storage*/storage.textures.indices;

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
			auto& keys = /*graph.storage*/storage.images.keys;
			auto it = std::find( keys.begin(), keys.end(), graph.images[instance.imageID] );
			UF_ASSERT( it != keys.end() );
			instance.imageID = it - keys.begin();
		}
	#endif
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

	uf::graph::reload();

	return graph;
}