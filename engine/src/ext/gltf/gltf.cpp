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
	pod::Node& loadNode( const tinygltf::Model& model, pod::Graph& graph, int32_t nodeIndex, pod::Node& parentNode );

	pod::Node& loadNodes( const tinygltf::Model& model, pod::Graph& graph, const std::vector<int>& nodes, pod::Node& node ) {
		for ( auto i : nodes ) {
			node.children.emplace_back(&loadNode( model, graph, i, node ));
		}
		return node;
	}
	pod::Node& loadNode( const tinygltf::Model& model, pod::Graph& graph, int32_t nodeIndex, pod::Node& parentNode ) {
		auto& newNode = uf::graph::node();
		if ( nodeIndex < 0 ) return newNode;
		auto& node = model.nodes[nodeIndex];
		newNode.parent = &parentNode;
		newNode.index = nodeIndex;
		newNode.skin = node.skin;

		newNode.name = node.name;

		auto& transform = newNode.transform;
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
		if ( newNode.parent != &newNode ) {
			transform.reference = &newNode.parent->transform;
		}

		if ( node.children.size() > 0 ) {
			loadNodes( model, graph, node.children, newNode );
		}

		auto& mesh = newNode.mesh;
		auto& collider = newNode.collider;
		if ( node.mesh > -1 ) {
		//	size_t id = 0;
			for ( auto& primitive : model.meshes[node.mesh].primitives ) {
				size_t verticesStart = mesh.vertices.size();
				size_t indicesStart = mesh.indices.size();

				size_t vertices = 0;
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
						vertices = accessor.count;
						pod::Vector3f minCorner = { accessor.minValues[0], accessor.minValues[1], accessor.minValues[2] };
						pod::Vector3f maxCorner = { accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2] };

						pod::Vector3f origin = (maxCorner + minCorner) * 0.5f;
						pod::Vector3f size = (maxCorner - minCorner) * 0.5f;
					/*
						if ( (graph.mode & ext::gltf::LoadMode::COLLISION) && (graph.mode & ext::gltf::LoadMode::AABB) ) {
							auto* box = new uf::BoundingBox( origin, size );
							collider.add(box);
						}
					*/
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
				mesh.vertices.reserve( vertices + verticesStart );
				for ( size_t i = 0; i < vertices; ++i ) {
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
					if ( !(graph.mode & ext::gltf::LoadMode::SEPARATE) && (graph.mode & ext::gltf::LoadMode::ATLAS) ) {
						auto& material = graph.materials[primitive.material];
						auto& texture = graph.textures[material.storage.indexAlbedo];
						vertex.uv = graph.atlas->mapUv( vertex.uv, texture.index );
					}
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
					vertex.id = primitive.material;
				}

				if ( primitive.indices > -1 ) {
					auto& accessor = model.accessors[primitive.indices];
					auto& view = model.bufferViews[accessor.bufferView];
					auto& buffer = model.buffers[view.buffer];

					auto indices = static_cast<uint32_t>(accessor.count);
					mesh.indices.reserve( indices + indicesStart );
					const void* pointer = &(buffer.data[accessor.byteOffset + view.byteOffset]);

					#define COPY_INDICES()\
						for (size_t index = 0; index < indices; index++)\
								mesh.indices.emplace_back(buf[index] + verticesStart);

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
			/*
				if ( graph.mode & ext::gltf::LoadMode::TRANSFORM ) {
					auto model = uf::transform::model( transform );
					for ( auto& vertex : mesh.vertices ) {
						vertex.position = uf::matrix::multiply<float>( model, vertex.position );
					}
				}
			*/
			//	++id;
			}
			// setup collision
		/*
			if ( (graph.mode & ext::gltf::LoadMode::COLLISION) && !(graph.mode & ext::gltf::LoadMode::AABB) ) {
			//	auto* c = new uf::MeshCollider( transform );
				auto* c = new uf::MeshCollider();
				c->setPositions( mesh );
				collider.add(c);
			}
		*/
		}
		return newNode;
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
		for ( auto& i : model.images ) {
			auto& image = graph.images.emplace_back();
			image.loadFromBuffer( &i.image[0], {i.width, i.height}, 8, i.component, true );
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
			texture.index = t.source;
			texture.sampler = t.sampler;
		}
	}
	// load materials
	{
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

			if ( 0 <= material.storage.indexAlbedo && material.storage.indexAlbedo < graph.textures.size() ) {
				material.storage.indexAlbedo = graph.textures[material.storage.indexAlbedo].index;
			}
			if ( 0 <= material.storage.indexNormal && material.storage.indexNormal < graph.textures.size() ) {
				material.storage.indexNormal = graph.textures[material.storage.indexNormal].index;
			}
			if ( 0 <= material.storage.indexEmissive && material.storage.indexEmissive < graph.textures.size() ) {
				material.storage.indexEmissive = graph.textures[material.storage.indexEmissive].index;
			}
			if ( 0 <= material.storage.indexOcclusion && material.storage.indexOcclusion < graph.textures.size() ) {
				material.storage.indexOcclusion = graph.textures[material.storage.indexOcclusion].index;
			}
			if ( 0 <= material.storage.indexMetallicRoughness && material.storage.indexMetallicRoughness < graph.textures.size() ) {
				material.storage.indexMetallicRoughness = graph.textures[material.storage.indexMetallicRoughness].index;
			}
		}
	}
	// load node information/meshes
	{
		const auto& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];
		auto& node = uf::graph::node();
		graph.node = &loadNodes( model, graph, scene.nodes, node );
	}
	// load lights
	{
		for ( auto& l : model.lights ) {
			auto* node = uf::graph::find( graph, l.name );
			if ( !node ) continue;
			auto& light = graph.lights.emplace_back();
			light.name = l.name;
			light.transform = node->transform;
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
				auto* node = uf::graph::find( graph, joint );
				if ( node ) skin.joints.emplace_back( node );
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
				channel.node = uf::graph::find(graph, c.target_node);
			}
		}
	}


	uf::graph::process( graph );
	// print node
/*
	std::cout << "Tree for " << filename << " (Mode: " << std::bitset<16>( graph.mode ) << "): " << std::endl;
	std::function<void(pod::Node&,size_t)> print = [&]( pod::Node& node, size_t indent ) {
		for ( size_t i = 0; i < indent; ++i ) std::cout << "\t";
		std::cout << "Node " << &node << " (" << node.index << "):" << std::endl;
		for ( auto* child : node.children ) print( *child, indent + 1 );
	};
	print( *graph.node, 1 );
	std::cout << std::endl;
*/
	return graph;

}