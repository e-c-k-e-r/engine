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
#include <uf/ext/lgs/mis.h>
#include <uf/utils/io/fmt.h>
#include <uf/utils/math/physics/broadphase/bvh.h>

#define UF_GRAPH_LOAD_MULTITHREAD 0

#if UF_ENV_DREAMCAST
	#define UF_DEBUG_TIMER_MULTITRACE_START(...) UF_TIMER_MULTITRACE_START(__VA_ARGS__)
	#define UF_DEBUG_TIMER_MULTITRACE(...) UF_TIMER_MULTITRACE(__VA_ARGS__);
	#define UF_DEBUG_TIMER_MULTITRACE_END(...) UF_TIMER_MULTITRACE_END(__VA_ARGS__)
#else
	#define UF_DEBUG_TIMER_MULTITRACE_START(...)
	#define UF_DEBUG_TIMER_MULTITRACE(...)
	#define UF_DEBUG_TIMER_MULTITRACE_END(...)
#endif

namespace {
	struct PendingImage {
		uf::stl::string name;
		uf::stl::vector<uint8_t> buffer;
		uf::stl::string extension;
		size_t layers;
	};

	size_t deduceFormat( const uf::stl::string& format ) {
		if ( format == "ARGB4444" ) return uf::renderer::enums::Format::R4G4B4A4_UNORM_PACK16;
		if ( format == "RGB565" ) return uf::renderer::enums::Format::R5G6B5_UNORM_PACK16;
		return 0;
	}
	
	uf::Image decodeImage( ext::json::Value& json, pod::Graph& graph, const uf::stl::string& key ) {
		uf::Image image;

		uf::stl::string filename = "";
		size_t offset = 0;
		size_t length = 0;
		size_t format = 0;
		size_t layers = json["layers"].as<size_t>(1);
		uf::stl::string extension = "";

		if ( json["format"].is<uf::stl::string>() ) {
			format = deduceFormat( json["format"].as<uf::stl::string>() );
		} else {
			format = json["format"].as<size_t>();
		}
	#if UF_ENV_DREAMCAST
		if ( json["dtex"].isObject() ) {
			filename = json["dtex"]["filename"].as<uf::stl::string>();
			extension = "dtex";

			offset = json["dtex"]["offset"].as<size_t>();
			length = json["dtex"]["length"].as<size_t>();
			if ( json["dtex"]["format"].is<uf::stl::string>() ) {
				format = deduceFormat( json["dtex"]["format"].as<uf::stl::string>() );
			} else {
				format = json["dtex"]["format"].as<size_t>();
			}
		} else
	#endif
		if ( json["filename"].is<uf::stl::string>() ) {
			filename = json["filename"].as<uf::stl::string>();
			extension = uf::io::extension(filename);
			
			offset = json["offset"].as<size_t>(0);
			length = json["length"].as<size_t>(0);
		} else {
			auto size = uf::vector::decode( json["size"], pod::Vector2ui{} );
			size_t bpp = json["bpp"].as<size_t>();
			size_t channels = json["channels"].as<size_t>();
			auto pixels = uf::base64::decode( json["data"].as<uf::stl::string>() );
			image.loadFromBuffer( &pixels[0], size, bpp, channels, true );
			image.setLayers( layers );
			image.setFormat( format );
			return image;
		}

		uf::stl::string fullPath = uf::io::directory( graph.name ) + "/" + filename;

		if ( graph.settings.stream.textures ) {
			auto& storage = uf::graph::getStorage(graph);
			graph.streams.images[key] = { fullPath, offset, length };
		} else {
			size_t readLen = length > 0 ? length : uf::io::size( fullPath );
			if ( readLen > 0 ) {
				uf::asset::read( fullPath, offset, readLen, [&graph, key, extension, layers]( uf::stl::vector<uint8_t>&& buffer ) {
					auto& storage = uf::graph::getStorage(graph);
					auto& image = storage.images[key].data;

					uf::image::open( image, buffer, extension, false );
					uf::image::layers( image, layers );
				} );
			}
		}

		image.setFilename( fullPath );
		image.setFormat( format );

		return image;
	}

	pod::Animation decodeAnimation( ext::json::Value& json, pod::Graph& graph, const uf::stl::string& animName ) {
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
				if ( inputsLen > 0 ) {
					sampler.inputs.resize(inputsCount);
					uf::asset::read( binPath, inputsOffset, inputsLen, (uint8_t*)(sampler.inputs.data()) );
				}
				if ( outputsLen > 0 ) {
					sampler.outputs.resize(outputsCount);
					uf::asset::read( binPath, outputsOffset, outputsLen, (uint8_t*)(sampler.outputs.data()) );
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

	pod::Skin decodeSkin( ext::json::Value& json, pod::Graph& graph, const uf::stl::string& skinName ) {
		pod::Skin skin;

		skin.name = json["name"].as(skin.name);

		skin.joints.reserve( json["joints"].size() );
		ext::json::forEach( json["joints"], [&]( ext::json::Value& value ){
			skin.joints.emplace_back( value.as<int32_t>() );
		});

		if ( json["inverseBindMatrices"].isObject() ) {
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
				if ( length > 0 ) {
					skin.inverseBindMatrices.resize(count);
					uf::asset::read( binPath, offset, length, (uint8_t*)(skin.inverseBindMatrices.data()) );
				}
			}
		}

		return skin;
	}

	uf::Mesh decodeMesh( ext::json::Value& json, pod::Graph& graph, const uf::stl::string& meshName ) {
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
		bool deferred = graph.settings.stream.enabled;

		ext::json::forEach( json["buffers"], [&]( ext::json::Value& value ){
			uf::stl::string filename;
			size_t offset = 0, length = 0;

			if ( value.isObject() ) {
				filename = value["filename"].as<uf::stl::string>();
				offset = value["offset"].as<size_t>();
				length = value["length"].as<size_t>();
			} else {
				filename = value.as<uf::stl::string>();
			}
			
			uf::stl::string fullPath = uf::io::directory( graph.name ) + "/" + filename;

			mesh.buffers.emplace_back();
			meshStream.buffers.emplace_back(pod::StreamRegion{ fullPath, offset, length });
		});

		auto queue = [&]( auto& attributes ) {
			for ( auto& attr : attributes ) {
				if ( !mesh.buffers[attr.buffer].empty() ) continue;

				auto region = meshStream.buffers[attr.buffer];
				if ( region.length == 0 ) continue;

				mesh.buffers[attr.buffer].resize(region.length);
				uf::asset::read( region.filename, region.offset, region.length, mesh.buffers[attr.buffer].data() );
			}
		};

		queue( mesh.indirect.attributes );
		if ( !deferred ) {
			queue( mesh.instance.attributes );
			queue( mesh.vertex.attributes );
			queue( mesh.index.attributes );
		}

		mesh.updateDescriptor();
		return mesh;
	}

	pod::BVH decodeBvh( ext::json::Value& json, pod::Graph& graph, const uf::stl::string& key ) {
		pod::BVH bvh;
		auto& storage = uf::graph::getStorage(graph);
		auto& bvhStream = graph.streams.bvhs[key];

		uf::stl::string filename = json["filename"].as<uf::stl::string>();
		size_t offset = json["offset"].as<size_t>();
		size_t length = json["length"].as<size_t>();

		uf::stl::string fullPath = uf::io::directory( graph.name ) + "/" + filename;
		bvhStream.buffer = pod::StreamRegion{ fullPath, offset, length };

		bool deferred = graph.settings.stream.enabled;
		if ( !deferred ) {
			size_t readLen = length > 0 ? length : uf::io::size( fullPath );
			if ( readLen > 0 ) {
				uf::asset::read( fullPath, offset, readLen, [&graph, key]( uf::stl::vector<uint8_t>&& buffer ) {
					auto& storage = uf::graph::getStorage(graph);
					auto& bvh = storage.bvhs[key];
					uf::bvh::deserialize( bvh, buffer );
				} );
			}
		}

		return bvh;
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
	const uf::stl::string extension = uf::string::lowercase( uf::io::extension( filename ) );
#if UF_USE_GLTF
	if ( extension == "glb" || extension == "gltf" ) return ext::gltf::load( graph, filename, metadata );
#endif
#if UF_USE_VALVE
	if ( extension == "bsp" ) return ext::valve::loadBsp( graph, filename, metadata );
#endif
#if UF_USE_LGS
	if ( extension == "mis" ) return ext::lgs::loadMis( graph, filename, metadata );
#endif

	const uf::stl::string directory = uf::io::directory( filename ) + "/";
	uf::Serializer serializer;
	UF_DEBUG_TIMER_MULTITRACE_START("Reading {}", filename);
	serializer.readFromFile( filename );
	graph.name = filename;
	graph.metadata = metadata;
	graph.metadata.import( serializer["metadata"] );

#if UF_GRAPH_LOAD_MULTITHREAD
	auto tasks = uf::thread::schedule(true);
#else
	auto tasks = uf::thread::schedule(false);
#endif

	if ( !graph.storage ) graph.storage = new pod::Graph::Storage();
	auto& storage = uf::graph::getStorage( graph ); // will just fetch the above

#if 0
	if ( !ext::json::isArray(graph.metadata["decode"]["attributes"]) ) {
	#if UF_USE_OPENGL
		graph.metadata["decode"]["attributes"] = uf::stl::vector<uf::stl::string>({ "position", "uv", "st" });
	#else
		graph.metadata["decode"]["attributes"] = uf::stl::vector<uf::stl::string>({ "position", "color", "uv", "st", "normal", "tangent", "joints", "weights" });
	#endif
	}
#endif

	// failsafes
	if ( graph.metadata["stream"]["enabled"].is<uf::stl::string>() && graph.metadata["stream"]["enabled"].as<uf::stl::string>() == "auto" ) {
	#if UF_USE_OPENGL
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

	//	graph.settings.stream.world = graph.metadata["stream"]["tag"].as(graph.settings.stream.world);
	//	graph.settings.stream.player = graph.metadata["stream"]["player"].as(graph.settings.stream.player);

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
	
	uf::stl::vector<PendingImage> pendingImages;
	uf::stl::vector<uf::stl::string> meshesToMinify;

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

		if ( uf::io::readAsBuffer(ioBuf, directory + binName) ) {
			allPrimitives = reinterpret_cast<pod::Primitive*>(ioBuf.data());
		}

		auto& primNode = serializer["primitives"];
		graph.primitives.reserve( primNode.size() );

		ext::json::forEach( primNode, [&]( ext::json::Value& value ){
			auto name = key + value["name"].as<uf::stl::string>();

			graph.primitives.emplace_back(name);

			bool hasOffset = !value["offset"].isNull();

			if ( allPrimitives && hasOffset ) {
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

			//UF_DEBUG_TIMER_MULTITRACE("Reading image={}", name);
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

	#if UF_USE_OPENGL
		bool preferMinified = true;
	#else
		bool preferMinified = false;
	#endif

		ext::json::forEach( serializer["meshes"], [&]( ext::json::Value& value ){
			auto name = key + value["name"].as<uf::stl::string>();

			bool hasMinifiedAsset = value["min"].isObject();
			auto& json = ( preferMinified && hasMinifiedAsset ) ? value["min"] : value;

			storage.meshes[name] = decodeMesh( json, graph, name );
			graph.meshes.emplace_back(name);

			if ( preferMinified && !hasMinifiedAsset && !graph.settings.stream.enabled )
				meshesToMinify.emplace_back( name );
		});

		UF_DEBUG_TIMER_MULTITRACE("Read meshes");
	});

	tasks.queue([&]{
		UF_DEBUG_TIMER_MULTITRACE("Reading BVHs...");
		ext::json::forEach( serializer["bvhs"], [&]( ext::json::Value& value ){
			auto name = key + value["name"].as<uf::stl::string>();
			storage.bvhs[name] = decodeBvh( value, graph, name );
		});

		UF_DEBUG_TIMER_MULTITRACE("Read BVHs");
	});

	tasks.queue([&]{
		UF_DEBUG_TIMER_MULTITRACE("Reading animation information...");
		auto& animNode = serializer["animations"];

		if ( animNode.isObject() ) {
			storage.animations.map.reserve( animNode.size() );
			ext::json::forEach( animNode, [&]( const uf::stl::string& rawName, ext::json::Value& value ){
				auto name = key + rawName;
				storage.animations[name] = decodeAnimation( value, graph, name );
				graph.animations.emplace_back(name);
			});
		}
		else if ( animNode.isArray() ) {
			storage.animations.map.reserve( animNode.size() );
			ext::json::forEach( animNode, [&]( ext::json::Value& value ){
				uf::stl::string path = directory + "/" + value.as<uf::stl::string>();
				uf::Serializer json;
				json.readFromFile( path );
				auto name = key + json["name"].as<uf::stl::string>();

				storage.animations[name] = decodeAnimation( json, graph, name );
				graph.animations.emplace_back(name);
			});
		}
		UF_DEBUG_TIMER_MULTITRACE("Read animations");
	});

	tasks.queue([&]{
		UF_DEBUG_TIMER_MULTITRACE("Reading skinning information...");
		graph.skins.reserve( serializer["skins"].size() );

		ext::json::forEach( serializer["skins"], [&]( ext::json::Value& value ){
			auto name = key + value["name"].as<uf::stl::string>();
			storage.skins[name] = decodeSkin( value, graph, name );
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

	UF_DEBUG_TIMER_MULTITRACE("Executing tasks");
	uf::thread::execute( tasks );
	UF_DEBUG_TIMER_MULTITRACE("Processing IO");
	uf::asset::processIO();


	// process meshes that need to be minified because I can't easily tie it to the callback
	UF_DEBUG_TIMER_MULTITRACE("Processing meshes for minification");
	for ( auto& name : meshesToMinify ) {
		auto& mesh = storage.meshes[name];
		mesh.prune( { "position", "uv", "st" } );
		mesh.convert<float, uint16_t>();
		mesh.interleave();
	}

	// re-reference all transform parents
	UF_DEBUG_TIMER_MULTITRACE("Re-referencing nodes");
	for ( auto& node : graph.nodes ) {
		if ( 0 <= node.parent && node.parent < graph.nodes.size() && node.index != node.parent ) {
			node.transform.reference = &graph.nodes[node.parent].transform;
		}
	}


	// migrate
	UF_DEBUG_TIMER_MULTITRACE("Migrating storage");
	if ( graph.storage && uf::graph::storageMode != pod::Graph::Storage::GRAPH ) {
		auto* pointer = graph.storage;
		graph.storage = NULL;
		auto& storage = *pointer;
		auto& target = uf::graph::getStorage( graph );
		uf::graph::import( target, storage, true );
		delete pointer;
	}

	UF_DEBUG_TIMER_MULTITRACE_END("Loaded graph.");
#if UF_ENV_DREAMCAST
	DC_STATS();
#endif
}