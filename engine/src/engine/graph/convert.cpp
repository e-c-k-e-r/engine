#include <uf/engine/graph/graph.h>
#include <uf/ext/gltf/gltf.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/mesh/grid.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/string/base64.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/camera/camera.h>

#if UF_USE_MESHOPT
	#include <uf/ext/meshopt/meshopt.h>
#endif
#if UF_USE_XATLAS
	#include <uf/ext/xatlas/xatlas.h>
#endif

namespace {
	size_t process( uf::Object& object, pod::Graph& graph, pod::Node& parent ) {
		auto& storage = uf::graph::getStorage( graph );

		size_t nodeID = graph.nodes.size();
		size_t meshID = graph.meshes.size();
		size_t objectID = storage.entities.keys.size();

		uf::stl::string keyName = graph.name + "[" + std::to_string(objectID) + "]";

		auto& node = graph.nodes.emplace_back();
		node.name = object.getName();
		node.index = nodeID;
		node.parent = parent.index;
		node.entity = &object;
		node.transform.reference = &object.getComponent<pod::Transform<>>();
		node.mesh = -1;
		node.skin = -1;

		storage.entities[std::to_string(objectID)] = &object;

		pod::Instance::Object instanceObject;
		instanceObject.model = uf::transform::model( object.getComponent<pod::Transform<>>() );
		instanceObject.previous = instanceObject.model;
		storage.objects[std::to_string(objectID)] = instanceObject;

		if ( object.hasComponent<uf::renderer::Graphic>() ) {
			auto& graphic = object.getComponent<uf::renderer::Graphic>();
			auto& textures = graphic.material.textures;

			size_t sub = 0;
			size_t materialID = graph.materials.size();
			for ( auto& t : textures ) {
				size_t textureID = graph.textures.size();
				size_t texture2DID = graph.images.size();

				uf::stl::string subName = keyName + "[" + std::to_string(sub++) + "]";
				auto& material = storage.materials[graph.materials.emplace_back(subName)];
				auto& texture = storage.textures[graph.textures.emplace_back(subName)];
				auto& texture2D = storage.images[graph.images.emplace_back(subName)].handle;

				material.indexAlbedo = textureID;
				material.colorBase = {1, 1, 1, 1};
				texture.index = texture2DID;
				texture2D.aliasTexture(t);
			}

			if ( object.hasComponent<uf::Mesh>() ) {
				node.mesh = meshID;
				auto& primitives = storage.primitives[graph.primitives.emplace_back(keyName)];
				auto& mesh = (storage.meshes[graph.meshes.emplace_back(keyName)] = object.getComponent<uf::Mesh>());

				pod::Vector3f boundsMin = {  std::numeric_limits<float>::max(),  std::numeric_limits<float>::max(),  std::numeric_limits<float>::max() };
				pod::Vector3f boundsMax = { -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max() };

				for ( auto& attribute : mesh.vertex.attributes ) {
					if ( attribute.descriptor.name != "position" ) continue;
					if ( mesh.vertex.count > 0 && attribute.pointer ) {
						for ( size_t i = 0; i < mesh.vertex.count; ++i ) {
							auto& position = *(const pod::Vector3f*) ( attribute.pointer + attribute.stride * (mesh.vertex.first + i));
							boundsMin = uf::vector::min( boundsMin, position );
							boundsMax = uf::vector::max( boundsMax, position );
						}
					}
				}

				size_t drawCommandCount = mesh.indirect.count > 0 ? mesh.indirect.count : 1;
				pod::DrawCommand* sourceDrawCommands = nullptr;

				if ( mesh.indirect.count > 0 ) {
					auto& attribute = mesh.indirect.attributes.front();
					auto& buffer = mesh.buffers[attribute.buffer];
					if ( !buffer.empty() ) {
						sourceDrawCommands = (pod::DrawCommand*) buffer.data();
					}
				}

				for ( size_t drawCommandID = 0; drawCommandID < drawCommandCount; ++drawCommandID ) {
					size_t primitiveID = primitives.size();
					size_t instanceID = primitiveID;

					auto& primitive = primitives.emplace_back();
					auto& drawCommand = primitive.drawCommand;
					auto& instance = primitive.instance;

					instance.materialID = materialID;
					instance.primitiveID = primitiveID;
					instance.meshID = meshID;
					instance.objectID = objectID;
					instance.jointID = -1;
					instance.bounds.min = boundsMin;
					instance.bounds.max = boundsMax;

					if ( sourceDrawCommands ) {
						drawCommand = sourceDrawCommands[drawCommandID];
					} else {
						drawCommand.indices = mesh.index.count;
						drawCommand.instances = 1;
						drawCommand.indexID = mesh.index.first;
						drawCommand.vertexID = mesh.vertex.first;
						drawCommand.instanceID = instanceID;
						drawCommand.vertices = mesh.vertex.count;
					}

					drawCommand.instanceID = instanceID;

					primitive.lod.levels[0].indices = drawCommand.indices;
					primitive.lod.levels[0].indexID = drawCommand.indexID;
					primitive.lod.levels[0].vertexID = drawCommand.vertexID;
					primitive.lod.levels[0].vertices = drawCommand.vertices;
				}

				uf::graph::initializeGraphics( graph, object, mesh, primitives );
			}
		}

		for ( auto* child : object.getChildren() ) {
			node.children.emplace_back( process( child->as<uf::Object>(), graph, node ) );
		}

		return nodeID;
	}
}

pod::Graph& uf::graph::convert( uf::Object& object, bool process ) {
	auto& graph = object.getComponent<pod::Graph>();
	auto& storage = uf::graph::getStorage( graph );

	graph.name = object.getName();
	graph.metadata = object.getComponent<uf::Serializer>()["graph"];
	graph.root.entity = &object;
	graph.root.index = 0;
	graph.root.parent = -1;

	::process( object, graph, graph.root );

	if ( process ) {
		for ( auto index : graph.root.children ) {
			uf::graph::process( graph, index, *graph.root.entity );
		}
	}

	for ( auto& name : graph.textures ) {
		auto& texture = storage.textures[name];
		auto& indices = storage.images.indices;

		if ( !(0 <= texture.index && texture.index < graph.images.size()) ) continue;

		auto& needle = graph.images[texture.index];
		texture.index = indices[needle];
	}

	for ( auto& name : graph.materials ) {
		auto& material = storage.materials[name];
		auto& indices = storage.textures.indices;

		int32_t* IDs[] = { &material.indexAlbedo, &material.indexNormal, &material.indexEmissive, &material.indexOcclusion, &material.indexMetallicRoughness };
		for ( auto* pointer : IDs ) {
			auto& ID = *pointer;
			if ( !(0 <= ID && ID < graph.textures.size()) ) continue;
			auto& needle = graph.textures[ID];
			ID = indices[needle];
		}
	}

	for ( auto& name : graph.primitives ) {
		auto& primitives = storage.primitives[name];
		for ( auto& primitive : primitives ) {
			auto& instance = primitive.instance;

			if ( 0 <= instance.materialID && instance.materialID < graph.materials.size() ) {
				auto& indices = storage.materials.indices;
				auto& needle = graph.materials[instance.materialID];
				instance.materialID = indices[needle];
			}

			if ( 0 <= instance.lightmapID && instance.lightmapID < graph.textures.size() ) {
				auto& indices = storage.textures.indices;
				auto& needle = graph.textures[instance.lightmapID];
				instance.lightmapID = indices[needle];
			}

			if ( 0 <= instance.jointID && instance.jointID < graph.skins.size() ) {
				auto& skinName = graph.skins[instance.jointID];
				instance.jointID = 0;
				for ( auto key : storage.joints.keys ) {
					if ( key == skinName ) break;
					auto& joints = storage.joints[key];
					instance.jointID += joints.size();
				}
			}
		}
	}

	uf::graph::reload();

	return graph;
}

void uf::graph::postprocess( pod::Graph& graph ) {
	auto& storage = uf::graph::getStorage( graph );
	// post-processing
#if UF_USE_XATLAS
	// generate STs
	if ( graph.metadata["exporter"]["unwrap"].as<bool>(true) || graph.metadata["exporter"]["unwrap"].as<uf::stl::string>() == "tagged" ) {
		//UF_MSG_DEBUG( "Generating ST's..." );
		size_t atlases = ext::xatlas::unwrap( graph );
		//UF_MSG_DEBUG( "Generated ST's for {} lightmaps", atlases );
	}
#endif
#if UF_USE_MESHOPT
	// cleanup if blender's exporter is poopy
	if ( graph.metadata["exporter"]["optimize"].as<bool>(false) || graph.metadata["exporter"]["optimize"].as<uf::stl::string>("") == "tagged" || ext::json::isObject( graph.metadata["exporter"]["optimize"] ) ) {
		//UF_MSG_DEBUG( "Optimizing meshes..." );
		for ( auto& keyName : graph.meshes ) {
			size_t level = SIZE_MAX;
			float simplify = 1.0f;
			bool print = false;
			bool lods = false;

			if ( graph.metadata["exporter"]["optimize"].as<uf::stl::string>("") == "tagged" ) {
				bool should = false;

				ext::json::forEach( graph.metadata["tags"], [&]( const uf::stl::string& key, ext::json::Value& value ) {
					if ( ext::json::isNull( value["optimize mesh"] ) ) return;
					if ( uf::string::isRegex( key ) ) {
						if ( !uf::string::matched( keyName, key ) ) return;
					} else if ( keyName != key ) return;
					should = true;
					if ( ext::json::isObject( value["optimize mesh"] ) ) {
						level = value["optimize mesh"]["level"].as(level);
						simplify = value["optimize mesh"]["simplify"].as(simplify);
						print = value["optimize mesh"]["print"].as(print);
						lods = value["optimize mesh"]["lods"].as(lods);
					}
				});

				if ( !should ) continue;
			} else if ( ext::json::isObject( graph.metadata["exporter"]["optimize"] ) ) {
				level = graph.metadata["exporter"]["optimize"]["level"].as( level );
				simplify = graph.metadata["exporter"]["optimize"]["simplify"].as( simplify );
				print = graph.metadata["exporter"]["optimize"]["print"].as( print );
				lods = graph.metadata["exporter"]["optimize"]["lods"].as( lods );
			}

			auto& mesh = storage.meshes[keyName];
			auto& primitives = storage.primitives[keyName];
			
			if ( level ) {
				//UF_MSG_DEBUG("Optimizing mesh at level {}: {}", level, keyName);
				if ( !ext::meshopt::optimize( mesh, simplify, level, print ) ) {
					UF_MSG_ERROR("Mesh optimization failed: {}", keyName );
				}
			}
			if ( lods ) {
				auto factors = ext::meshopt::computeLODs( mesh.index.count );
				auto lodMetadata = ext::meshopt::generateLODs( mesh, factors, print );
				if ( lodMetadata.empty() ) {
					UF_MSG_ERROR("LOD generation failed: {}", keyName );
				} else {
					//UF_MSG_DEBUG("Generated {} LODs: {}", factors.size() - 1, keyName);
					UF_ASSERT( primitives.size() == lodMetadata.size() );
					for ( auto i = 0; i < primitives.size(); ++i ) {
						primitives[i].lod = lodMetadata[i];
					}
				}
			}
		}

		//UF_MSG_DEBUG( "Optimized mesh" );
	}
#endif
	{
		// update primitive info
		for ( auto& keyName : graph.meshes ) {
			auto& mesh = storage.meshes[keyName];
			auto& primitives = storage.primitives[keyName];
			
			UF_ASSERT( primitives.size() == mesh.indirect.count );
			auto& attribute = mesh.indirect.attributes.front();
			auto& buffer = mesh.buffers[attribute.buffer];
			pod::DrawCommand* drawCommands = (pod::DrawCommand*) buffer.data();
			for ( auto drawID = 0; drawID < primitives.size(); ++drawID ) {
				primitives[drawID].drawCommand = drawCommands[drawID];
			}
		}
	}

	if ( graph.metadata["exporter"]["enabled"].as<bool>() ) {
	#if !UF_ENV_DREAMCAST
		graph.name = uf::graph::save( graph, graph.name );
	#endif
		
	// 	disable baking, doesn't output right if baking from a gltf imported model
	//	graph.metadata["baking"]["enabled"] = false;

	//	disable lightmap loading, 99.999% of the time a previously baked lightmap will not work due to changing STs
		graph.metadata["lights"]["lightmapped"] = false;
	}

	// disable streaming
	{
		graph.settings.stream.enabled = false;

		graph.settings.stream.textures = false;
		graph.settings.stream.animations = false;

		graph.settings.stream.radius = 0;
		graph.settings.stream.every = 0;
	}
}