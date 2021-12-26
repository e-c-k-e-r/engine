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
	size_t UF_API process( uf::Object& object, pod::Graph& graph, pod::Node& parent ) {
		// grab relevant IDs
		size_t nodeID = graph.nodes.size();
		size_t instanceID = uf::graph::storage.instances.keys.size(); // graph.instances.size();
		size_t primitiveID = graph.primitives.size();
		size_t drawCommandID = graph.drawCommands.size();
		size_t meshID = graph.meshes.size();
		size_t objectID = uf::graph::storage.entities.keys.size();

		uf::stl::string keyName = graph.name + "[" + std::to_string(nodeID) + "]";
		// create node
		auto& node = graph.nodes.emplace_back();
		node.index = nodeID;
		node.parent = parent.index;
		node.entity = &object;
		node.transform.reference = &object.getComponent<pod::Transform<>>();
		uf::graph::storage.entities[std::to_string(objectID)] = &object;
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
				auto& material = uf::graph::storage.materials[graph.materials.emplace_back(subName)];
				auto& texture = uf::graph::storage.textures[graph.textures.emplace_back(subName)];
				auto& texture2D = uf::graph::storage.texture2Ds[graph.texture2Ds.emplace_back(subName)];

				material.indexAlbedo = textureID;
				texture.index = texture2DID;
				texture2D.aliasTexture(t);
			}


			if ( object.hasComponent<uf::Mesh>() ) {
				node.mesh = meshID;
				// import
				auto& instance = uf::graph::storage.instances[graph.instances.emplace_back(keyName)];
				auto& drawCommands = uf::graph::storage.drawCommands[graph.drawCommands.emplace_back(keyName)];
				auto& drawCommand = drawCommands.emplace_back();
				auto& primitives = uf::graph::storage.primitives[graph.primitives.emplace_back(keyName)];
				auto& primitive = primitives.emplace_back();
				auto& mesh = (uf::graph::storage.meshes[graph.meshes.emplace_back(keyName)] = object.getComponent<uf::Mesh>());
		
				pod::Vector3f boundsMin = { -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max() };
				pod::Vector3f boundsMax = {  std::numeric_limits<float>::max(),  std::numeric_limits<float>::max(),  std::numeric_limits<float>::max() };

				// do some position min/maxing
				{

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
				
				graphic.initialize();
				graphic.initializeMesh( mesh );
				graphic.process = true;

				uf::graph::initializeGraphics( graph, object );
			}
		}


		// recurse
		for ( auto* child : object.getChildren() ) {
			node.children.emplace_back( process( child->as<uf::Object>(), graph, node ) );
		}

		return nodeID;
	}
}

pod::Graph& UF_API uf::graph::convert( uf::Object& object, bool process ) {
	auto& graph = object.getComponent<pod::Graph>();

	graph.name = object.getName();
	graph.metadata = object.getComponent<uf::Serializer>()["model"];
	graph.root.entity = &object;

	::process( object, graph, graph.root );

	// process nodes
	if ( process ) {
		for ( auto index : graph.root.children ) uf::graph::process( graph, index, *graph.root.entity );
	}

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
/*
	for ( auto& name : graph.textures ) {
		auto& texture = uf::graph::storage.textures[name];
		auto& keys = uf::graph::storage.images.keys;
		
		if ( !(0 <= texture.index && texture.index < graph.textures.size()) ) continue;
		auto it = std::find( keys.begin(), keys.end(), graph.images[texture.index] );
		UF_ASSERT( it != keys.end() );
		texture.index = it - keys.begin();
	}
*/
	// remap instance variables
	for ( auto& name : graph.instances ) {
		auto& instance = uf::graph::storage.instances[name];
		
		if ( 0 <= instance.materialID && instance.materialID < graph.materials.size() ) {
			auto& keys = /*graph.storage*/uf::graph::storage.materials.keys;
			auto it = std::find( keys.begin(), keys.end(), graph.materials[instance.materialID] );
			UF_ASSERT( it != keys.end() );
			instance.materialID = it - keys.begin();
		}
		if ( 0 <= instance.imageID && instance.imageID < graph.images.size() ) {
			auto& keys = /*graph.storage*/uf::graph::storage.images.keys;
			auto it = std::find( keys.begin(), keys.end(), graph.images[instance.imageID] );
			UF_ASSERT( it != keys.end() );
			instance.imageID = it - keys.begin();
		}
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
		if ( 0 <= instance.lightmapID && instance.lightmapID < graph.textures.size() ) {
			auto& keys = /*graph.storage*/uf::graph::storage.textures.keys;
			auto it = std::find( keys.begin(), keys.end(), graph.textures[instance.lightmapID] );
			UF_ASSERT( it != keys.end() );
			instance.lightmapID = it - keys.begin();
		}
	}

	uf::graph::reload();

	return graph;
}