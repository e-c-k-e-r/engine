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
#include <uf/ext/valve/bsp.h>
#include <uf/utils/io/fmt.h>

#define UF_GRAPH_LOAD_MULTITHREAD 0
#define UF_GRAPH_EXTENDED 1

#if UF_ENV_DREAMCAST
	#define UF_DEBUG_TIMER_MULTITRACE_START(...) UF_TIMER_MULTITRACE_START(__VA_ARGS__)
	#define UF_DEBUG_TIMER_MULTITRACE(...) UF_TIMER_MULTITRACE(__VA_ARGS__)
	#define UF_DEBUG_TIMER_MULTITRACE_END(...) UF_TIMER_MULTITRACE_END(__VA_ARGS__)
#else
	#define UF_DEBUG_TIMER_MULTITRACE_START(...)
	#define UF_DEBUG_TIMER_MULTITRACE(...)
	#define UF_DEBUG_TIMER_MULTITRACE_END(...)
#endif

namespace {
	uf::Image decodeImage( ext::json::Value& json, pod::Graph& graph, const uf::stl::string& imageName ) {
		uf::Image image;

		uf::stl::string filename = "";
		size_t offset = 0, length = 0, layers = json["layers"].as<size_t>(1);
		uf::stl::string formatHint = "";

	#if UF_ENV_DREAMCAST
		if (json["dtex"].isObject()) {
			filename = json["dtex"]["filename"].as<uf::stl::string>();
			offset = json["dtex"]["offset"].as<size_t>();
			length = json["dtex"]["length"].as<size_t>();
			formatHint = "dtex";
		} else
	#endif
		if ( json["filename"].is<uf::stl::string>() ) {
			filename = json["filename"].as<uf::stl::string>();
			offset = json["offset"].as<size_t>(0);
			length = json["length"].as<size_t>(0);
			formatHint = uf::io::extension(filename);
		} else {
			auto size = uf::vector::decode( json["size"], pod::Vector2ui{} );
			size_t bpp = json["bpp"].as<size_t>();
			size_t channels = json["channels"].as<size_t>();
			auto pixels = uf::base64::decode( json["data"].as<uf::stl::string>() );
			image.loadFromBuffer( &pixels[0], size, bpp, channels, true );
			image.setLayers( layers );
			return image;
		}

		uf::stl::string fullPath = uf::io::directory( graph.name ) + "/" + filename;

		if ( graph.settings.stream.textures ) {
			auto& storage = uf::graph::getStorage(graph);
			graph.streams.images[imageName] = { fullPath, offset, length };
			image.setFilename(fullPath);
		} else {
			uf::stl::vector<uint8_t> buffer;
			if (length > 0) {
				uf::io::readAsBuffer(buffer, fullPath, offset, length);
			} else {
				uf::io::readAsBuffer(buffer, fullPath);
			}

			uf::image::open( image, buffer, formatHint, false );
			uf::image::layers( image, layers );
			image.setFilename(fullPath);
		}

		return image;
	}

	pod::Animation decodeAnimation( ext::json::Value& json, pod::Graph& graph, const uf::stl::string& animName, const uf::stl::vector<uint8_t>& megaBuffer ) {
		pod::Animation animation = {};
		animation.name = json["name"].as(animation.name);
		animation.start = json["start"].as<float>(0.0f);
		animation.end = json["end"].as<float>(1.0f);

		uf::stl::string binPath = "";
		if (json["buffer"].is<uf::stl::string>()) {
			binPath = uf::io::directory(graph.name) + "/" + json["buffer"].as<uf::stl::string>();
		}

		auto& storage = uf::graph::getStorage(graph);
		auto& animStream = graph.streams.animations[animName];

		 ext::json::forEach( json["samplers"], [&]( ext::json::Value& value ){
			auto& sampler = animation.samplers.emplace_back();
			sampler.interpolator = value["interpolator"].as(sampler.interpolator);

			size_t inputsCount = value["inputs"]["count"].as<size_t>();
			size_t inputsOffset = value["inputs"]["offset"].as<size_t>();
			size_t inputsLen = value["inputs"]["length"].as<size_t>();

			size_t outputsCount = value["outputs"]["count"].as<size_t>();
			size_t outputsOffset = value["outputs"]["offset"].as<size_t>();
			size_t outputsLen = value["outputs"]["length"].as<size_t>();

			if ( graph.settings.stream.animations ) {
				pod::AnimationStream::SamplerStream sStream;
				sStream.inputs = { binPath, inputsOffset, inputsLen };
				sStream.outputs = { binPath, outputsOffset, outputsLen };
				animStream.samplers.emplace_back(sStream);
			} else {
				if (inputsLen > 0 && !megaBuffer.empty()) {
					sampler.inputs.resize(inputsCount);
					memcpy(sampler.inputs.data(), megaBuffer.data() + inputsOffset, inputsLen);
				}
				if (outputsLen > 0 && !megaBuffer.empty()) {
					sampler.outputs.resize(outputsCount);
					memcpy(sampler.outputs.data(), megaBuffer.data() + outputsOffset, outputsLen);
				}
			}
		});

		ext::json::forEach( json["channels"], [&]( ext::json::Value& value ){
			auto& channel = animation.channels.emplace_back();
			channel.path = value["path"].as(channel.path);
			channel.node = value["node"].as(channel.node);
			channel.sampler = value["sampler"].as(channel.sampler);
		});

		return animation;
	}

	pod::Skin decodeSkin( ext::json::Value& json, pod::Graph& graph, const uf::stl::string& skinName, const uf::stl::vector<uint8_t>& megaBuffer ) {
		pod::Skin skin;

		skin.name = json["name"].as(skin.name);

		skin.joints.reserve( json["joints"].size() );
		ext::json::forEach( json["joints"], [&]( ext::json::Value& value ){
			skin.joints.emplace_back( value.as<int32_t>() );
		});

		if (json["inverseBindMatrices"].isObject()) {
			auto& invJson = json["inverseBindMatrices"];
			size_t count = invJson["count"].as<size_t>();
			size_t offset = invJson["offset"].as<size_t>();
			size_t length = invJson["length"].as<size_t>();
			uf::stl::string binPath = uf::io::directory(graph.name) + "/" + invJson["buffer"].as<uf::stl::string>();

			auto& storage = uf::graph::getStorage(graph);
			auto& skinStream = graph.streams.skins[skinName];

			if ( graph.settings.stream.enabled ) {
				skinStream.inverseBindMatrices = { binPath, offset, length };
			} else {
				if (length > 0 && !megaBuffer.empty()) {
					skin.inverseBindMatrices.resize(count);
					memcpy(skin.inverseBindMatrices.data(), megaBuffer.data() + offset, length);
				}
			}
		}

		return skin;
	}

	uf::Mesh decodeMesh( ext::json::Value& json, pod::Graph& graph, const uf::stl::string& meshName, const uf::stl::vector<uint8_t>& megaBuffer ) {
		uf::Mesh mesh;

		#define DESERIALIZE_MESH(N) {\
			auto& input = json["inputs"][#N];\
			mesh.N.attributes.reserve( input["attributes"].size() );\
			mesh.N.count = input["count"].as( mesh.N.count );\
			mesh.N.first = input["first"].as( mesh.N.first );\
			mesh.N.size = input["size"].as( mesh.N.size );\
			mesh.N.offset = input["offset"].as( mesh.N.offset );\
			ext::json::forEach( input["attributes"], [&]( ext::json::Value& value ){\
				auto& attribute = mesh.N.attributes.emplace_back();\
				attribute.descriptor.offset = value["descriptor"]["offset"].as(attribute.descriptor.offset);\
				attribute.descriptor.size = value["descriptor"]["size"].as(attribute.descriptor.size);\
				attribute.descriptor.format = (uf::renderer::enums::Format::type_t) value["descriptor"]["format"].as<size_t>(attribute.descriptor.format);\
				attribute.descriptor.name = value["descriptor"]["name"].as(attribute.descriptor.name);\
				attribute.descriptor.type = (uf::renderer::enums::Type::type_t) value["descriptor"]["type"].as(attribute.descriptor.type);\
				attribute.descriptor.components = value["descriptor"]["components"].as(attribute.descriptor.components);\
				attribute.buffer = value["buffer"].as(attribute.buffer);\
				attribute.offset = value["offset"].as(attribute.offset);\
				attribute.stride = value["stride"].as(attribute.stride);\
				attribute.length = value["length"].as(attribute.length);\
			});\
		}

		DESERIALIZE_MESH(vertex);
		DESERIALIZE_MESH(index);
		DESERIALIZE_MESH(instance);
		DESERIALIZE_MESH(indirect);
		#undef DESERIALIZE_MESH

		auto& storage = uf::graph::getStorage(graph);
		auto& meshStream = graph.streams.meshes[meshName];

		mesh.buffers.reserve( json["buffers"].size() );

		uf::stl::vector<pod::StreamRegion> localRegions;
		localRegions.reserve( json["buffers"].size() );

		ext::json::forEach( json["buffers"], [&]( ext::json::Value& value ){
			uf::stl::string filename;
			size_t offset = 0, length = 0;

			if (value.isObject()) {
				filename = value["filename"].as<uf::stl::string>();
				offset = value["offset"].as<size_t>();
				length = value["length"].as<size_t>();
			} else {
				filename = value.as<uf::stl::string>();
			}

			uf::stl::string fullPath = uf::io::directory( graph.name ) + "/" + filename;
			pod::StreamRegion region = { fullPath, offset, length };

			if ( graph.settings.stream.enabled ) {
				mesh.buffers.emplace_back();
				meshStream.buffers.push_back(region);
			} else {
				uf::stl::vector<uint8_t> buf;
				if (length > 0 && !megaBuffer.empty()) {
					buf.assign(megaBuffer.begin() + offset, megaBuffer.begin() + offset + length);
				} else if (length > 0) {
					uf::io::readAsBuffer(buf, fullPath, offset, length);
				} else {
					uf::io::readAsBuffer(buf, fullPath);
				}
				mesh.buffers.emplace_back(std::move(buf));
				localRegions.push_back(region);
			}
		});

		auto getRegion = [&](size_t bufferIdx) -> pod::StreamRegion {
			if ( graph.settings.stream.enabled ) return meshStream.buffers[bufferIdx];
			return localRegions[bufferIdx];
		};

		for ( size_t i = 0; i < mesh.instance.attributes.size(); ++i ) {
			auto& attr = mesh.instance.attributes[i];
			if ( !mesh.buffers[attr.buffer].empty() ) continue;
			auto region = getRegion(attr.buffer);
			uf::io::readAsBuffer(mesh.buffers[attr.buffer], region.filename, region.offset, region.length);
		}
		for ( size_t i = 0; i < mesh.indirect.attributes.size(); ++i ) {
			auto& attr = mesh.indirect.attributes[i];
			if ( !mesh.buffers[attr.buffer].empty() ) continue;
			auto region = getRegion(attr.buffer);
			uf::io::readAsBuffer(mesh.buffers[attr.buffer], region.filename, region.offset, region.length);
		}

		#if UF_ENV_DREAMCAST
		// remove extraneous buffers
		// if ( graph.metadata["renderer"]["separate"].as<bool>() )
		{
			uf::stl::vector<uf::stl::string> attributesKept = ext::json::vector<uf::stl::string>(graph.metadata["decode"]["attributes"]);
			uf::stl::vector<size_t> remove; remove.reserve(mesh.vertex.attributes.size());

			for ( size_t i = 0; i < mesh.vertex.attributes.size(); ++i ) {
				auto& attribute = mesh.vertex.attributes[i];
				if ( std::find( attributesKept.begin(), attributesKept.end(), attribute.descriptor.name ) != attributesKept.end() ) continue;
				remove.insert(remove.begin(), i);
			}
			for ( auto& i : remove ) {
				mesh.buffers[mesh.vertex.attributes[i].buffer].clear();
				mesh.buffers[mesh.vertex.attributes[i].buffer].shrink_to_fit();
				mesh.vertex.attributes.erase(mesh.vertex.attributes.begin() + i);
			}
		}
	#endif

		// if ( graph.metadata["renderer"]["separate"].as<bool>() )
		{
		#if UF_ENV_DREAMCAST && GL_QUANTIZED_SHORT
			mesh.convert<float, uint16_t>();
		#else
			auto conversion = graph.metadata["decode"]["conversion"].as<uf::stl::string>();
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
			.metadata = json["metadata"],
		};

		node.children.reserve( json["children"].size() );
		ext::json::forEach( json["children"], [&]( ext::json::Value& value ){
			node.children.emplace_back( value.as<int32_t>() );
		});
		return node;
	}
}

void uf::graph::load( pod::Graph& graph, const uf::stl::string& filename, const uf::Serializer& metadata ) {
	const uf::stl::string extension = uf::io::extension( filename );
#if UF_USE_GLTF
	if ( extension == "glb" || extension == "gltf" ) {
		return ext::gltf::load( graph, filename, metadata );
	}
#endif
	if ( extension == "bsp" ) {
		return ext::valve::loadBsp( graph, filename, metadata );
	}
	const uf::stl::string directory = uf::io::directory( filename ) + "/";
	uf::Serializer serializer;
	UF_DEBUG_TIMER_MULTITRACE_START("Reading {}", filename);
	serializer.readFromFile( filename );
	graph.name = filename;
	graph.metadata = metadata;
	graph.metadata.merge( serializer["metadata"], true );

#if UF_GRAPH_LOAD_MULTITHREAD
	auto tasks = uf::thread::schedule(true);
#else
	auto tasks = uf::thread::schedule(false);
#endif

	if ( !graph.storage ) graph.storage = new pod::Graph::Storage();
	auto& storage = uf::graph::getStorage( graph ); // will just fetch the above

	if ( !ext::json::isArray(graph.metadata["decode"]["attributes"]) ) {
	#if UF_USE_OPENGL
		graph.metadata["decode"]["attributes"] = uf::stl::vector<uf::stl::string>({ "position", "uv", "st" });
	#else
		graph.metadata["decode"]["attributes"] = uf::stl::vector<uf::stl::string>({ "position", "color", "uv", "st", "normal", "tangent", "joints", "weights" });
	#endif
	}

	// failsafes
	if ( graph.metadata["stream"]["enabled"].is<uf::stl::string>() && graph.metadata["stream"]["enabled"].as<uf::stl::string>() == "auto" ) {
	#if UF_ENV_DREAMCAST
		graph.metadata["stream"]["enabled"] = true;
	#else
		graph.metadata["stream"]["enabled"] = false;
	#endif
	}
	if ( graph.metadata["stream"]["enabled"].as<bool>() && graph.metadata["stream"]["radius"].as<float>(0) <= 0.0f ) {
		graph.metadata["stream"]["enabled"] = false;
	}
	if ( !graph.metadata["stream"]["enabled"].as<bool>() ) {
		graph.metadata["stream"]["radius"] = 0.0f;
	}

	// copy important settings
	{
		graph.settings.stream.enabled = graph.metadata["stream"]["enabled"].as(graph.settings.stream.enabled);
		
		graph.settings.stream.textures = graph.settings.stream.enabled && graph.metadata["stream"]["textures"].as(graph.settings.stream.textures);
		graph.settings.stream.animations = graph.settings.stream.enabled && graph.metadata["stream"]["animations"].as(graph.settings.stream.animations);

		graph.settings.stream.radius = graph.metadata["stream"]["radius"].as(graph.settings.stream.radius);
		graph.settings.stream.every = graph.metadata["stream"]["every"].as(graph.settings.stream.every);

		graph.settings.stream.tag = graph.metadata["stream"]["tag"].as(graph.settings.stream.tag);
		graph.settings.stream.player = graph.metadata["stream"]["player"].as(graph.settings.stream.player);

		graph.settings.stream.hash = graph.metadata["stream"]["hash"].as(graph.settings.stream.hash);
		graph.settings.stream.lastUpdate = graph.metadata["stream"]["lastUpdate"].as(graph.settings.stream.lastUpdate);
	}
	// store offsets
	{
		size_t instances = 0;
		for ( auto& key : storage.primitives.keys ) {
			instances += storage.primitives.map[key].size();
		}

		graph.metadata["offsets"]["instances"] = instances;
		graph.metadata["offsets"]["materials"] = storage.materials.keys.size();
		graph.metadata["offsets"]["joints"] = storage.joints.keys.size();
	}

	uf::stl::string key = graph.metadata["key"].as<uf::stl::string>("");
	if ( key != "" ) key += ":";

	tasks.queue([&]{
		UF_DEBUG_TIMER_MULTITRACE("Reading material information...");
		auto& node = serializer["materials"];
		if (node.isObject() && node["buffer"].is<uf::stl::string>()) {
			uf::stl::string binName = node["buffer"].as<uf::stl::string>();
			uf::stl::vector<uint8_t> ioBuf;

			if (uf::io::readAsBuffer(ioBuf, directory + binName)) {
				pod::Material* rawMaterials = reinterpret_cast<pod::Material*>(ioBuf.data());
				auto names = node["names"].as<uf::stl::vector<uf::stl::string>>();

				graph.materials.reserve(names.size());
				for (size_t i = 0; i < names.size(); ++i) {
					auto name = key + names[i];
					storage.materials[name] = rawMaterials[i];
					graph.materials.emplace_back(name);
				}
			}
		}
		UF_DEBUG_TIMER_MULTITRACE("Read material information");
	});

	tasks.queue([&]{
		UF_DEBUG_TIMER_MULTITRACE("Reading texture information...");
		auto& node = serializer["textures"];
		if (node.isObject() && node["buffer"].is<uf::stl::string>()) {
			uf::stl::string binName = node["buffer"].as<uf::stl::string>();
			uf::stl::vector<uint8_t> ioBuf;

			if (uf::io::readAsBuffer(ioBuf, directory + binName)) {
				pod::Texture* rawTextures = reinterpret_cast<pod::Texture*>(ioBuf.data());
				auto names = node["names"].as<uf::stl::vector<uf::stl::string>>();

				graph.textures.reserve(names.size());
				for (size_t i = 0; i < names.size(); ++i) {
					auto name = key + names[i];
					storage.textures[name] = rawTextures[i];
					graph.textures.emplace_back(name);
				}
			}
		}
		UF_DEBUG_TIMER_MULTITRACE("Read texture information");
	});

	tasks.queue([&]{
		UF_DEBUG_TIMER_MULTITRACE("Reading sampler information...");
		auto& node = serializer["samplers"];
		if (node.isObject() && node["buffer"].is<uf::stl::string>()) {
			uf::stl::string binName = node["buffer"].as<uf::stl::string>();
			uf::stl::vector<uint8_t> ioBuf;

			if (uf::io::readAsBuffer(ioBuf, directory + binName)) {
				uf::renderer::Sampler* rawSamplers = reinterpret_cast<uf::renderer::Sampler*>(ioBuf.data());
				auto names = node["names"].as<uf::stl::vector<uf::stl::string>>();

				graph.samplers.reserve(names.size());
				for (size_t i = 0; i < names.size(); ++i) {
					auto name = key + names[i];
					storage.samplers[name] = rawSamplers[i];
					graph.samplers.emplace_back(name);
				}
			}
		}
		UF_DEBUG_TIMER_MULTITRACE("Read sampler information");
	});

	tasks.queue([&]{
		UF_DEBUG_TIMER_MULTITRACE("Reading lighting information...");
		auto& node = serializer["lights"];
		if (node.isObject() && node["buffer"].is<uf::stl::string>()) {
			uf::stl::string binName = node["buffer"].as<uf::stl::string>();
			uf::stl::vector<uint8_t> ioBuf;

			if (uf::io::readAsBuffer(ioBuf, directory + binName)) {
				pod::Light* rawLights = reinterpret_cast<pod::Light*>(ioBuf.data());
				auto names = node["names"].as<uf::stl::vector<uf::stl::string>>();

				graph.lights.reserve(names.size());
				for (size_t i = 0; i < names.size(); ++i) {
					auto name = key + names[i];
					graph.lights[name] = rawLights[i];
				}
			}
		}
		UF_DEBUG_TIMER_MULTITRACE("Read lighting information");
	});
	tasks.queue([&]{
		UF_DEBUG_TIMER_MULTITRACE("Reading primitives...");

		uf::stl::string binName = "primitives.bin";
		uf::stl::vector<uint8_t> ioBuf;
		pod::Primitive* allPrimitives = nullptr;

		if (uf::io::readAsBuffer(ioBuf, directory + binName)) {
			allPrimitives = reinterpret_cast<pod::Primitive*>(ioBuf.data());
		}

		auto& primNode = serializer["primitives"];
		graph.primitives.reserve( primNode.size() );

		ext::json::forEach( primNode, [&]( ext::json::Value& value ){
			auto name = key + value["name"].as<uf::stl::string>();

			graph.primitives.emplace_back(name);

			bool hasOffset = !value["offset"].isNull();

			if (allPrimitives && hasOffset) {
				size_t count = value["count"].as<size_t>();
				size_t offsetBytes = value["offset"].as<size_t>();
				size_t startIndex = offsetBytes / sizeof(pod::Primitive);

				storage.primitives[name].assign(&allPrimitives[startIndex], &allPrimitives[startIndex + count]);
			} else {
				UF_MSG_WARNING("Primitive '{}' missing binary data. Buffer Loaded: {} | Offset in JSON: {}",
					name, (allPrimitives != nullptr), hasOffset);
			}
		});
		UF_DEBUG_TIMER_MULTITRACE("Read primitives.");
	});

	tasks.queue([&]{
		UF_DEBUG_TIMER_MULTITRACE("Reading images...");
		graph.images.reserve( serializer["images"].size() );
		ext::json::forEach( serializer["images"], [&]( ext::json::Value& value ){
			auto name = key + value["name"].as<uf::stl::string>();

			storage.images[name] = {
				.data = decodeImage( value, graph, name ),
			};
			graph.images.emplace_back(name);
		});
		UF_DEBUG_TIMER_MULTITRACE("Read images");
	});

	tasks.queue([&]{
		UF_DEBUG_TIMER_MULTITRACE("Reading meshes...");
		graph.meshes.reserve( serializer["meshes"].size() );

		uf::stl::vector<uint8_t> megaBuffer;
		bool bufferAttempted = false;

		ext::json::forEach( serializer["meshes"], [&]( ext::json::Value& value ){
			auto name = key + value["name"].as<uf::stl::string>();

			if (!bufferAttempted && value["buffers"].size() > 0 && value["buffers"][0].isObject()) {
				uf::stl::string binName = value["buffers"][0]["filename"].as<uf::stl::string>();
				uf::io::readAsBuffer(megaBuffer, directory + binName);
				bufferAttempted = true;
			}

			storage.meshes[name] = decodeMesh( value, graph, name, megaBuffer );
			graph.meshes.emplace_back(name);
		});
		UF_DEBUG_TIMER_MULTITRACE("Read meshes");
	});

	tasks.queue([&]{
		UF_DEBUG_TIMER_MULTITRACE("Reading animation information...");
		auto& animNode = serializer["animations"];

		uf::stl::vector<uint8_t> megaBuffer;
		bool bufferAttempted = false;

		if (animNode.isObject()) {
			storage.animations.map.reserve( animNode.size() );
			ext::json::forEach( animNode, [&]( const uf::stl::string& rawName, ext::json::Value& value ){
				auto name = key + rawName;

				if (!bufferAttempted && value["buffer"].is<uf::stl::string>()) {
					uf::stl::string binName = value["buffer"].as<uf::stl::string>();
					uf::io::readAsBuffer(megaBuffer, directory + binName);
					bufferAttempted = true;
				}

				storage.animations[name] = decodeAnimation( value, graph, name, megaBuffer );
				graph.animations.emplace_back(name);
			});
		}
		else if (animNode.isArray()) {
			storage.animations.map.reserve( animNode.size() );
			ext::json::forEach( animNode, [&]( ext::json::Value& value ){
				uf::stl::string path = directory + "/" + value.as<uf::stl::string>();
				uf::Serializer json;
				json.readFromFile( path );
				auto name = key + json["name"].as<uf::stl::string>();

				storage.animations[name] = decodeAnimation( json, graph, name, megaBuffer );
				graph.animations.emplace_back(name);
			});
		}
		UF_DEBUG_TIMER_MULTITRACE("Read animations");
	});

	tasks.queue([&]{
		UF_DEBUG_TIMER_MULTITRACE("Reading skinning information...");
		graph.skins.reserve( serializer["skins"].size() );

		uf::stl::vector<uint8_t> megaBuffer;
		bool bufferAttempted = false;

		ext::json::forEach( serializer["skins"], [&]( ext::json::Value& value ){
			auto name = key + value["name"].as<uf::stl::string>();

			if (!bufferAttempted && value["inverseBindMatrices"].isObject()) {
				uf::stl::string binName = value["inverseBindMatrices"]["buffer"].as<uf::stl::string>();
				uf::io::readAsBuffer(megaBuffer, directory + binName);
				bufferAttempted = true;
			}

			storage.skins[name] = decodeSkin( value, graph, name, megaBuffer );
			graph.skins.emplace_back(name);
		});
		UF_DEBUG_TIMER_MULTITRACE("Read skins");
	});

	tasks.queue([&]{
		UF_DEBUG_TIMER_MULTITRACE("Reading nodes...");
		graph.nodes.reserve( serializer["nodes"].size() );
		ext::json::forEach( serializer["nodes"], [&]( ext::json::Value& value ){
			graph.nodes.emplace_back(decodeNode( value, graph ));
		});
		auto entity = graph.root.entity;
		graph.root = decodeNode( serializer["root"], graph );
		graph.root.entity = entity;
		UF_DEBUG_TIMER_MULTITRACE("Read nodes");
	#if UF_ENV_DREAMCAST
		DC_STATS();
	#endif
	});

	uf::thread::execute( tasks );

	// re-reference all transform parents
	for ( auto& node : graph.nodes ) {
		if ( 0 <= node.parent && node.parent < graph.nodes.size() && node.index != node.parent ) {
			node.transform.reference = &graph.nodes[node.parent].transform;
		}
	}
	UF_DEBUG_TIMER_MULTITRACE_END("Processing graph...");


	// migrate
	if ( graph.storage && uf::graph::storageMode != pod::Graph::Storage::GRAPH ) {
		auto* pointer = graph.storage;
		graph.storage = NULL;
		auto& storage = *pointer;
		auto& target = uf::graph::getStorage( graph );
		uf::graph::import( target, storage );
		delete pointer;
	}

#if UF_ENV_DREAMCAST
	DC_STATS();
#endif
}