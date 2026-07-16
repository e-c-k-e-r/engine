#include <uf/config.h>
#if UF_USE_GLTF
#define TINYGLTF_IMPLEMENTATION

#if UF_JSON_USE_NLOHMANN
	#define TINYGLTF_NO_INCLUDE_JSON
#endif
#if UF_USE_DRACO
	#define TINYGLTF_ENABLE_DRACO
#endif

#include <uf/utils/string/ext.h>

#include <uf/utils/thread/thread.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/image/atlas.h>
#include <uf/utils/string/hash.h>
#include <uf/utils/mesh/grid.h>
#include <uf/utils/io/vfs.h>

#include <gltf/tiny_gltf.h>
#include <uf/ext/gltf/gltf.h>

namespace {
	typedef uf::Meshlet_T<uf::graph::mesh::Skinned, uint32_t> Meshlet;

	decltype(auto) getWrapMode(int32_t wrapMode) {
		switch (wrapMode) {
			case 10497: return uf::renderer::enums::AddressMode::REPEAT;
			case 33071: return uf::renderer::enums::AddressMode::CLAMP_TO_EDGE;
			case 33648: return uf::renderer::enums::AddressMode::MIRRORED_REPEAT;
			default: return uf::renderer::enums::AddressMode::REPEAT;
		}
	}

	decltype(auto) getFilterMode(int32_t filterMode) {
		switch (filterMode) {
			case 9728: return uf::renderer::enums::Filter::NEAREST;
			case 9984: return uf::renderer::enums::Filter::NEAREST;
			case 9985: return uf::renderer::enums::Filter::NEAREST;
			case 9729: return uf::renderer::enums::Filter::LINEAR;
			case 9986: return uf::renderer::enums::Filter::LINEAR;
			case 9987: return uf::renderer::enums::Filter::LINEAR;
			default: return uf::renderer::enums::Filter::LINEAR;
		}
	}

	int32_t loadNode( const tinygltf::Model& model, pod::Graph& graph, int32_t nodeIndex, int32_t parentIndex );

	int32_t loadNodes( const tinygltf::Model& model, pod::Graph& graph, const std::vector<int>& nodes, int32_t nodeIndex ) {
		graph.nodes[nodeIndex].children.reserve( nodes.size() );
		for ( auto i : nodes ) {
			int32_t childIndex = loadNode( model, graph, i, nodeIndex );
			if ( 0 <= childIndex && childIndex < graph.nodes.size() && childIndex != nodeIndex &&
				std::find( graph.nodes[nodeIndex].children.begin(), graph.nodes[nodeIndex].children.end(), childIndex ) == graph.nodes[nodeIndex].children.end()
			 ) {
				graph.nodes[nodeIndex].children.emplace_back(childIndex);
			}
		}
		return nodeIndex;
	}
	int32_t loadNode( const tinygltf::Model& model, pod::Graph& graph, int32_t nodeIndex, int32_t parentIndex ) {
		if ( nodeIndex < 0 ) return nodeIndex;
		auto& node = model.nodes[nodeIndex];

		graph.nodes[nodeIndex].parent = parentIndex;
		graph.nodes[nodeIndex].index = nodeIndex;
		graph.nodes[nodeIndex].skin = node.skin;

		graph.nodes[nodeIndex].name = node.name;

		auto& transform = graph.nodes[nodeIndex].transform;
		if ( node.translation.size() == 3 ) {
			transform.position.x = node.translation[0];
			transform.position.y = node.translation[1];
			transform.position.z = node.translation[2];
		} else {
			transform.position = { 0, 0, 0 };
		}
		if ( graph.metadata["renderer"]["invert"].as<bool>(true) ) {
			transform.position.x *= -1;
		}
		if ( node.rotation.size() == 4 ) {
			transform.orientation.x = node.rotation[0];
			transform.orientation.y = node.rotation[1];
			transform.orientation.z = node.rotation[2];
			transform.orientation.w = node.rotation[3];
		} else {
			transform.orientation = { 0, 0, 0, 1 };
		}
		if ( node.scale.size() == 3 ) {
			transform.scale.x = node.scale[0];
			transform.scale.y = node.scale[1];
			transform.scale.z = node.scale[2];
		} else {
			transform.scale = { 1, 1, 1 };
		}
		/*
		if ( node.matrix.size() == 16 ) {
			for ( size_t i = 0; i < node.matrix.size(); ++i ) transform.model[i] = node.matrix[i];
		} else {
			transform.model = uf::matrix::identity();
		}
		*/
		if ( 0 <= parentIndex && parentIndex < graph.nodes.size() && nodeIndex != parentIndex ) {
			transform.reference = &graph.nodes[parentIndex].transform;
		}
		if ( node.children.size() > 0 ) {
			loadNodes( model, graph, node.children, nodeIndex );
		}
		if ( node.mesh > -1 ) {
			graph.nodes[nodeIndex].mesh = node.mesh;
		}
		return nodeIndex;
	}

	pod::Vector3f computeTangent( const pod::Vector3f& normal ) {
		pod::Vector3f up = ( std::fabs(normal.y) < 0.999f ) ? pod::Vector3f{0,1,0} : pod::Vector3f{1,0,0}; // pick a vector not parallel to normal
		pod::Vector3f tangent = uf::vector::normalize( uf::vector::cross( up, normal ) );
		return tangent;
	}
}

void ext::gltf::load( pod::Graph& graph, const uf::stl::string& filename, const uf::Serializer& metadata ) {
	uf::stl::string extension = uf::io::extension( filename );
	if ( extension != "glb" && extension != "gltf" ) {
		return uf::graph::load( graph, filename, metadata );
	}

	tinygltf::Model model;
	tinygltf::TinyGLTF loader;

	uf::stl::string warn, err;
	//bool ret = extension == "glb" ? loader.LoadBinaryFromFile(&model, &err, &warn, filename) : loader.LoadASCIIFromFile(&model, &err, &warn, filename);

	bool ret = false;
	if ( extension == "glb" ) {
	//	uf::stl::vector<uint8_t> buffer;
	//	if ( !uf::io::readAsBuffer( buffer, filename ) ) return;
	//	ret = loader.LoadBinaryFromMemory(&model, &err, &warn, buffer.data(), buffer.size());
		ret = loader.LoadBinaryFromFile(&model, &err, &warn, uf::vfs::resolveBase(filename));
	} else {
		// crunge
		ret = loader.LoadASCIIFromFile(&model, &err, &warn, uf::vfs::resolveBase(filename));
	}

	if ( !warn.empty() ) UF_MSG_WARNING("glTF warning: {}", warn);
	if ( !err.empty() ) UF_MSG_ERROR("glTF error: {}", err);
	if ( !ret ) { UF_MSG_ERROR("glTF error: failed to parse file: {}", filename);
		return;
	}

	uf::graph::preprocess( graph, metadata, filename );
	auto& storage = uf::graph::getStorage( graph );

	uf::stl::string key = graph.metadata["key"].as<uf::stl::string>("");
	if ( key != "" ) key += ":";

	// load images
	{
		graph.images.reserve(model.images.size());
		storage.images.reserve(model.images.size());

		for ( auto& i : model.images ) {
			auto imageID = graph.images.size();
			auto keyName = graph.images.emplace_back(key + i.name);
			auto& image = storage.images[keyName].data;
			if ( graph.metadata["debug"]["print"]["images"].as<bool>() ) {
				UF_MSG_DEBUG("Image: {}", i.name );
			}

			if ( i.component == 4 ) {
				image.loadFromBuffer( &i.image[0], {i.width, i.height}, 8, i.component, graph.metadata["renderer"]["flip textures"].as<bool>(true) );
			} else {
				const uint8_t* buffer = &i.image[0];
				pod::Image::container_t pixels;
				size_t len = i.width * i.height;
				pixels.resize(len * 4);
				for ( auto p = 0; p < len; ++p ) {
					if ( i.component == 1 ) {
						pixels[p * 4 + 0] = buffer[p]; // R
						pixels[p * 4 + 1] = buffer[p]; // G
						pixels[p * 4 + 2] = buffer[p]; // B
						pixels[p * 4 + 3] = 255; // A
					} else if ( i.component == 2 ) {
						pixels[p * 4 + 0] = buffer[p * 2 + 0];			   // R
						pixels[p * 4 + 1] = buffer[p * 2 + 0];			   // G
						pixels[p * 4 + 2] = buffer[p * 2 + 0];			   // B
						pixels[p * 4 + 3] = buffer[p * 2 + 1]; // A
					} else if ( i.component == 3 ) {
						pixels[p * 4 + 0] = buffer[p * 3 + 0]; // R
						pixels[p * 4 + 1] = buffer[p * 3 + 1]; // G
						pixels[p * 4 + 2] = buffer[p * 3 + 2]; // B
						pixels[p * 4 + 3] = 255;			   // A
					}
				}
				image.loadFromBuffer( &pixels[0], { i.width, i.height }, 8, 4, graph.metadata["renderer"]["flip textures"].as<bool>(true) );
			}
		}
	}
	// load samplers
	{
		storage.samplers.reserve(model.samplers.size());
		for ( auto& s : model.samplers ) {
			auto samplerID = graph.samplers.size();
			auto keyName = graph.samplers.emplace_back(key + s.name);
			auto& sampler = storage.samplers[keyName];
			if ( graph.metadata["debug"]["print"]["samplers"].as<bool>() ) {
				UF_MSG_DEBUG("Sampler: {}", s.name );
			}

			sampler.descriptor.filter.min = getFilterMode( s.minFilter );
			sampler.descriptor.filter.mag = getFilterMode( s.magFilter );
			sampler.descriptor.addressMode.u = getWrapMode( s.wrapS );
			sampler.descriptor.addressMode.v = getWrapMode( s.wrapT );
			sampler.descriptor.addressMode.w = sampler.descriptor.addressMode.v;
		}
	}
	// load textures
	{
		graph.textures.reserve(model.textures.size());
		storage.textures.reserve(model.textures.size());

		for ( auto& t : model.textures ) {
			auto textureID = graph.textures.size();
			auto keyName = graph.textures.emplace_back((t.name == "" ? graph.images[t.source] : (key + t.name)));
			auto& texture = storage.textures[keyName];
			if ( graph.metadata["debug"]["print"]["textures"].as<bool>() ) {
				UF_MSG_DEBUG("Texture: {}", t.name );
			}

			texture.index = t.source;
			texture.sampler = t.sampler;
		}
	}
	// load materials
	{
		graph.materials.reserve(model.materials.size());
		storage.materials.reserve(model.materials.size());

		for ( auto& m : model.materials ) {
			auto materialID = graph.materials.size();
			auto keyName = graph.materials.emplace_back(key + m.name);
			auto& material = storage.materials[keyName];
			if ( graph.metadata["debug"]["print"]["materials"].as<bool>() ) {
				UF_MSG_DEBUG("Material: {}", m.name );
			}

			material.indexAlbedo = m.pbrMetallicRoughness.baseColorTexture.index;
			material.indexNormal = m.normalTexture.index;
			material.indexEmissive = m.emissiveTexture.index;
			material.indexOcclusion = m.occlusionTexture.index;
			material.indexMetallicRoughness = m.pbrMetallicRoughness.metallicRoughnessTexture.index;
			material.colorBase = {
				m.pbrMetallicRoughness.baseColorFactor[0],
				m.pbrMetallicRoughness.baseColorFactor[1],
				m.pbrMetallicRoughness.baseColorFactor[2],
				m.pbrMetallicRoughness.baseColorFactor[3],
			};
			material.colorEmissive = {
				m.emissiveFactor[0],
				m.emissiveFactor[1],
				m.emissiveFactor[2],
				0
			};

			material.factorMetallic = m.pbrMetallicRoughness.metallicFactor;
			material.factorRoughness = m.pbrMetallicRoughness.roughnessFactor;
			material.factorOcclusion = m.occlusionTexture.strength;
			material.factorAlphaCutoff = m.alphaCutoff;

			const auto mode = uf::string::lowercase( graph.metadata["renderer"]["alpha mode"].as<uf::stl::string>(m.alphaMode) );
			if ( mode == "opaque" ) material.modeAlpha = 0;
			else if ( mode == "blend" ) material.modeAlpha = 1;
			else if ( mode == "mask" ) material.modeAlpha = 2;
			else UF_MSG_WARNING("Invalid AlphaMode enum string specified: {}", mode);

			if ( m.doubleSided && graph.metadata["renderer"]["cull mode"].as<uf::stl::string>() == "auto" ) {
				graph.metadata["renderer"]["cull mode"] = "none";
			}
		}
	}
	// load meshes
	{
		size_t masterInstanceID = 0;

		graph.meshes.reserve(model.meshes.size());
		storage.meshes.reserve(model.meshes.size());

		for ( auto& m : model.meshes ) {
			auto meshID = graph.meshes.size();
			auto keyName = graph.meshes.emplace_back(key + m.name);
			if ( graph.metadata["debug"]["print"]["meshes"].as<bool>() ) {
				UF_MSG_DEBUG("Mesh: {}", m.name );
			}

			graph.primitives.emplace_back(keyName);

			auto& primitives = storage.primitives[keyName];
			auto& mesh = storage.meshes[keyName];
			
			struct {
				uf::meshgrid::Grid grid;
				ext::json::Value metadata;
				float eps = std::numeric_limits<float>::epsilon();
				bool print = false;
				bool clip = true;
				bool cleanup = true;
			} meshgrid;

			struct {
				bool should = false;
				bool print = false;
				bool lods = false;
				size_t level = SIZE_MAX;
				float simplify = 1.0f;
			} meshopt;

			ext::json::forEach( graph.metadata["tags"], [&]( const uf::stl::string& key, ext::json::Value& value ) {
				if ( !ext::json::isObject( value["grid"] ) ) return; // no tag["grid"] defined
				if (  ext::json::isNull( value["grid"]["size"] ) ) return; // no tag["grid"]["size"] defined
				if ( uf::string::isRegex( key ) ) {
					if ( !uf::string::matched( m.name, key ) ) return;
				} else if ( m.name != key ) return;
				meshgrid.metadata = value["grid"];
			});

			if ( ext::json::isObject( meshgrid.metadata ) ) {
				if ( meshgrid.metadata["size"].is<size_t>() ) {
					size_t d = meshgrid.metadata["size"].as<size_t>();
					meshgrid.grid.divisions = {d, d, d};
				} else {
					meshgrid.grid.divisions = uf::vector::decode( meshgrid.metadata["size"], meshgrid.grid.divisions );
				}

				meshgrid.eps = meshgrid.metadata["epsilon"].as(meshgrid.eps);
				meshgrid.print = meshgrid.metadata["print"].as(meshgrid.print);
				meshgrid.clip = meshgrid.metadata["clip"].as(meshgrid.clip);
				meshgrid.cleanup = meshgrid.metadata["cleanup"].as(meshgrid.cleanup);
			}

			{
				#include "processPrimitives.inl"
			}
		}
	}
	// load skins
	{
		graph.skins.reserve( model.skins.size() );
		storage.skins.reserve( model.skins.size() );

		for ( auto& s : model.skins ) {
			auto skinID = graph.skins.size();
			auto keyName = graph.skins.emplace_back(key + s.name);
			auto& skin = storage.skins[keyName];
			if ( graph.metadata["debug"]["print"]["skins"].as<bool>() ) {
				UF_MSG_DEBUG("Skin: {}", s.name );
			}

			skin.name = s.name;			
			if ( s.inverseBindMatrices > -1 ) {
				const tinygltf::Accessor& accessor = model.accessors[s.inverseBindMatrices];
				const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = model.buffers[view.buffer];
				const void* dataPtr = &buffer.data[accessor.byteOffset + view.byteOffset];

				skin.inverseBindMatrices.reserve(accessor.count);
				const pod::Matrix4f* buf = static_cast<const pod::Matrix4f*>(dataPtr);
				for ( size_t i = 0; i < accessor.count; ++i ) skin.inverseBindMatrices.emplace_back( buf[i] );
			}

			skin.joints.reserve( s.joints.size() );
			for ( auto& joint : s.joints ) skin.joints.emplace_back( joint );
		}
	}
	// load animations
	{
		graph.animations.reserve( model.animations.size() );
		storage.animations.reserve( model.animations.size() );

		for ( auto& a : model.animations ) {
			auto animationID = graph.animations.size();
			auto keyName = graph.animations.emplace_back(key + a.name);
			auto& animation = storage.animations[keyName];
			if ( graph.metadata["debug"]["print"]["animations"].as<bool>() ) {
				UF_MSG_DEBUG("Animation: {}", a.name );
			}

			animation.name = a.name;

			// load samplers
			animation.samplers.reserve( a.samplers.size() );
			for ( auto& s : a.samplers ) {
				auto& sampler = animation.samplers.emplace_back();
				sampler.interpolator = s.interpolation;
				{
					const tinygltf::Accessor& accessor = model.accessors[s.input];
					const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = model.buffers[view.buffer];
					const void* dataPtr = &buffer.data[accessor.byteOffset + view.byteOffset];
					const float* buf = static_cast<const float*>(dataPtr);
					
					sampler.inputs.reserve( accessor.count );
					for ( size_t i = 0; i < accessor.count; ++i )
						sampler.inputs.emplace_back( buf[i] );

					for ( auto& input : sampler.inputs ) {
						if ( input < animation.start ) animation.start = input;
						else if ( input > animation.end ) animation.end = input;
					}
				}
				{
					const tinygltf::Accessor& accessor = model.accessors[s.output];
					const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = model.buffers[view.buffer];
					const void* dataPtr = &buffer.data[accessor.byteOffset + view.byteOffset];
					const float* buf = static_cast<const float*>(dataPtr);

					sampler.outputs.reserve( accessor.count );
					size_t components = accessor.ByteStride(view) / sizeof(float);

					for ( size_t i = 0; i < accessor.count; ++i ) {
						auto& output = sampler.outputs.emplace_back(pod::Vector4f{0, 0, 0, 0});
						for ( size_t j = 0; j < components; ++j ) {
							output[j] = buf[i * components + j];
						}
					}
				}
			}

			// load channels
			animation.channels.reserve( a.channels.size() );
			for ( auto& c : a.channels ) {
				auto& channel = animation.channels.emplace_back();
				channel.path = c.target_path;
				channel.sampler = c.sampler;
				channel.node = c.target_node;
			}
		}
	}
		// load lights
	{
		for ( auto& l : model.lights ) {
			auto& light = graph.lights[l.name];
			if ( graph.metadata["debug"]["print"]["lights"].as<bool>() ) {
				UF_MSG_DEBUG("Light: {}", l.name );
			}

			light.color = { l.color[0], l.color[1], l.color[2], };
			light.intensity = l.intensity;
			light.range = l.range;
		}
	}
	// load node information/meshes
	{
		const auto& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];
		graph.nodes.resize( model.nodes.size() );
		graph.root.children.reserve( scene.nodes.size() );
		for ( auto i : scene.nodes ) {
			size_t childIndex = loadNode( model, graph, i, -1 );
			graph.root.children.emplace_back(childIndex);
		}
	}
	// generate atlas
	if ( graph.metadata["renderer"]["atlas"].as<bool>() ) {
		auto atlasName = filename + "/" + "atlas";
		auto& atlas = storage.atlases[atlasName];
		auto atlasImageIndex = graph.images.size();
		auto atlasTextureIndex = graph.textures.size();
	
		for ( auto& keyName : graph.images ) atlas.addImage( storage.images[keyName].data );
		atlas.generate();

		for ( auto& keyName : graph.images ) {
			auto& texture = storage.textures[keyName];
			if ( texture.index < 0 ) continue;
			auto& image = storage.images[keyName].data;

			const auto& hash = image.getHash();
			auto min = atlas.mapUv( {0, 0}, hash );
			auto max = atlas.mapUv( {1, 1}, hash );
			
			texture.lerp = pod::Vector4f{ min.x, min.y, max.x, max.y, };
			texture.index = atlasImageIndex;
		}

		{
			graph.images.emplace_back(atlasName);
			auto& image = storage.images[atlasName].data;
			image = atlas.getAtlas();

			graph.textures.emplace_back(atlasName);
			auto& texture = storage.textures[atlasName];
			texture.index = atlasImageIndex;
		}
	}

	uf::graph::postprocess( graph );
}
#endif