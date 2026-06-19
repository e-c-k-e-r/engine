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
#include <uf/ext/texconv/texconv.h>

#if !UF_ENV_DREAMCAST
namespace {
	struct EncodingSettings : public ext::json::EncodingSettings {
		bool combined = false;
		bool encodeBuffers = true;
		bool unwrap = true;
		uf::stl::string filename = "";
		uf::stl::string conversion = "";
	};

	
	uf::Serializer encode( const pod::Animation& animation, const EncodingSettings& settings, const pod::Graph& graph, uf::stl::vector<uint8_t>& outBuffer, const uf::stl::string& binFilename ) {
		uf::Serializer json;
		json["name"] = animation.name;
		json["start"] = animation.start;
		json["end"] = animation.end;

		auto appendToBuffer = [&](const void* data, size_t size) -> size_t {
			size_t offset = outBuffer.size();
			if (size > 0) {
				const uint8_t* bytes = static_cast<const uint8_t*>(data);
				outBuffer.insert(outBuffer.end(), bytes, bytes + size);
			}
			return offset;
		};

		ext::json::reserve( json["samplers"], animation.samplers.size() );
		auto& samplers = json["samplers"];
		for ( auto& sampler : animation.samplers ) {
			auto& sJson = samplers.emplace_back();
			sJson["interpolator"] = sampler.interpolator;

			size_t inputsSize = sampler.inputs.size() * sizeof(float);
			sJson["inputs"]["count"] = sampler.inputs.size();
			sJson["inputs"]["offset"] = appendToBuffer(sampler.inputs.data(), inputsSize);
			sJson["inputs"]["length"] = inputsSize;

			size_t outputsSize = sampler.outputs.size() * sizeof(pod::Vector4f);
			sJson["outputs"]["count"] = sampler.outputs.size();
			sJson["outputs"]["offset"] = appendToBuffer(sampler.outputs.data(), outputsSize);
			sJson["outputs"]["length"] = outputsSize;
		}

		json["buffer"] = binFilename;

		ext::json::reserve( json["channels"], animation.channels.size() );
		auto& channels = json["channels"];
		for ( auto& channel : animation.channels ) {
			auto& cJson = channels.emplace_back();
			cJson["path"] = channel.path;
			cJson["node"] = channel.node;
			cJson["sampler"] = channel.sampler;
		}

		return json;
	}
	uf::Serializer encode( const pod::Skin& skin, const EncodingSettings& settings, const pod::Graph& graph, uf::stl::vector<uint8_t>& outBuffer, const uf::stl::string& binFilename ) {
		uf::Serializer json;
		json["name"] = skin.name;

		ext::json::reserve( json["joints"], skin.joints.size() );
		for ( auto& joint : skin.joints ) {
			json["joints"].emplace_back( joint );
		}

		size_t matricesSize = skin.inverseBindMatrices.size() * sizeof(pod::Matrix4f);
		size_t offset = outBuffer.size();

		if ( matricesSize > 0 ) {
			const uint8_t* bytes = reinterpret_cast<const uint8_t*>(skin.inverseBindMatrices.data());
			outBuffer.insert(outBuffer.end(), bytes, bytes + matricesSize);
		}

		json["inverseBindMatrices"]["buffer"] = binFilename;
		json["inverseBindMatrices"]["count"] = skin.inverseBindMatrices.size();
		json["inverseBindMatrices"]["offset"] = offset;
		json["inverseBindMatrices"]["length"] = matricesSize;

		return json;
	}
	
	uf::Mesh reencode( const uf::Mesh& _mesh, const uf::stl::string& conversion ) {
		uf::Mesh mesh = _mesh.copy();
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
		mesh.updateDescriptor();
		return mesh;
	}

	uf::Serializer encode( const uf::Mesh& mesh, const EncodingSettings& settings, const pod::Graph& graph, uf::stl::vector<uint8_t>& outBuffer, const uf::stl::string& binFilename, bool checkForConversions = true ) {
		if ( checkForConversions && settings.conversion != "" ) {
		//  return encode( reencode( mesh, settings.conversion ), settings, graph, outBuffer, binFilename, false );
		}

		uf::Serializer json;
		#define SERIALIZE_MESH(N) {\
			auto& input = json["inputs"][#N];\
			input["count"] = mesh.N.count;\
			input["first"] = mesh.N.first;\
			input["size"] = mesh.N.size;\
			input["offset"] = mesh.N.offset;\
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
			size_t offset = outBuffer.size();
			size_t length = mesh.buffers[i].size();

			outBuffer.insert(outBuffer.end(), mesh.buffers[i].begin(), mesh.buffers[i].end());

			auto& bufJson = json["buffers"].emplace_back();
			bufJson["filename"] = binFilename;
			bufJson["offset"] = offset;
			bufJson["length"] = length;
		}

		#undef SERIALIZE_MESH

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
		json["metadata"] = node.metadata;
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
		/*.conversion = */graph.metadata["exporter"]["conversion"].as<uf::stl::string>(),
	};

	if ( graph.metadata["exporter"]["compression"].is<bool>() ) {
		settings.compression = graph.metadata["exporter"]["compression"].as<bool>() ? "auto" : "none";
	}
	
	if ( settings.encoding == "auto" ) settings.encoding = ext::json::PREFERRED_ENCODING;
	if ( settings.compression == "auto" ) settings.compression = ext::json::PREFERRED_COMPRESSION;

	if ( !settings.combined ) uf::io::mkdir(directory);
	serializer["metadata"] = graph.metadata;

#if UF_GRAPH_LOAD_MULTITHREAD
	auto tasks = uf::thread::schedule(true);
#else
	auto tasks = uf::thread::schedule(false);
#endif
	auto& storage = uf::graph::getStorage( graph );

	tasks.queue([&]{
		uf::stl::vector<pod::Material> flatMaterials;
		flatMaterials.reserve(graph.materials.size());
		for ( auto& name : graph.materials ) {
			flatMaterials.push_back(storage.materials.map.at(name));
		}

		if (!flatMaterials.empty()) {
			size_t length = flatMaterials.size() * sizeof(pod::Material);
			uf::stl::string binName = "materials.bin";
			uf::io::write(directory + "/" + binName, flatMaterials.data(), length);

			serializer["materials"]["names"] = graph.materials;
			serializer["materials"]["buffer"] = binName;
			serializer["materials"]["count"] = flatMaterials.size();
			serializer["materials"]["length"] = length;
		}
	});

	tasks.queue([&]{
		uf::stl::vector<pod::Texture> flatTextures;
		flatTextures.reserve(graph.textures.size());
		for ( auto& name : graph.textures ) {
			flatTextures.push_back(storage.textures.map.at(name));
		}

		if (!flatTextures.empty()) {
			size_t length = flatTextures.size() * sizeof(pod::Texture);
			uf::stl::string binName = "textures.bin";
			uf::io::write(directory + "/" + binName, flatTextures.data(), length);

			serializer["textures"]["names"] = graph.textures;
			serializer["textures"]["buffer"] = binName;
			serializer["textures"]["count"] = flatTextures.size();
			serializer["textures"]["length"] = length;
		}
	});

	tasks.queue([&]{
		uf::stl::vector<uf::renderer::Sampler> flatSamplers;
		flatSamplers.reserve(graph.samplers.size());
		for ( auto& name : graph.samplers ) {
			flatSamplers.push_back(storage.samplers.map.at(name));
		}

		if (!flatSamplers.empty()) {
			size_t length = flatSamplers.size() * sizeof(uf::renderer::Sampler);
			uf::stl::string binName = "samplers.bin";
			uf::io::write(directory + "/" + binName, flatSamplers.data(), length);

			serializer["samplers"]["names"] = graph.samplers;
			serializer["samplers"]["buffer"] = binName;
			serializer["samplers"]["count"] = flatSamplers.size();
			serializer["samplers"]["length"] = length;
		}
	});

	tasks.queue([&]{
		uf::stl::vector<pod::Light> flatLights;
		uf::stl::vector<uf::stl::string> lightNames;
		flatLights.reserve(graph.lights.size());
		lightNames.reserve(graph.lights.size());

		for ( auto& pair : graph.lights ) {
			lightNames.push_back(pair.first);
			flatLights.push_back(pair.second);
		}

		if (!flatLights.empty()) {
			size_t length = flatLights.size() * sizeof(pod::Light);
			uf::stl::string binName = "lights.bin";
			uf::io::write(directory + "/" + binName, flatLights.data(), length);

			serializer["lights"]["names"] = lightNames;
			serializer["lights"]["buffer"] = binName;
			serializer["lights"]["count"] = flatLights.size();
			serializer["lights"]["length"] = length;
		}
	});

	tasks.queue([&]{
		uf::stl::vector<pod::Primitive> flatPrimitives;
		size_t totalPrimitives = 0;
		for ( auto& name : graph.primitives ) {
			totalPrimitives += storage.primitives.map.at(name).size();
		}
		flatPrimitives.reserve(totalPrimitives);

		ext::json::reserve( serializer["primitives"], graph.primitives.size() );

		for ( size_t i = 0; i < graph.primitives.size(); ++i ) {
			auto& name = graph.primitives[i];
			auto& primArray = storage.primitives.map.at(name);

			size_t byteOffset = flatPrimitives.size() * sizeof(pod::Primitive);
			size_t byteLength = primArray.size() * sizeof(pod::Primitive);

			flatPrimitives.insert(flatPrimitives.end(), primArray.begin(), primArray.end());

			auto& json = serializer["primitives"].emplace_back();
			json["name"] = name;
			json["count"] = primArray.size();
			json["offset"] = byteOffset;
			json["length"] = byteLength;
		}

		if ( !flatPrimitives.empty() ) {
			uf::stl::string binName = "primitives.bin";
			uf::io::write(directory + "/" + binName, flatPrimitives.data(), flatPrimitives.size() * sizeof(pod::Primitive));
			serializer["metadata"]["buffers"]["primitives"] = binName;
		}
	});

	tasks.queue([&]{
		ext::json::reserve( serializer["meshes"], graph.meshes.size() );

		uf::stl::vector<uint8_t> meshesBuffer;
		uf::stl::string binName = "meshes." + (settings.compression == "none" ? "bin" : settings.compression);

		for ( auto& name : graph.meshes ) {
			auto& mesh = storage.meshes.map.at(name);
			auto json = encode(mesh, settings, graph, meshesBuffer, binName);
			json["name"] = name;
			serializer["meshes"].emplace_back(json);
		}

		if ( !meshesBuffer.empty() ) {
			uf::io::write(directory + "/" + binName, meshesBuffer);
		}
	});

	tasks.queue([&]{
		ext::json::reserve( serializer["animations"], graph.animations.size() );

		uf::stl::vector<uint8_t> animsBuffer;
		uf::stl::string binName = "animations.bin";

		for ( auto& name : graph.animations ) {
			auto& animation = storage.animations.map.at(name);
			serializer["animations"][name] = encode(animation, settings, graph, animsBuffer, binName);
		}

		if ( !animsBuffer.empty() ) {
			uf::io::write(directory + "/" + binName, animsBuffer);
		}
	});

	tasks.queue([&]{
		ext::json::reserve( serializer["skins"], graph.skins.size() );

		uf::stl::vector<uint8_t> skinsBuffer;
		uf::stl::string binName = "skins.bin";

		for ( auto& name : graph.skins ) {
			auto& skin = storage.skins.map.at(name);
			serializer["skins"].emplace_back( encode(skin, settings, graph, skinsBuffer, binName) );
		}

		if ( !skinsBuffer.empty() ) {
			uf::io::write(directory + "/" + binName, skinsBuffer);
		}
	});

	tasks.queue([&]{
		ext::json::reserve( serializer["images"], graph.images.size() );

		uf::stl::vector<uint8_t> imagesBuffer;
		uf::stl::vector<uint8_t> dtexBuffer;
		uf::stl::string binName = "images.bin";
		uf::stl::string dtexBinName = "images.dtex.bin";

		for ( size_t i = 0; i < graph.images.size(); ++i ) {
			auto& name = graph.images[i];
			auto& image = storage.images.map.at(name).data;
			uf::Serializer json;
			json["name"] = name;

			if ( !settings.combined ) {
				uf::stl::string f = ::fmt::format("image.{}.png", i );
				image.save(::fmt::format("{}/{}", directory, f));
				json["filename"] = f;
			} else {
				uf::stl::vector<uint8_t> pngBytes;
				image.save( pngBytes );

				size_t offset = imagesBuffer.size();
				size_t length = pngBytes.size();
				imagesBuffer.insert(imagesBuffer.end(), pngBytes.begin(), pngBytes.end());

				json["filename"] = binName;
				json["offset"] = offset;
				json["length"] = length;
			}

		#if UF_USE_DC_TEXCONV
			auto converted = image.scale( {32, 32}, "nearest" );
			uf::stl::vector<uint8_t> dtexBytes;
			auto dtex = ext::texconv::convert( converted );
			ext::texconv::save( dtex, dtexBytes );

			size_t dtexOffset = dtexBuffer.size();
			size_t dtexLength = dtexBytes.size();
			dtexBuffer.insert(dtexBuffer.end(), dtexBytes.begin(), dtexBytes.end());

			json["dtex"]["filename"] = dtexBinName;
			json["dtex"]["offset"] = dtexOffset;
			json["dtex"]["length"] = dtexLength;
		#endif

			serializer["images"].emplace_back( json );
		}

		if ( settings.combined && !imagesBuffer.empty() ) {
			uf::io::write(directory + "/" + binName, imagesBuffer);
		}

	#if UF_USE_DC_TEXCONV
		if ( !dtexBuffer.empty() ) {
			uf::io::write(directory + "/" + dtexBinName, dtexBuffer);
		}
	#endif
	});

	tasks.queue([&]{
		ext::json::reserve( serializer["nodes"], graph.nodes.size() );
		for ( auto& node : graph.nodes ) serializer["nodes"].emplace_back( encode(node, settings, graph) );
		serializer["root"] = encode(graph.root, settings, graph);
	});

	uf::thread::execute( tasks );

	if ( !settings.combined ) target = directory + "/graph.json";
	serializer.writeToFile( target );
	UF_MSG_DEBUG("Saved graph to {}", target);

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