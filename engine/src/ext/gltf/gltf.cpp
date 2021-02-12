#include <uf/config.h>

#define TINYGLTF_IMPLEMENTATION

#if defined(UF_JSON_USE_NLOHMANN) && UF_JSON_USE_NLOHMANN
	#define TINYGLTF_NO_INCLUDE_JSON
#endif

#include <uf/utils/string/ext.h>

#include <uf/utils/thread/thread.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/image/atlas.h>
#include <uf/utils/string/hash.h>

#include <gltf/tiny_gltf.h>
#include <uf/ext/gltf/gltf.h>

namespace {
	VkSamplerAddressMode getVkWrapMode(int32_t wrapMode) {
		switch (wrapMode) {
			case 10497: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
			case 33071: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			case 33648: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			default: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		}
	}

	VkFilter getVkFilterMode(int32_t filterMode) {
		switch (filterMode) {
			case 9728: return VK_FILTER_NEAREST;
			case 9729: return VK_FILTER_LINEAR;
			case 9984: return VK_FILTER_NEAREST;
			case 9985: return VK_FILTER_NEAREST;
			case 9986: return VK_FILTER_LINEAR;
			case 9987: return VK_FILTER_LINEAR;
			default: return VK_FILTER_LINEAR;
		}
	}
}

namespace {
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
		if ( !(graph.mode & ext::gltf::LoadMode::INVERT) ) {
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
		if ( node.matrix.size() == 16 ) {
			for ( size_t i = 0; i < node.matrix.size(); ++i ) transform.model[i] = node.matrix[i];
		} else {
			transform.model = uf::matrix::identity();
		}
		if ( 0 <= parentIndex && parentIndex < graph.nodes.size() && nodeIndex != parentIndex ) {
			transform.reference = &graph.nodes[parentIndex].transform;
		}
		if ( node.children.size() > 0 ) {
			loadNodes( model, graph, node.children, nodeIndex );
		}

		auto& mesh = graph.nodes[nodeIndex].mesh;
		auto& collider = graph.nodes[nodeIndex].collider;
		if ( node.mesh > -1 ) {
			auto& drawCall = graph.nodes[nodeIndex].drawCall;
			drawCall.name = node.name;
			drawCall.verticesIndex = mesh.vertices.size();
			drawCall.indicesIndex = mesh.indices.size();
			drawCall.vertices = 0;
			drawCall.indices = 0;
			for ( auto& primitive : model.meshes[node.mesh].primitives ) {
				auto& dc = graph.nodes[nodeIndex].primitives.emplace_back();
				dc.verticesIndex = mesh.vertices.size();
				dc.indicesIndex = mesh.indices.size();

				struct Attribute {
					std::string name = "";
					size_t components = 1;
					std::vector<float> buffer;
					std::vector<uint16_t> indices;
				};
				std::unordered_map<std::string, Attribute> attributes = {
					{"POSITION", {}},
					{"TEXCOORD_0", {}},
					{"NORMAL", {}},
					{"TANGENT", {}},
					{"JOINTS_0", {}},
					{"WEIGHTS_0", {}},
				};

				for ( auto& kv : attributes ) {
					auto& attribute = kv.second;
					attribute.name = kv.first;
					auto it = primitive.attributes.find(attribute.name);
					if ( it == primitive.attributes.end() ) continue;

					auto& accessor = model.accessors[it->second];
					auto& view = model.bufferViews[accessor.bufferView];
					
					if ( attribute.name == "POSITION" ) {
						dc.vertices = accessor.count;
						pod::Vector3f minCorner = { accessor.minValues[0], accessor.minValues[1], accessor.minValues[2] };
						pod::Vector3f maxCorner = { accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2] };

						pod::Vector3f origin = (maxCorner + minCorner) * 0.5f;
						pod::Vector3f size = (maxCorner - minCorner) * 0.5f;
					}
					if ( attribute.name == "JOINTS_0" ) {
						auto* buffer = reinterpret_cast<const uint16_t*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
						attribute.components = accessor.ByteStride(view) / sizeof(uint16_t);
						size_t len = accessor.count * attribute.components;
						attribute.indices.reserve( len );
						attribute.indices.insert( attribute.indices.end(), &buffer[0], &buffer[len] );
					} else {
						auto* buffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
						attribute.components = accessor.ByteStride(view) / sizeof(float);
						size_t len = accessor.count * attribute.components;
						attribute.buffer.reserve( len );
						attribute.buffer.insert( attribute.buffer.end(), &buffer[0], &buffer[len] );
					}
				}

				auto modelMatrix = uf::transform::model( transform );
				mesh.vertices.reserve( dc.vertices + dc.verticesIndex );
				for ( size_t i = 0; i < dc.vertices; ++i ) {
					#define ITERATE_ATTRIBUTE( name, member )\
						if ( !attributes[name].indices.empty() ) { \
							for ( size_t j = 0; j < attributes[name].components; ++j )\
								vertex.member[j] = attributes[name].indices[i * attributes[name].components + j];\
						} else if ( !attributes[name].buffer.empty() ) { \
							for ( size_t j = 0; j < attributes[name].components; ++j )\
								vertex.member[j] = attributes[name].buffer[i * attributes[name].components + j];\
						}

					auto& vertex = mesh.vertices.emplace_back();
					ITERATE_ATTRIBUTE("POSITION", position);
					ITERATE_ATTRIBUTE("TEXCOORD_0", uv);
					ITERATE_ATTRIBUTE("NORMAL", normal);
					ITERATE_ATTRIBUTE("TANGENT", tangent);
					ITERATE_ATTRIBUTE("JOINTS_0", joints);
					ITERATE_ATTRIBUTE("WEIGHTS_0", weights);

					#undef ITERATE_ATTRIBUTE
					vertex.id.x = nodeIndex;
					vertex.id.y = primitive.material;

				/*
					if ( !(graph.mode & ext::gltf::LoadMode::SEPARATE) && (graph.mode & ext::gltf::LoadMode::ATLAS) ) {
						auto& material = graph.materials[primitive.material];
						auto& texture = graph.textures[material.storage.indexAlbedo];
						vertex.uv = graph.atlas->mapUv( vertex.uv, texture.index );
					}
				*/
					// required due to reverse-Z projection matrix flipping the X axis as well
					// default is to proceed with this
					if ( !(graph.mode & ext::gltf::LoadMode::INVERT) ){
						vertex.position.x *= -1;
						vertex.normal.x *= -1;
						vertex.tangent.x *= -1;
					}
					if ( graph.mode & ext::gltf::LoadMode::TRANSFORM ) {
						vertex.position = uf::matrix::multiply<float>( modelMatrix, vertex.position );
					}
				}

				if ( primitive.indices > -1 ) {
					auto& accessor = model.accessors[primitive.indices];
					auto& view = model.bufferViews[accessor.bufferView];
					auto& buffer = model.buffers[view.buffer];

					dc.indices = static_cast<uint32_t>(accessor.count);
					mesh.indices.reserve( dc.indices + dc.indicesIndex );
					const void* pointer = &(buffer.data[accessor.byteOffset + view.byteOffset]);

					#define COPY_INDICES()\
						for (size_t index = 0; index < dc.indices; index++)\
								mesh.indices.emplace_back(buf[index] + dc.verticesIndex);

					switch (accessor.componentType) {
						case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
							auto* buf = static_cast<const uint32_t*>( pointer );
							COPY_INDICES()
							break;
						}
						case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
							auto* buf = static_cast<const uint16_t*>( pointer );
							COPY_INDICES()
							break;
						}
						case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
							auto* buf = static_cast<const uint8_t*>( pointer );
							COPY_INDICES()
							break;
						}
					}
					#undef COPY_INDICES
				}

				drawCall.vertices += dc.vertices;
				drawCall.indices += dc.indices;

				graph.drawCall.vertices += dc.vertices;
				graph.drawCall.indices += dc.indices;
			/*
				if ( graph.mode & ext::gltf::LoadMode::TRANSFORM ) {
					auto model = uf::transform::model( transform );
					for ( auto& vertex : mesh.vertices ) {
						vertex.position = uf::matrix::multiply<float>( model, vertex.position );
					}
				}
			*/
			}
		}
		return nodeIndex;
	}
}

pod::Graph ext::gltf::load( const std::string& filename, ext::gltf::load_mode_t mode, const uf::Serializer& metadata ) {
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;

	std::string warn, err;
	std::string extension = uf::io::extension( filename );
	bool ret = extension == "glb" ? loader.LoadBinaryFromFile(&model, &err, &warn, filename) : loader.LoadASCIIFromFile(&model, &err, &warn, filename);

	pod::Graph graph;
	graph.mode = mode;
	graph.metadata = metadata;


	if ( !warn.empty() ) uf::iostream << "glTF warning: " << warn << "\n";
	if ( !err.empty() ) uf::iostream << "glTF error: " << err << "\n";
	if ( !ret ) { uf::iostream << "glTF error: failed to parse file: " << filename << "\n";
		return graph;
	}

	// load samplers
	for ( auto& s : model.samplers ) {
		auto& sampler = graph.samplers.emplace_back();
		sampler.descriptor.filter.min = getVkFilterMode( s.minFilter );
		sampler.descriptor.filter.mag = getVkFilterMode( s.magFilter );
		sampler.descriptor.addressMode.u = getVkWrapMode( s.wrapS );
		sampler.descriptor.addressMode.v = getVkWrapMode( s.wrapT );
		sampler.descriptor.addressMode.w = sampler.descriptor.addressMode.v;
	}

	// load images
	{
		size_t _i = 0;
		for ( auto& i : model.images ) {
			auto& image = graph.images.emplace_back();
			image.loadFromBuffer( &i.image[0], {i.width, i.height}, 8, i.component, true );

			if ( graph.metadata["dump images"].as<bool>() ) {
				std::string target = "./data/dump/" + uf::string::sha256(filename) + "." + std::to_string(_i++) + ".png";
				image.save( target );
			}
		}
	}
	// generate atlas
	if ( mode & ext::gltf::LoadMode::ATLAS ) { if ( graph.atlas ) delete graph.atlas; graph.atlas = new uf::Atlas;
		auto& atlas = *graph.atlas;
		atlas.generate( graph.images );
	}
	// load textures
	{
		for ( auto& t : model.textures ) {
			auto& texture = graph.textures.emplace_back();
			texture.name = t.name;

			texture.storage.index = t.source;
			texture.storage.sampler = t.sampler;
			if ( graph.atlas ) {
				auto& atlas = *graph.atlas;
				auto atlasMin = atlas.mapUv( {0, 0}, t.source );
				auto atlasMax = atlas.mapUv( {1, 1}, t.source );

				texture.storage.lerp = {
					atlasMin.x,
					atlasMin.y,

					atlasMax.x,
					atlasMax.y,
				};
			}
		}
	}
	// load materials
	{
		// null material
		// graph.materials.emplace_back();
		for ( auto& m : model.materials ) {
			auto& material = graph.materials.emplace_back();
			material.name = m.name;
			material.storage.indexAlbedo = m.pbrMetallicRoughness.baseColorTexture.index;
			material.storage.indexNormal = m.normalTexture.index;
			material.storage.indexEmissive = m.emissiveTexture.index;
			material.storage.indexOcclusion = m.occlusionTexture.index;
			material.storage.indexMetallicRoughness = m.pbrMetallicRoughness.metallicRoughnessTexture.index;
			material.storage.colorBase = {
				m.pbrMetallicRoughness.baseColorFactor[0],
				m.pbrMetallicRoughness.baseColorFactor[1],
				m.pbrMetallicRoughness.baseColorFactor[2],
				m.pbrMetallicRoughness.baseColorFactor[3],
			};
			material.storage.colorEmissive = {
				m.emissiveFactor[0],
				m.emissiveFactor[1],
				m.emissiveFactor[2],
				0
			};

			material.storage.factorMetallic = m.pbrMetallicRoughness.metallicFactor;
			material.storage.factorRoughness = m.pbrMetallicRoughness.roughnessFactor;
			material.storage.factorOcclusion = m.occlusionTexture.strength;
			material.storage.factorAlphaCutoff = m.alphaCutoff;

			material.alphaMode = m.alphaMode;
			if ( material.alphaMode == "OPAQUE" ) {
				material.storage.modeAlpha = 0;
			} else if ( material.alphaMode == "BLEND" ) {
				material.storage.modeAlpha = 1;
			} else if ( material.alphaMode == "MASK" ) {
				material.storage.modeAlpha = 2;
			} else {
				std::cout << "Unhandled alpha mode: " << material.alphaMode << std::endl;
			}
			if ( m.doubleSided && graph.metadata["cull mode"].as<std::string>() == "auto" ) {
				graph.metadata["cull mode"] = "none";
			}
		}
	}
	// load node information/meshes
	{
		size_t rootIndex = model.defaultScene > -1 ? model.defaultScene : 0;
		const auto& scene = model.scenes[rootIndex];
		graph.nodes.resize( model.nodes.size() );
		graph.root.index = -1;
		graph.root.children.reserve( scene.nodes.size() );
		for ( auto i : scene.nodes ) {
			size_t childIndex = loadNode( model, graph, i, -1 );
			graph.root.children.emplace_back(childIndex);
		}
	}
	// load lights
	{
		for ( auto& l : model.lights ) {
			auto& light = graph.lights.emplace_back();
			light.name = l.name;
			light.color = {
				l.color[0],
				l.color[1],
				l.color[2],
			};
			light.intensity = l.intensity;
			light.range = l.range;
		}
	}
	// load skins
	{
		graph.skins.reserve( model.skins.size() );
		for ( auto& s : model.skins ) {
			auto& skin = graph.skins.emplace_back();
			skin.name = s.name;			
			if ( s.inverseBindMatrices > -1 ) {
				const tinygltf::Accessor& accessor = model.accessors[s.inverseBindMatrices];
				const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = model.buffers[view.buffer];
				const void* dataPtr = &buffer.data[accessor.byteOffset + view.byteOffset];
				const pod::Matrix4f* buf = static_cast<const pod::Matrix4f*>(dataPtr);
					
				skin.inverseBindMatrices.reserve(accessor.count);
				for ( size_t i = 0; i < accessor.count; ++i )
					skin.inverseBindMatrices.emplace_back( buf[i] );
			}

			for ( auto& joint : s.joints ) {
				skin.joints.emplace_back( joint );
			}
		}
	}
	// load animations
	{
		graph.animations.reserve( model.animations.size() );
		for ( auto& a : model.animations ) {
			auto& animation = graph.animations[a.name];
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
						if ( !(graph.mode & ext::gltf::LoadMode::INVERT) ){
						//	output.x *= -1;
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


	uf::graph::process( graph );
	// print node
/*
	std::cout << "Tree for " << filename << " (Mode: " << std::bitset<16>( graph.mode ) << "): " << std::endl;
	std::function<void(pod::Node&, size_t)> print = [&]( pod::Node& node, size_t indent ) {
		for ( size_t i = 0; i < indent; ++i ) std::cout << "\t";
		std::cout << "Node " << node.name << " (" << node.index << ": " << &node << "):" << std::endl;
		for ( auto index : node.children ) {
			print( graph.nodes[index], indent + 1 );
		}
	};
	print( graph.root, 1 );
	std::cout << std::endl;
*/

	return graph;
}