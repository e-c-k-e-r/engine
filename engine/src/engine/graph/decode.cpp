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

// it's too unstable right now to do multithreaded loading, perhaps there's a better way
#if UF_USE_OPENGL
	#define UF_GRAPH_LOAD_MULTITHREAD 0
#else
	#define UF_GRAPH_LOAD_MULTITHREAD 0
#endif

#define UF_GRAPH_EXTENDED 1

#if 0 && UF_ENV_DREAMCAST
	#define UF_DEBUG_TIMER_MULTITRACE_START(...) UF_TIMER_MULTITRACE_START(__VA_ARGS__)
	#define UF_DEBUG_TIMER_MULTITRACE(...) UF_TIMER_MULTITRACE(__VA_ARGS__)
	#define UF_DEBUG_TIMER_MULTITRACE_END(...) UF_TIMER_MULTITRACE_END(__VA_ARGS__)
#else
	#define UF_DEBUG_TIMER_MULTITRACE_START(...)
	#define UF_DEBUG_TIMER_MULTITRACE(...)
	#define UF_DEBUG_TIMER_MULTITRACE_END(...)
#endif

namespace {
	uf::Image decodeImage( ext::json::Value& json, pod::Graph& graph ) {
		uf::Image image;

		if ( json["filename"].is<uf::stl::string>() ) {
			const uf::stl::string directory = uf::io::directory( graph.name );
			const uf::stl::string filename = uf::io::filename( json["filename"].as<uf::stl::string>() );
			const uf::stl::string name = directory + "/" + filename;
			if ( graph.settings.stream.textures ) {
				image.setFilename(name);
			} else {
				image.open(name, false);
			}
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

	#if UF_ENV_DREAMCAST
		sampler.descriptor.filter.min = uf::renderer::enums::Filter::NEAREST;
		sampler.descriptor.filter.mag = uf::renderer::enums::Filter::NEAREST;
	#endif
		
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
		instance.auxID = json["auxID"].as( instance.auxID );
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
		drawCommand.auxID = json["auxID"].as( drawCommand.auxID );
		drawCommand.materialID = json["materialID"].as( drawCommand.materialID );
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

		#define DESERIALIZE_MESH(N) {\
			auto& input = json["inputs"][#N];\
			mesh.N.attributes.reserve( input["attributes"].size() );\
			mesh.N.count = input["count"].as( mesh.N.count );\
			mesh.N.first = input["first"].as( mesh.N.first );\
			mesh.N.size = input["size"].as( mesh.N.size );\
			mesh.N.offset = input["offset"].as( mesh.N.offset );\
			mesh.N.interleaved = input["interleaved"].as( mesh.N.interleaved );\
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

		mesh.buffers.reserve( json["buffers"].size() );
		ext::json::forEach( json["buffers"], [&]( ext::json::Value& value ){
			const uf::stl::string filename = value.as<uf::stl::string>();
			const uf::stl::string directory = uf::io::directory( graph.name );
			// uf::io::readAsBuffer( directory + "/" + filename )
		#if !UF_GRAPH_EXTENDED
			mesh.buffers.emplace_back(uf::io::readAsBuffer( directory + "/" + filename ));
		#else
			if ( graph.metadata["stream"]["enabled"].as<bool>() ) {
				mesh.buffers.emplace_back();
				mesh.buffer_paths.emplace_back(directory + "/" + filename);
			} else {
				// to-do: make it work for interleaved meshes
				mesh.buffers.emplace_back(uf::io::readAsBuffer( directory + "/" + filename ));
			}
		#endif
		});

		// load non vertex/index buffers
		for ( size_t i = 0; i < mesh.instance.attributes.size(); ++i ) {
			auto& attribute = mesh.instance.attributes[i];
			if ( !mesh.buffers[attribute.buffer].empty() ) continue;
			mesh.buffers[attribute.buffer] = uf::io::readAsBuffer( mesh.buffer_paths[attribute.buffer] );
		}
		for ( size_t i = 0; i < mesh.indirect.attributes.size(); ++i ) {
			auto& attribute = mesh.indirect.attributes[i];
			if ( !mesh.buffers[attribute.buffer].empty() ) continue;
			mesh.buffers[attribute.buffer] = uf::io::readAsBuffer( mesh.buffer_paths[attribute.buffer] );
		}

	#if UF_ENV_DREAMCAST
		// remove extraneous buffers
		// if ( graph.metadata["renderer"]["separate"].as<bool>() )
		{
			uf::stl::vector<uf::stl::string> attributesKept = ext::json::vector<uf::stl::string>(graph.metadata["decode"]["attributes"]);
			if ( !mesh.isInterleaved() ) {
				uf::stl::vector<size_t> remove; remove.reserve(mesh.vertex.attributes.size());

				for ( size_t i = 0; i < mesh.vertex.attributes.size(); ++i ) {
					auto& attribute = mesh.vertex.attributes[i];
					if ( std::find( attributesKept.begin(), attributesKept.end(), attribute.descriptor.name ) != attributesKept.end() ) continue;
					remove.insert(remove.begin(), i);
					UF_MSG_DEBUG("Removing mesh attribute: {}", attribute.descriptor.name);
				}
				for ( auto& i : remove ) {
					mesh.buffers[mesh.vertex.attributes[i].buffer].clear();
					mesh.buffers[mesh.vertex.attributes[i].buffer].shrink_to_fit();
					mesh.vertex.attributes.erase(mesh.vertex.attributes.begin() + i);
				}
			} else {
				UF_MSG_DEBUG("Attribute removal requested yet mesh is interleaved, ignoring...");
			}
		}
	#endif

		// if ( graph.metadata["renderer"]["separate"].as<bool>() )
		{
		#if UF_ENV_DREAMCAST && GL_QUANTIZED_SHORT
			mesh.convert<float, uint16_t>();
			UF_MSG_DEBUG("Quantizing mesh to GL_QUANTIZED_SHORT");
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
			mesh.updateDescriptor();
		}

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

void uf::graph::load( pod::Graph& graph, const uf::stl::string& filename, const uf::Serializer& metadata ) {
	const uf::stl::string extension = uf::io::extension( filename );
#if UF_USE_GLTF
	if ( extension == "glb" || extension == "gltf" ) {
		return ext::gltf::load( graph, filename, metadata );
	}
#endif
	const uf::stl::string directory = uf::io::directory( filename ) + "/";
	uf::Serializer serializer;
	UF_DEBUG_TIMER_MULTITRACE_START("Reading {}", filename);
	serializer.readFromFile( filename );
	// load metadata
	graph.name = filename; //serializer["name"].as<uf::stl::string>();
//	graph.metadata = metadata; // serializer["metadata"];

//	UF_MSG_DEBUG("A: {}", serializer["metadata"].dump(1, '\t'));
//	UF_MSG_DEBUG("B: {}", metadata.dump(1, '\t'));
#if 0
	graph.metadata = serializer["metadata"];
	graph.metadata.merge( metadata, false );
#else
	graph.metadata = metadata;
	graph.metadata.merge( serializer["metadata"], true );
#endif
//	UF_MSG_DEBUG("C: {}", graph.metadata.dump(1, '\t'));

#if UF_GRAPH_LOAD_MULTITHREAD
	auto tasks = uf::thread::schedule(true);
#else
	auto tasks = uf::thread::schedule(false);
#endif

	auto& scene = uf::scene::getCurrentScene();
	auto& storage = uf::graph::globalStorage ? uf::graph::storage : scene.getComponent<pod::Graph::Storage>();

	if ( !ext::json::isArray(graph.metadata["decode"]["attributes"]) ) {
	#if UF_USE_OPENGL
		graph.metadata["decode"]["attributes"] = uf::stl::vector<uf::stl::string>({ "position", "uv", "st" });
	#else
		graph.metadata["decode"]["attributes"] = uf::stl::vector<uf::stl::string>({ "position", "color", "uv", "st", "tangent", "joints", "weights", "normal", "id" });
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

	uf::stl::string key = graph.metadata["key"].as<uf::stl::string>("");
	if ( key != "" ) {
		key += ":";
	}

	tasks.queue([&]{
		// load images
		UF_DEBUG_TIMER_MULTITRACE("Reading instances...");
		graph.instances.reserve( serializer["instances"].size() );
		ext::json::forEach( serializer["instances"], [&]( ext::json::Value& value ){
			auto name = key + value["name"].as<uf::stl::string>();
			// UF_MSG_DEBUG("{}", name);
			/*graph.storage*/storage.instances[name] = decodeInstance( value, graph );
			graph.instances.emplace_back(name);
		});
		UF_DEBUG_TIMER_MULTITRACE("Read instances");
	#if UF_ENV_DREAMCAST
		DC_STATS();
	#endif
	});
	tasks.queue([&]{
		// load images
		UF_DEBUG_TIMER_MULTITRACE("Reading primitives...");
		graph.primitives.reserve( serializer["primitives"].size() );
		ext::json::forEach( serializer["primitives"], [&]( ext::json::Value& value ){
			auto name = key + value["name"].as<uf::stl::string>();
			// UF_MSG_DEBUG("{}", name);
			/*graph.storage*/storage.primitives[name] = decodePrimitives( value, graph );
			graph.primitives.emplace_back(name);
		});
		UF_DEBUG_TIMER_MULTITRACE("Read primitives.");
	#if UF_ENV_DREAMCAST
		DC_STATS();
	#endif
	});
	tasks.queue([&]{
		// load images
		UF_DEBUG_TIMER_MULTITRACE("Reading drawCommands...");
		graph.drawCommands.reserve( serializer["drawCommands"].size() );
		ext::json::forEach( serializer["drawCommands"], [&]( ext::json::Value& value ){
			auto name = key + value["name"].as<uf::stl::string>();
			// UF_MSG_DEBUG("{}", name);
			/*graph.storage*/storage.drawCommands[name] = decodeDrawCommands( value, graph );
			graph.drawCommands.emplace_back(name);
		});
		UF_DEBUG_TIMER_MULTITRACE("Read drawCommands");
	#if UF_ENV_DREAMCAST
		DC_STATS();
	#endif
	});
	tasks.queue([&]{
		// load mesh information
		UF_DEBUG_TIMER_MULTITRACE("Reading meshes..."); 
		graph.meshes.reserve( serializer["meshes"].size() );
		ext::json::forEach( serializer["meshes"], [&]( ext::json::Value& value ){
			auto name = key + value["name"].as<uf::stl::string>();
			// UF_MSG_DEBUG("{}", name);
			/*graph.storage*/storage.meshes[name] = decodeMesh( value, graph );
			graph.meshes.emplace_back(name);
		});
		UF_DEBUG_TIMER_MULTITRACE("Read meshes"); 
	#if UF_ENV_DREAMCAST
		DC_STATS();
	#endif
	});
	tasks.queue([&]{
		// load images
		UF_DEBUG_TIMER_MULTITRACE("Reading images...");
		graph.images.reserve( serializer["images"].size() );
		ext::json::forEach( serializer["images"], [&]( ext::json::Value& value ){
			auto name = key + value["name"].as<uf::stl::string>();
			// UF_MSG_DEBUG("{}", name);
			/*graph.storage*/storage.images[name] = decodeImage( value, graph );
			graph.images.emplace_back(name);
		});
		UF_DEBUG_TIMER_MULTITRACE("Read images");
	#if UF_ENV_DREAMCAST
		DC_STATS();
	#endif
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
	tasks.queue([&]{
		// load texture information
		UF_DEBUG_TIMER_MULTITRACE("Reading texture information...");
		graph.textures.reserve( serializer["textures"].size() );
		ext::json::forEach( serializer["textures"], [&]( ext::json::Value& value ){
			auto name = key + value["name"].as<uf::stl::string>();
			// UF_MSG_DEBUG("{}", name);
			/*graph.storage*/storage.textures[name] = decodeTexture( value, graph );
			graph.textures.emplace_back(name);
		});
		UF_DEBUG_TIMER_MULTITRACE("Read texture information");
	#if UF_ENV_DREAMCAST
		DC_STATS();
	#endif
	});
	tasks.queue([&]{
		// load sampler information
		UF_DEBUG_TIMER_MULTITRACE("Reading sampler information...");
		graph.samplers.reserve( serializer["samplers"].size() );
		ext::json::forEach( serializer["samplers"], [&]( ext::json::Value& value ){
			auto name = key + value["name"].as<uf::stl::string>();
			// UF_MSG_DEBUG("{}", name);
			/*graph.storage*/storage.samplers[name] = decodeSampler( value, graph );
			graph.samplers.emplace_back(name);
		});
		UF_DEBUG_TIMER_MULTITRACE("Read sampler information");
	#if UF_ENV_DREAMCAST
		DC_STATS();
	#endif
	});
	tasks.queue([&]{
		// load material information
		UF_DEBUG_TIMER_MULTITRACE("Reading material information...");
		graph.materials.reserve( serializer["materials"].size() );
		ext::json::forEach( serializer["materials"], [&]( ext::json::Value& value ){
			auto name = key + value["name"].as<uf::stl::string>();
			// UF_MSG_DEBUG("{}", name);
			/*graph.storage*/storage.materials[name] = decodeMaterial( value, graph );
			graph.materials.emplace_back(name);
		});
		UF_DEBUG_TIMER_MULTITRACE("Read material information");
	#if UF_ENV_DREAMCAST
		DC_STATS();
	#endif
	});
	tasks.queue([&]{
		// load light information
		UF_DEBUG_TIMER_MULTITRACE("Reading lighting information...");
		graph.lights.reserve( serializer["lights"].size() );
		ext::json::forEach( serializer["lights"], [&]( ext::json::Value& value ){
			auto name = key + value["name"].as<uf::stl::string>();
			// UF_MSG_DEBUG("{}", name);
			graph.lights[name] = decodeLight( value, graph );
		});
		UF_DEBUG_TIMER_MULTITRACE("Read lighting information");
	#if UF_ENV_DREAMCAST
		DC_STATS();
	#endif
	});
#if 1
	tasks.queue([&]{
		// load animation information
		UF_DEBUG_TIMER_MULTITRACE("Reading animation information...");
		/*graph.storage*/storage.animations.map.reserve( serializer["animations"].size() );
		ext::json::forEach( serializer["animations"], [&]( ext::json::Value& value ){
			if ( value.is<uf::stl::string>() ) {
				auto path = directory + "/" + value.as<uf::stl::string>();
				uf::Serializer json;
				json.readFromFile( path );
				auto name = key + json["name"].as<uf::stl::string>();
				if ( graph.settings.stream.animations ) {
					/*graph.storage*/storage.animations[name].path = path;
				} else {
					/*graph.storage*/storage.animations[name] = decodeAnimation( json, graph );
				}
				graph.animations.emplace_back(name);
			} else {
				// UF_MSG_DEBUG("{}", name);
				if ( value["filename"].is<uf::stl::string>() ) {
					auto path = directory + "/" + value.as<uf::stl::string>();
					uf::Serializer json;
					json.readFromFile( path );

					auto name = key + json["name"].as<uf::stl::string>();
					if ( graph.settings.stream.animations ) {
						/*graph.storage*/storage.animations[name].path = path;
					} else {
						/*graph.storage*/storage.animations[name] = decodeAnimation( json, graph );
					}
					graph.animations.emplace_back(name);
				} else {
					auto name = key + value["name"].as<uf::stl::string>();
					/*graph.storage*/storage.animations[name] = decodeAnimation( value, graph );
					graph.animations.emplace_back(name);
				}
			}
		});
	#if UF_ENV_DREAMCAST
		DC_STATS();
	#endif
	});
	tasks.queue([&]{
		// load skin information
		UF_DEBUG_TIMER_MULTITRACE("Reading skinning information...");
		graph.skins.reserve( serializer["skins"].size() );
		ext::json::forEach( serializer["skins"], [&]( ext::json::Value& value ){
			auto name = key + value["name"].as<uf::stl::string>();
			// UF_MSG_DEBUG("{}", name);
			/*graph.storage*/storage.skins[name] = decodeSkin( value, graph );
			graph.skins.emplace_back(name);
		});
	#if UF_ENV_DREAMCAST
		DC_STATS();
	#endif
	});
#endif
	tasks.queue([&]{
		// load node information
		UF_DEBUG_TIMER_MULTITRACE("Reading nodes...");
		graph.nodes.reserve( serializer["nodes"].size() );
		ext::json::forEach( serializer["nodes"], [&]( ext::json::Value& value ){
			graph.nodes.emplace_back(decodeNode( value, graph ));
		});
		graph.root = decodeNode( serializer["root"], graph );
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

#if UF_ENV_DREAMCAST
	DC_STATS();
#endif
}