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

#if UF_ENV_DREAMCAST
	#define UF_GRAPH_LOAD_MULTITHREAD 0
#else
	#define UF_GRAPH_LOAD_MULTITHREAD 1
#endif

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
			/*graph.storage*/uf::graph::storage.instances[name] = decodeInstance( value, graph );
			graph.instances.emplace_back(name);
		});
	});
	jobs.emplace_back([&]{
		// load images
		UF_DEBUG_TIMER_MULTITRACE("Reading primitives...");
		graph.primitives.reserve( serializer["primitives"].size() );
		ext::json::forEach( serializer["primitives"], [&]( ext::json::Value& value ){
			auto name = value["name"].as<uf::stl::string>();
			/*graph.storage*/uf::graph::storage.primitives[name] = decodePrimitives( value, graph );
			graph.primitives.emplace_back(name);
		});
	});
	jobs.emplace_back([&]{
		// load images
		UF_DEBUG_TIMER_MULTITRACE("Reading drawCommands...");
		graph.drawCommands.reserve( serializer["drawCommands"].size() );
		ext::json::forEach( serializer["drawCommands"], [&]( ext::json::Value& value ){
			auto name = value["name"].as<uf::stl::string>();
			/*graph.storage*/uf::graph::storage.drawCommands[name] = decodeDrawCommands( value, graph );
			graph.drawCommands.emplace_back(name);
		});
	});
	jobs.emplace_back([&]{
		// load mesh information
		UF_DEBUG_TIMER_MULTITRACE("Reading meshes..."); 
		graph.meshes.reserve( serializer["meshes"].size() );
		ext::json::forEach( serializer["meshes"], [&]( ext::json::Value& value ){
			auto name = value["name"].as<uf::stl::string>();
			/*graph.storage*/uf::graph::storage.meshes[name] = decodeMesh( value, graph );
			graph.meshes.emplace_back(name);
		});
	});
	jobs.emplace_back([&]{
		// load images
		UF_DEBUG_TIMER_MULTITRACE("Reading images...");
		graph.images.reserve( serializer["images"].size() );
		ext::json::forEach( serializer["images"], [&]( ext::json::Value& value ){
			auto name = value["name"].as<uf::stl::string>();
			/*graph.storage*/uf::graph::storage.images[name] = decodeImage( value, graph );
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
			/*graph.storage*/uf::graph::storage.textures[name] = decodeTexture( value, graph );
			graph.textures.emplace_back(name);
		});
	});
	jobs.emplace_back([&]{
		// load sampler information
		UF_DEBUG_TIMER_MULTITRACE("Reading sampler information...");
		graph.samplers.reserve( serializer["samplers"].size() );
		ext::json::forEach( serializer["samplers"], [&]( ext::json::Value& value ){
			auto name = value["name"].as<uf::stl::string>();
			/*graph.storage*/uf::graph::storage.samplers[name] = decodeSampler( value, graph );
			graph.samplers.emplace_back(name);
		});
	});
	jobs.emplace_back([&]{
		// load material information
		UF_DEBUG_TIMER_MULTITRACE("Reading material information...");
		graph.materials.reserve( serializer["materials"].size() );
		ext::json::forEach( serializer["materials"], [&]( ext::json::Value& value ){
			auto name = value["name"].as<uf::stl::string>();
			/*graph.storage*/uf::graph::storage.materials[name] = decodeMaterial( value, graph );
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
		/*graph.storage*/uf::graph::storage.animations.map.reserve( serializer["animations"].size() );
		ext::json::forEach( serializer["animations"], [&]( ext::json::Value& value ){
			auto name = value["name"].as<uf::stl::string>();
			if ( value["filename"].is<uf::stl::string>() ) {
				uf::Serializer json;
				json.readFromFile( directory + "/" + value["filename"].as<uf::stl::string>() );
				/*graph.storage*/uf::graph::storage.animations[name] = decodeAnimation( json, graph );
			} else {
				/*graph.storage*/uf::graph::storage.animations[name] = decodeAnimation( value, graph );
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
			/*graph.storage*/uf::graph::storage.skins[name] = decodeSkin( value, graph );
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