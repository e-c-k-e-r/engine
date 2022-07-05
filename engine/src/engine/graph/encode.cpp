#include <uf/engine/graph/graph.h>
#include <uf/ext/gltf/gltf.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/mesh/grid.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/string/base64.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/math/physics.h>
#include <uf/ext/xatlas/xatlas.h>

#if !UF_ENV_DREAMCAST
namespace {
	struct EncodingSettings : public ext::json::EncodingSettings {
		bool combined = false;
		bool encodeBuffers = true;
		bool unwrap = true;
		uf::stl::string filename = "";
	};

	uf::Serializer encode( const uf::Image& image, const EncodingSettings& settings, const pod::Graph& graph ) {
		uf::Serializer json;
		json["size"] = uf::vector::encode( image.getDimensions() );
		json["bpp"] = image.getBpp() / image.getChannels();
		json["channels"] = image.getChannels();
		json["data"] = uf::base64::encode( image.getPixels() );
		return json;
	}
	uf::Serializer encode( const pod::Texture& texture, const EncodingSettings& settings, const pod::Graph& graph ) {
		uf::Serializer json;
		json["index"] = texture.index;
		json["sampler"] = texture.sampler;
		json["remap"] = texture.remap;
		json["blend"] = texture.blend;
		json["lerp"] = uf::vector::encode( texture.lerp, settings );
		return json;
	}
	uf::Serializer encode( const uf::renderer::Sampler& sampler, const EncodingSettings& settings, const pod::Graph& graph ) {
		uf::Serializer json;
		json["min"] = sampler.descriptor.filter.min;
		json["mag"] = sampler.descriptor.filter.mag;
		json["u"] = sampler.descriptor.addressMode.u;
		json["v"] = sampler.descriptor.addressMode.v;
		return json;
	}
	uf::Serializer encode( const pod::Material& material, const EncodingSettings& settings, const pod::Graph& graph ) {
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
	uf::Serializer encode( const pod::Light& light, const EncodingSettings& settings, const pod::Graph& graph ) {
		uf::Serializer json;
		json["color"] = uf::vector::encode( light.color, settings );
		json["intensity"] = light.intensity;
		json["range"] = light.range;
		return json;
	}
	uf::Serializer encode( const pod::Animation& animation, const EncodingSettings& settings, const pod::Graph& graph ) {
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
	uf::Serializer encode( const pod::Skin& skin, const EncodingSettings& settings, const pod::Graph& graph ) {
		uf::Serializer json;
		json["name"] = skin.name;

		ext::json::reserve( json["joints"], skin.joints.size() );
		for ( auto& joint : skin.joints ) json["joints"].emplace_back( joint );

		ext::json::reserve( json["inverseBindMatrices"], skin.inverseBindMatrices.size() );
		for ( auto& inverseBindMatrix : skin.inverseBindMatrices )
			json["inverseBindMatrices"].emplace_back( uf::matrix::encode(inverseBindMatrix, settings) );
		return json;
	}
	uf::Serializer encode( const pod::Instance& instance, const EncodingSettings& settings, const pod::Graph& graph ) {
		uf::Serializer json;
		json["model"] = uf::matrix::encode( instance.model, settings );
		json["color"] = uf::vector::encode( instance.color, settings );
		json["materialID"] = instance.materialID;
		json["primitiveID"] = instance.primitiveID;
		json["meshID"] = instance.meshID;
		json["auxID"] = instance.auxID;
		json["objectID"] = instance.objectID;

		json["bounds"]["min"] = uf::vector::encode( instance.bounds.min, settings );
		json["bounds"]["max"] = uf::vector::encode( instance.bounds.max, settings );

		return json;
	}
	uf::Serializer encode( const pod::DrawCommand& drawCommand, const EncodingSettings& settings, const pod::Graph& graph ) {
		uf::Serializer json;
		json["indices"] = drawCommand.indices;
		json["instances"] = drawCommand.instances;
		json["indexID"] = drawCommand.indexID;
		json["vertexID"] = drawCommand.vertexID;
		json["instanceID"] = drawCommand.instanceID;
		json["auxID"] = drawCommand.auxID;
		json["materialID"] = drawCommand.materialID;
		json["vertices"] = drawCommand.vertices;
		return json;
	}
	uf::Serializer encode( const pod::Primitive& primitive, const EncodingSettings& settings, const pod::Graph& graph ) {
		uf::Serializer json;
		json["drawCommand"] = encode( primitive.drawCommand, settings, graph );
		json["instance"] = encode( primitive.instance, settings, graph );
		return json;
	}
	uf::Serializer encode( const uf::Mesh& mesh, const EncodingSettings& settings, const pod::Graph& graph ) {
		uf::Serializer json;
	#if 0
		uf::Mesh mesh = mesh;
		// remove extraneous buffers
		if ( !mesh.isInterleaved() ) {
			uf::stl::vector<size_t> remove; remove.reserve(mesh.vertex.attributes.size());

			for ( size_t i = 0; i < mesh.vertex.attributes.size(); ++i ) {
				auto& attribute = mesh.vertex.attributes[i];
				if ( attribute.descriptor.name == "position" ) continue;
				if ( attribute.descriptor.name == "color" ) continue;
				if ( attribute.descriptor.name == "uv" ) continue;
				if ( attribute.descriptor.name == "st" ) continue;

				if ( graph.metadata["flags"]["SKINNED"].as<bool>() ) {
					if ( attribute.descriptor.name == "tangent" ) continue;
					if ( attribute.descriptor.name == "joints" ) continue;
					if ( attribute.descriptor.name == "weights" ) continue;
				}
			#if !UF_USE_OPENGL
				if ( attribute.descriptor.name == "normal" ) continue;
			#endif

				remove.insert(remove.begin(), i);
			}
			for ( auto& i : remove ) {
				mesh.buffers[mesh.vertex.attributes[i].buffer].clear();
				mesh.buffers[mesh.vertex.attributes[i].buffer].shrink_to_fit();
				mesh.vertex.attributes.erase(mesh.vertex.attributes.begin() + i);
			}
		} else {
			UF_MSG_DEBUG("Attribute removal requested yet mesh is not interleaved, ignoring...");
		}
	#endif

		#define SERIALIZE_MESH(N) {\
			auto& input = json["inputs"][#N];\
			input["count"] = mesh.N.count;\
			input["first"] = mesh.N.first;\
			input["size"] = mesh.N.size;\
			input["offset"] = mesh.N.offset;\
			input["interleaved"] = mesh.N.interleaved;\
			ext::json::reserve( input["attributes"], mesh.N.attributes.size() );\
			for ( auto& attribute : mesh.N.attributes ) {\
				auto& a = input["attributes"].emplace_back();\
				a["descriptor"]["offset"] = attribute.descriptor.offset;\
				a["descriptor"]["size"] = attribute.descriptor.size;\
				a["descriptor"]["format"] = attribute.descriptor.format;\
				a["descriptor"]["name"] = attribute.descriptor.name;\
				a["descriptor"]["type"] = attribute.descriptor.type;\
				a["descriptor"]["components"] = attribute.descriptor.components;\
				a["buffer"] = attribute.buffer;\
				a["offset"] = attribute.offset;\
				a["stride"] = attribute.stride;\
				a["length"] = attribute.length;\
			}\
		}

		SERIALIZE_MESH(vertex);
		SERIALIZE_MESH(index);
		SERIALIZE_MESH(instance);
		SERIALIZE_MESH(indirect);

		ext::json::reserve( json["buffers"], mesh.buffers.size() );
		for ( auto i = 0; i < mesh.buffers.size(); ++i ) {
			const uf::stl::string filename = settings.filename + ".buffer." + std::to_string(i) + "." + ( settings.compression != "" ? settings.compression : "bin" );
			uf::io::write( filename, mesh.buffers[i] );
			json["buffers"].emplace_back(uf::io::filename( filename ));
		}
		return json;
	}
	uf::Serializer encode( const pod::Node& node, const EncodingSettings& settings, const pod::Graph& graph ) {
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
uf::stl::string uf::graph::save( const pod::Graph& graph, const uf::stl::string& filename ) {
	uf::stl::string directory = uf::io::directory( filename ) + "/" + uf::io::basename(filename) + "/";
	uf::stl::string target = uf::io::directory( filename ) + "/" + uf::io::basename(filename) + ".graph";

	uf::Serializer serializer;
	uf::Serializer metadata;

	::EncodingSettings settings = ::EncodingSettings{
		{
			/*.compression = */graph.metadata["exporter"]["compression"].as<uf::stl::string>("auto"),
			/*.encoding = */graph.metadata["exporter"]["encoding"].as<uf::stl::string>("auto"),
			/*.pretty = */graph.metadata["exporter"]["pretty"].as<bool>(),
			/*.quantize = */graph.metadata["exporter"]["quantize"].as<bool>(),
			/*.precision = */graph.metadata["exporter"]["precision"].as<uint8_t>(),
		},
		/*.combined = */graph.metadata["exporter"]["combined"].as<bool>(),
		/*.encodeBuffers = */graph.metadata["exporter"]["encode buffers"].as<bool>(true),
		/*.unwrap = */graph.metadata["exporter"]["unwrap"].as<bool>(true),
		/*.filename = */directory + "/graph.json",
	};
	
	if ( settings.encoding == "auto" ) settings.encoding = ext::json::PREFERRED_ENCODING;
	if ( settings.compression == "auto" ) settings.compression = ext::json::PREFERRED_COMPRESSION;

	if ( !settings.combined ) uf::io::mkdir(directory);
#if UF_USE_XATLAS
/*
	if ( settings.unwrap ) {
		pod::Graph& g = const_cast<pod::Graph&>(graph);
		auto size = ext::xatlas::unwrap( g );
		serializer["wrapped"] = uf::vector::encode( size );
	}
*/
#endif

#if UF_GRAPH_LOAD_MULTITHREAD
	auto tasks = uf::thread::schedule(true);
#else
	auto tasks = uf::thread::schedule(false);
#endif
	tasks.queue([&]{
		ext::json::reserve( serializer["instances"], graph.instances.size() );
		for ( size_t i = 0; i < graph.instances.size(); ++i ) {
			auto& name = graph.instances[i];
			auto& instance = /*graph.storage*/uf::graph::storage.instances.map.at(name);
			uf::Serializer json = encode( instance, settings, graph );
			json["name"] = name;
			serializer["instances"].emplace_back( json );
		}
	});
	tasks.queue([&]{
		ext::json::reserve( serializer["primitives"], graph.primitives.size() );
		for ( size_t i = 0; i < graph.primitives.size(); ++i ) {
			auto& name = graph.primitives[i];
			auto& primitives = /*graph.storage*/uf::graph::storage.primitives.map.at(name);
			auto& json = serializer["primitives"].emplace_back();
			json["name"] = name;
		//	ext::json::reserve( json["primitives"], primitives.size() );
			for ( auto& primitive : primitives ) {
				json["primitives"].emplace_back( encode( primitive, settings, graph ) );
			}
		}
	});
	tasks.queue([&]{
		ext::json::reserve( serializer["drawCommands"], graph.drawCommands.size() );
		for ( size_t i = 0; i < graph.drawCommands.size(); ++i ) {
			auto& name = graph.drawCommands[i];
			auto& drawCommands = /*graph.storage*/uf::graph::storage.drawCommands.map.at(name);
			auto& json = serializer["drawCommands"].emplace_back();
			json["name"] = name;
		//	ext::json::reserve( json["drawCommands"], drawCommands.size() );
			for ( auto& drawCommand : drawCommands ) {
				json["drawCommands"].emplace_back( encode( drawCommand, settings, graph ) );
			}
		}
	});
	tasks.queue([&]{
		// store mesh information
		ext::json::reserve( serializer["meshes"], graph.meshes.size() );
		if ( !settings.combined ) {
			::EncodingSettings s = settings;
			for ( size_t i = 0; i < graph.meshes.size(); ++i ) {
				auto& name = graph.meshes[i];
				auto& mesh = /*graph.storage*/uf::graph::storage.meshes.map.at(name);
				if ( !s.encodeBuffers ) {
					s.filename = directory+"/mesh."+std::to_string(i)+".json";
					encode(mesh, s, graph).writeToFile(s.filename);
					uf::Serializer json;
					json["name"] = name;
					json["filename"] = uf::io::filename(s.filename);
					serializer["meshes"].emplace_back( json );
				} else {
					s.filename = directory+"/mesh."+std::to_string(i);
					auto json = encode(mesh, s, graph);
					json["name"] = name;
					serializer["meshes"].emplace_back(json);
				}
			}
		} else {
			for ( auto& name : graph.meshes ) {
				auto& mesh = /*graph.storage*/uf::graph::storage.meshes.map.at(name);
				auto json = encode(mesh, settings, graph);
				json["name"] = name;
				serializer["meshes"].emplace_back(json);
			}
		}
	});
#if 0
	tasks.queue([&]{
		if ( uf::graphs::storage.atlases[graph.atlas].generated() ) {
			auto atlasName = filename + "/" + "atlas";
			auto& atlas = /*graph.storage*/uf::graph::storage.atlases[atlasName];
			auto& image = atlas.getAtlas();
			if ( !settings.combined ) {
				image.save(directory + "/atlas.png");
				serializer["atlas"] = "atlas.png";
			} else {
				serializer["atlas"] = encode(image, settings, graph);
			}
		}
	});
#endif
	tasks.queue([&]{
		ext::json::reserve( serializer["images"], graph.images.size() );
		if ( !settings.combined ) {
			for ( size_t i = 0; i < graph.images.size(); ++i ) {
				auto& name = graph.images[i];
				auto& image = /*graph.storage*/uf::graph::storage.images.map.at(name);
				uf::stl::string f = "image."+std::to_string(i)+".png";

				image.save(directory + "/" + f);
				uf::Serializer json;
				json["name"] = name;
				json["filename"] = f;
				serializer["images"].emplace_back( json );
			}
		} else {
			for ( auto& name : graph.images ) {
				auto& image = /*graph.storage*/uf::graph::storage.images.map.at(name);
				auto json = encode(image, settings, graph);
				json["name"] = name;
				serializer["images"].emplace_back( json );
			}
		}
	});
	tasks.queue([&]{
		// store texture information
		ext::json::reserve( serializer["textures"], graph.textures.size() );
		for ( auto& name : graph.textures ) {
			auto& texture = /*graph.storage*/uf::graph::storage.textures.map.at(name);
			auto json = encode(texture, settings, graph);
			json["name"] = name;
			serializer["textures"].emplace_back(json);
		}
	});
	tasks.queue([&]{
		// store sampler information
		ext::json::reserve( serializer["samplers"], graph.samplers.size() );
		for ( auto& name : graph.samplers ) {
			auto& sampler = /*graph.storage*/uf::graph::storage.samplers.map.at(name);
			auto json = encode(sampler, settings, graph);
			json["name"] = name;
			serializer["samplers"].emplace_back(json);
		}
	});
	tasks.queue([&]{
		// store material information
		ext::json::reserve( serializer["materials"], graph.materials.size() );
		for ( auto& name : graph.materials ) {
			auto& material = /*graph.storage*/uf::graph::storage.materials.map.at(name);
			auto json = encode(material, settings, graph);
			json["name"] = name;
			serializer["materials"].emplace_back(json);
		}
	});
	tasks.queue([&]{
		// store light information
		ext::json::reserve( serializer["lights"], graph.lights.size() );
		for ( auto pair : graph.lights ) {
			auto& name = pair.first;
			auto& light = pair.second;
			auto json = encode(light, settings, graph);
			json["name"] = name;
			serializer["lights"].emplace_back(json);
		}
	});
	tasks.queue([&]{
		// store animation information
		ext::json::reserve( serializer["animations"], graph.animations.size() );
		if ( !settings.combined ) {
			for ( auto i = 0; i < graph.animations.size(); ++i ) {
				auto& name = graph.animations[i];
				uf::stl::string f = "animation."+std::to_string(i)+".json";
				auto& animation = /*graph.storage*/uf::graph::storage.animations.map.at(name);
				encode(animation, settings, graph).writeToFile(directory+"/"+f);
				serializer["animations"].emplace_back(f);
			}
		} else {
			for ( auto& name : graph.animations ) {
				auto& animation = /*graph.storage*/uf::graph::storage.animations.map.at(name);
				serializer["animations"][name] = encode(animation, settings, graph);
			}
		}
	});
	tasks.queue([&]{
		// store skin information
		ext::json::reserve( serializer["skins"], graph.skins.size() );
		for ( auto& name : graph.skins ) {
			auto& skin = /*graph.storage*/uf::graph::storage.skins.map.at(name);
			serializer["skins"].emplace_back( encode(skin, settings, graph) );
		}
	});
	tasks.queue([&]{
		// store node information
		ext::json::reserve( serializer["nodes"], graph.nodes.size() );
		for ( auto& node : graph.nodes ) serializer["nodes"].emplace_back( encode(node, settings, graph) );
		serializer["root"] = encode(graph.root, settings, graph);
	});

	uf::thread::execute( tasks );

	if ( !settings.combined ) target = directory + "/graph.json";
	serializer.writeToFile( target );
	UF_MSG_DEBUG("Saving graph to {}", target);

/*
	if ( graph.metadata["exporter"]["quit"].as<bool>(true) ) {
		ext::json::Value payload;
		payload["message"] = "Termination after gltf conversion requested.";
		uf::scene::getCurrentScene().queueHook("system:Quit", payload);
	}
*/

	return target;
}

uf::stl::string uf::graph::print( const pod::Graph& graph ) {
	uf::stl::stringstream ss;
#if 1
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
	size_t memoryTextures = sizeof(pod::Texture) * graph.textures.size();
	size_t memoryMaterials = sizeof(pod::Material) * graph.materials.size();
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
#endif