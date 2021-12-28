#include <uf/config.h>

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

#include <gltf/tiny_gltf.h>
#include <uf/ext/gltf/gltf.h>

namespace {
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
		if ( !(graph.metadata["flags"]["INVERT"].as<bool>()) ) {
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
		if ( node.mesh > -1 ) {
			graph.nodes[nodeIndex].mesh = node.mesh;
		}
		return nodeIndex;
	}
}

pod::Graph ext::gltf::load( const uf::stl::string& filename, const uf::Serializer& metadata ) {
	uf::stl::string extension = uf::io::extension( filename );
	if ( extension != "glb" && extension != "gltf" ) {
		return uf::graph::load( filename, metadata );
	}

#if UF_ENV_DREAMCAST
	UF_EXCEPTION("glTF loading is highly discouraged on this platform:" << filename);
	return {};
#endif

	tinygltf::Model model;
	tinygltf::TinyGLTF loader;

	uf::stl::string warn, err;
	bool ret = extension == "glb" ? loader.LoadBinaryFromFile(&model, &err, &warn, filename) : loader.LoadASCIIFromFile(&model, &err, &warn, filename);

	pod::Graph graph;
	graph.name = filename;
	graph.metadata = metadata;

	if ( !warn.empty() ) UF_MSG_WARNING("glTF warning: " << warn);
	if ( !err.empty() ) UF_MSG_ERROR("glTF error: " << err);
	if ( !ret ) { UF_MSG_ERROR("glTF error: failed to parse file: " << filename);
		return graph;
	}

	// load images
	{
		graph.images.reserve(model.images.size());
		/*graph.storage*/uf::graph::storage.images.reserve(model.images.size());

		for ( auto& i : model.images ) {
			auto imageID = graph.images.size();
			auto keyName = graph.images.emplace_back(filename + "/" + i.name);
			auto& image = /*graph.storage*/uf::graph::storage.images[keyName];
			image.loadFromBuffer( &i.image[0], {i.width, i.height}, 8, i.component, graph.metadata["flip textures"].as<bool>(true) );
		}
	}
	// load samplers
	{
		/*graph.storage*/uf::graph::storage.samplers.reserve(model.samplers.size());
		for ( auto& s : model.samplers ) {
			auto samplerID = graph.samplers.size();
			auto keyName = graph.samplers.emplace_back(filename + "/" + s.name);
			auto& sampler = /*graph.storage*/uf::graph::storage.samplers[keyName];

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
		/*graph.storage*/uf::graph::storage.textures.reserve(model.textures.size());

		for ( auto& t : model.textures ) {
			auto textureID = graph.textures.size();
			auto keyName = graph.textures.emplace_back((t.name == "" ? graph.images[t.source] : (filename + "/" + t.name)));
			auto& texture = /*graph.storage*/uf::graph::storage.textures[keyName];

			texture.index = t.source;
			texture.sampler = t.sampler;
		}
	}
	// load materials
	{
		graph.materials.reserve(model.materials.size());
		/*graph.storage*/uf::graph::storage.materials.reserve(model.materials.size());

		for ( auto& m : model.materials ) {
			auto materialID = graph.materials.size();
			auto keyName = graph.materials.emplace_back(filename + "/" + m.name);
			auto& material = /*graph.storage*/uf::graph::storage.materials[keyName];

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

			const uf::stl::string mode = graph.metadata["alpha mode"].as<uf::stl::string>(m.alphaMode);
			if ( mode == "OPAQUE" ) material.modeAlpha = 0;
			else if ( mode == "BLEND" ) material.modeAlpha = 1;
			else if ( mode == "MASK" ) material.modeAlpha = 2;
			else UF_MSG_WARNING("Unhandled alpha mode: " << mode);

			if ( m.doubleSided && graph.metadata["cull mode"].as<uf::stl::string>() == "auto" ) {
				graph.metadata["cull mode"] = "none";
			}
		}
	}
	// load meshes
	{
		graph.meshes.reserve(model.meshes.size());
		/*graph.storage*/uf::graph::storage.meshes.reserve(model.meshes.size());

		for ( auto& m : model.meshes ) {
			auto meshID = graph.meshes.size();
			auto keyName = graph.meshes.emplace_back(filename + "/" + m.name);
			graph.primitives.emplace_back(keyName);
			graph.drawCommands.emplace_back(keyName);

			auto& drawCommands = /*graph.storage*/uf::graph::storage.drawCommands[keyName];
			auto& primitives = /*graph.storage*/uf::graph::storage.primitives[keyName];
			auto& mesh = /*graph.storage*/uf::graph::storage.meshes[keyName];
			mesh.bindIndirect<pod::DrawCommand>();
		#if UF_USE_VULKAN
			bool full = true;
		#else
			bool full = graph.metadata["flags"]["SKINNED"].as<bool>();
		#endif
			if ( full ) {
				mesh.bind<uf::graph::mesh::Skinned, uint32_t>();
				uf::stl::vector<uf::graph::mesh::Skinned> vertices;
				uf::stl::vector<uint32_t> indices;
				for ( auto& p : m.primitives ) {
					vertices.clear();
					indices.clear();
					auto& primitive = primitives.emplace_back();

					struct Attribute {
						uf::stl::string name = "";
						size_t components = 0;
						size_t length = 0;
						size_t stride = 0;
						uint8_t* buffer = NULL;

						uf::stl::vector<float> floats;
						uf::stl::vector<uint8_t> int8s;
						uf::stl::vector<uint16_t> int16s;
						uf::stl::vector<uint32_t> int32s;
					};

					uf::stl::unordered_map<uf::stl::string, Attribute> attributes = {
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
						auto it = p.attributes.find(attribute.name);
						if ( it == p.attributes.end() ) continue;

						auto& accessor = model.accessors[it->second];
						auto& view = model.bufferViews[accessor.bufferView];
						
						if ( attribute.name == "POSITION" ) {
							vertices.resize(accessor.count);
							primitive.instance.bounds.min = pod::Vector3f{ accessor.minValues[0], accessor.minValues[1], accessor.minValues[2] };
							primitive.instance.bounds.max = pod::Vector3f{ accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2] };

							if ( !(graph.metadata["flags"]["INVERT"].as<bool>()) ){
								primitive.instance.bounds.min.x = -primitive.instance.bounds.min.x;
								primitive.instance.bounds.max.x = -primitive.instance.bounds.max.x;
							}
						}

						switch ( accessor.componentType ) {
							case TINYGLTF_COMPONENT_TYPE_BYTE:
							case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
								auto* buffer = reinterpret_cast<const uint8_t*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
								attribute.components = accessor.ByteStride(view) / sizeof(uint8_t);
								attribute.length = accessor.count * attribute.components;
								attribute.int8s.assign( &buffer[0], &buffer[attribute.length] );
							} break;
							case TINYGLTF_COMPONENT_TYPE_SHORT:
							case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
								auto* buffer = reinterpret_cast<const uint16_t*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
								attribute.components = accessor.ByteStride(view) / sizeof(uint16_t);
								attribute.length = accessor.count * attribute.components;
								attribute.int16s.assign( &buffer[0], &buffer[attribute.length] );
							} break;
							case TINYGLTF_COMPONENT_TYPE_INT:
							case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
								auto* buffer = reinterpret_cast<const uint32_t*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
								attribute.components = accessor.ByteStride(view) / sizeof(uint32_t);
								attribute.length = accessor.count * attribute.components;
								attribute.int32s.assign( &buffer[0], &buffer[attribute.length] );
							} break;
							case TINYGLTF_COMPONENT_TYPE_FLOAT: {
								auto* buffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
								attribute.components = accessor.ByteStride(view) / sizeof(float);
								attribute.length = accessor.count * attribute.components;
								attribute.floats.assign( &buffer[0], &buffer[attribute.length] );
							} break;
							default: UF_MSG_ERROR("Unsupported component type");
						}
					}

					for ( size_t i = 0; i < vertices.size(); ++i ) {
					#if 0
						#define ITERATE_ATTRIBUTE( name, member )\
							memcpy( &vertex.member[0], &attributes[name].buffer[i * attributes[name].components], attributes[name].stride );
					#else
						#define ITERATE_ATTRIBUTE( name, member )\
							if ( !attributes[name].int8s.empty() ) { \
								for ( size_t j = 0; j < attributes[name].components; ++j )\
									vertex.member[j] = attributes[name].int8s[i * attributes[name].components + j];\
							} else if ( !attributes[name].int16s.empty() ) { \
								for ( size_t j = 0; j < attributes[name].components; ++j )\
									vertex.member[j] = attributes[name].int16s[i * attributes[name].components + j];\
							} else if ( !attributes[name].int32s.empty() ) { \
								for ( size_t j = 0; j < attributes[name].components; ++j )\
									vertex.member[j] = attributes[name].int32s[i * attributes[name].components + j];\
							} else if ( !attributes[name].floats.empty() ) { \
								for ( size_t j = 0; j < attributes[name].components; ++j )\
									vertex.member[j] = attributes[name].floats[i * attributes[name].components + j];\
							}
					#endif

						auto& vertex = vertices[i];
						ITERATE_ATTRIBUTE("POSITION", position);
						ITERATE_ATTRIBUTE("TEXCOORD_0", uv);
						ITERATE_ATTRIBUTE("NORMAL", normal);
						ITERATE_ATTRIBUTE("TANGENT", tangent);
						ITERATE_ATTRIBUTE("JOINTS_0", joints);
						ITERATE_ATTRIBUTE("WEIGHTS_0", weights);

						#undef ITERATE_ATTRIBUTE

						// required due to reverse-Z projection matrix flipping the X axis as well
						// default is to proceed with this
						if ( !(graph.metadata["flags"]["INVERT"].as<bool>()) ){
							vertex.position.x = -vertex.position.x;
							vertex.normal.x = -vertex.normal.x;
							vertex.tangent.x = -vertex.tangent.x;
						}
					}

					if ( p.indices > -1 ) {
						auto& accessor = model.accessors[p.indices];
						auto& view = model.bufferViews[accessor.bufferView];
						auto& buffer = model.buffers[view.buffer];

						indices.reserve( static_cast<uint32_t>(accessor.count) );

					#define COPY_INDICES() for (size_t index = 0; index < accessor.count; index++) indices.emplace_back(buf[index]);

						const void* pointer = &(buffer.data[accessor.byteOffset + view.byteOffset]);
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

					primitive.instance.materialID = p.material;
					primitive.instance.primitiveID = primitives.size() - 1;
					primitive.instance.meshID = meshID;
					primitive.instance.objectID = 0;

					primitive.drawCommand.indices = indices.size();
					primitive.drawCommand.instances = 1;
					primitive.drawCommand.indexID = 0;
					primitive.drawCommand.vertexID = 0;
					primitive.drawCommand.instanceID = 0;
					primitive.drawCommand.vertices = vertices.size();

					auto& drawCommand = drawCommands.emplace_back(pod::DrawCommand{
						.indices = indices.size(),
						.instances = 1,
						.indexID = mesh.index.count,
						.vertexID = mesh.vertex.count,
						.instanceID = 0,


						.vertices = vertices.size(),
					});
					
					mesh.insertVertices(vertices);
					mesh.insertIndices(indices);
				}
			} else {
				mesh.bind<uf::graph::mesh::Base, uint32_t>();
				uf::stl::vector<uf::graph::mesh::Base> vertices;
				uf::stl::vector<uint32_t> indices;
				for ( auto& p : m.primitives ) {
					vertices.clear();
					indices.clear();
					auto& primitive = primitives.emplace_back();

					struct Attribute {
						uf::stl::string name = "";
						size_t components = 1;
						uf::stl::vector<float> floats;
						uf::stl::vector<uint8_t> int8s;
						uf::stl::vector<uint16_t> int16s;
						uf::stl::vector<uint32_t> int32s;
					};

					uf::stl::unordered_map<uf::stl::string, Attribute> attributes = {
						{"POSITION", {}},
						{"TEXCOORD_0", {}},
						{"NORMAL", {}},
					};

					for ( auto& kv : attributes ) {
						auto& attribute = kv.second;
						attribute.name = kv.first;
						auto it = p.attributes.find(attribute.name);
						if ( it == p.attributes.end() ) continue;

						auto& accessor = model.accessors[it->second];
						auto& view = model.bufferViews[accessor.bufferView];
						
						if ( attribute.name == "POSITION" ) {
							vertices.resize(accessor.count);
							primitive.instance.bounds.min = pod::Vector3f{ accessor.minValues[0], accessor.minValues[1], accessor.minValues[2] };
							primitive.instance.bounds.max = pod::Vector3f{ accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2] };

							if ( !(graph.metadata["flags"]["INVERT"].as<bool>()) ){
								primitive.instance.bounds.min.x = -primitive.instance.bounds.min.x;
								primitive.instance.bounds.max.x = -primitive.instance.bounds.max.x;
							}
						}
						switch ( accessor.componentType ) {
							case TINYGLTF_COMPONENT_TYPE_BYTE:
							case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
								auto* buffer = reinterpret_cast<const uint8_t*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
								attribute.components = accessor.ByteStride(view) / sizeof(uint8_t);
								size_t len = accessor.count * attribute.components;
								attribute.int8s.assign( &buffer[0], &buffer[len] );
							} break;
							case TINYGLTF_COMPONENT_TYPE_SHORT:
							case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
								auto* buffer = reinterpret_cast<const uint16_t*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
								attribute.components = accessor.ByteStride(view) / sizeof(uint16_t);
								size_t len = accessor.count * attribute.components;
								attribute.int16s.assign( &buffer[0], &buffer[len] );
							} break;
							case TINYGLTF_COMPONENT_TYPE_INT:
							case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
								auto* buffer = reinterpret_cast<const uint32_t*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
								attribute.components = accessor.ByteStride(view) / sizeof(uint32_t);
								size_t len = accessor.count * attribute.components;
								attribute.int32s.assign( &buffer[0], &buffer[len] );
							} break;
							case TINYGLTF_COMPONENT_TYPE_FLOAT: {
								auto* buffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
								attribute.components = accessor.ByteStride(view) / sizeof(float);
								size_t len = accessor.count * attribute.components;
								attribute.floats.assign( &buffer[0], &buffer[len] );
							} break;
							default: UF_MSG_ERROR("Unsupported component type");
						}
					}

					for ( size_t i = 0; i < vertices.size(); ++i ) {
						#define ITERATE_ATTRIBUTE( name, member )\
							if ( !attributes[name].int8s.empty() ) { \
								for ( size_t j = 0; j < attributes[name].components; ++j )\
									vertex.member[j] = attributes[name].int8s[i * attributes[name].components + j];\
							} else if ( !attributes[name].int16s.empty() ) { \
								for ( size_t j = 0; j < attributes[name].components; ++j )\
									vertex.member[j] = attributes[name].int16s[i * attributes[name].components + j];\
							} else if ( !attributes[name].int32s.empty() ) { \
								for ( size_t j = 0; j < attributes[name].components; ++j )\
									vertex.member[j] = attributes[name].int32s[i * attributes[name].components + j];\
							} else if ( !attributes[name].floats.empty() ) { \
								for ( size_t j = 0; j < attributes[name].components; ++j )\
									vertex.member[j] = attributes[name].floats[i * attributes[name].components + j];\
							}

						auto& vertex = vertices[i];
						ITERATE_ATTRIBUTE("POSITION", position);
						ITERATE_ATTRIBUTE("TEXCOORD_0", uv);
						ITERATE_ATTRIBUTE("NORMAL", normal);

						#undef ITERATE_ATTRIBUTE

						// required due to reverse-Z projection matrix flipping the X axis as well
						// default is to proceed with this
						if ( !(graph.metadata["flags"]["INVERT"].as<bool>()) ){
							vertex.position.x = -vertex.position.x;
							vertex.normal.x = -vertex.normal.x;
						}
					}

					if ( p.indices > -1 ) {
						auto& accessor = model.accessors[p.indices];
						auto& view = model.bufferViews[accessor.bufferView];
						auto& buffer = model.buffers[view.buffer];

						indices.reserve( static_cast<uint32_t>(accessor.count) );

					#define COPY_INDICES() for (size_t index = 0; index < accessor.count; index++) indices.emplace_back(buf[index]);

						const void* pointer = &(buffer.data[accessor.byteOffset + view.byteOffset]);
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

					primitive.instance.materialID = p.material;
					primitive.instance.primitiveID = primitives.size() - 1;
					primitive.instance.meshID = meshID;
					primitive.instance.objectID = 0;

					primitive.drawCommand.indices = indices.size();
					primitive.drawCommand.instances = 1;
					primitive.drawCommand.indexID = 0;
					primitive.drawCommand.vertexID = 0;
					primitive.drawCommand.instanceID = 0;
					primitive.drawCommand.vertices = vertices.size();

					auto& drawCommand = drawCommands.emplace_back(pod::DrawCommand{
						.indices = indices.size(),
						.instances = 1,
						.indexID = mesh.index.count,
						.vertexID = mesh.vertex.count,
						.instanceID = 0,


						.vertices = vertices.size(),
					});
					
					mesh.insertVertices(vertices);
					mesh.insertIndices(indices);
				}
			}
			mesh.insertIndirects(drawCommands);
			mesh.updateDescriptor();
		}
	}
	// load skins
	{
		graph.skins.reserve( model.skins.size() );
		/*graph.storage*/uf::graph::storage.skins.reserve( model.skins.size() );

		for ( auto& s : model.skins ) {
			auto skinID = graph.skins.size();
			auto keyName = graph.skins.emplace_back(filename + "/" + s.name);
			auto& skin = /*graph.storage*/uf::graph::storage.skins[keyName];

			skin.name = s.name;			
			if ( s.inverseBindMatrices > -1 ) {
				const tinygltf::Accessor& accessor = model.accessors[s.inverseBindMatrices];
				const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = model.buffers[view.buffer];
				const void* dataPtr = &buffer.data[accessor.byteOffset + view.byteOffset];

			#if UF_MATRIX_ALIGNED	
				skin.inverseBindMatrices.resize(accessor.count);
				const float* buf = static_cast<const float*>(dataPtr);
				for ( size_t i = 0; i < accessor.count; ++i ) {
					memcpy( &skin.inverseBindMatrices[i], (const void*) &buf[i*16], sizeof(float) * 16 );
				}
			#else
				skin.inverseBindMatrices.reserve(accessor.count);
				const pod::Matrix4f* buf = static_cast<const pod::Matrix4f*>(dataPtr);
				for ( size_t i = 0; i < accessor.count; ++i ) skin.inverseBindMatrices.emplace_back( buf[i] );
			#endif
			}

			skin.joints.reserve( s.joints.size() );
			for ( auto& joint : s.joints ) skin.joints.emplace_back( joint );
		}
	}
	// load animations
	{
		graph.animations.reserve( model.animations.size() );
		/*graph.storage*/uf::graph::storage.animations.reserve( model.animations.size() );

		for ( auto& a : model.animations ) {
			auto animationID = graph.animations.size();
			auto keyName = graph.animations.emplace_back(filename + "/" + a.name);
			auto& animation = /*graph.storage*/uf::graph::storage.animations[keyName];
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
			light.color = { l.color[0], l.color[1], l.color[2], };
			light.intensity = l.intensity;
			light.range = l.range;
		}
	}
	// load node information/meshes
	{
		const auto& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];
		graph.nodes.resize( model.nodes.size() );

		graph.root.name = "%ROOT%";
		graph.root.index = -1;
		graph.root.children.reserve( scene.nodes.size() );
		for ( auto i : scene.nodes ) {
			size_t childIndex = loadNode( model, graph, i, -1 );
			graph.root.children.emplace_back(childIndex);
		}
	}
	// generate atlas
	if ( graph.metadata["flags"]["ATLAS"].as<bool>() ) {
		auto atlasName = filename + "/" + "atlas";
		auto& atlas = /*graph.storage*/uf::graph::storage.atlases[atlasName];
		auto atlasImageIndex = graph.images.size();
		auto atlasTextureIndex = graph.textures.size();
	
		for ( auto& keyName : graph.images ) atlas.addImage( /*graph.storage*/uf::graph::storage.images[keyName] );
		atlas.generate();

		for ( auto& keyName : graph.images ) {
			auto& texture = /*graph.storage*/uf::graph::storage.textures[keyName];
			if ( texture.index < 0 ) continue;
			auto& image = /*graph.storage*/uf::graph::storage.images[keyName];

			const auto& hash = image.getHash();
			auto min = atlas.mapUv( {0, 0}, hash );
			auto max = atlas.mapUv( {1, 1}, hash );
			
			texture.lerp = pod::Vector4f{ min.x, min.y, max.x, max.y, };
			texture.index = atlasImageIndex;
		}

		{
			graph.images.emplace_back(atlasName);
			auto& image = /*graph.storage*/uf::graph::storage.images[atlasName];
			image = atlas.getAtlas();

			graph.textures.emplace_back(atlasName);
			auto& texture = /*graph.storage*/uf::graph::storage.textures[atlasName];
			texture.index = atlasImageIndex;
		}
	}

	if ( graph.metadata["export"]["should"].as<bool>() ) uf::graph::save( graph, filename );
	return graph;
}