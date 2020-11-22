#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <gltf/tiny_gltf.h>

#include <uf/ext/gltf/gltf.h>

#include <uf/utils/string/ext.h>

#include <uf/utils/thread/thread.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/image/atlas.h>
#include <uf/utils/string/hash.h>

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

		auto& transform = newNode.transform;
		if ( node.translation.size() == 3 ) {
			transform.position.x = node.translation[0];
			transform.position.y = node.translation[1];
			transform.position.z = node.translation[2];
		} else {
			transform.position = { 0, 0, 0 };
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

		if ( node.children.size() > 0 ) {
			loadNodes( model, graph, node.children, newNode );
		}

		auto& mesh = newNode.mesh;
		auto& collider = newNode.collider;
		if ( node.mesh > -1 ) {
			size_t id = 0;
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

						if ( (graph.mode & ext::gltf::LoadMode::COLLISION) && (graph.mode & ext::gltf::LoadMode::AABB) ) {
							auto* box = new uf::BoundingBox( origin, size );
							collider.add(box);
						}
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
					ITERATE_ATTRIBUTE("JOINTS_0", joints);
					ITERATE_ATTRIBUTE("WEIGHTS_0", weights);

					#undef ITERATE_ATTRIBUTE
					if ( !(graph.mode & ext::gltf::LoadMode::SEPARATE_MESHES) && (graph.mode & ext::gltf::LoadMode::USE_ATLAS) ) {
						vertex.uv = graph.atlas->mapUv( vertex.uv, id );
					}
					if ( graph.mode & ext::gltf::LoadMode::FLIP_XY ) {
						std::swap( vertex.position.x, vertex.position.y );
					}
					vertex.id = id;
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
				// recalc normals
				if ( graph.mode & ext::gltf::LoadMode::GENERATE_NORMALS ) {
					// bool invert = false;
					if ( !mesh.indices.empty() ) {
						for ( size_t i = 0; i < mesh.vertices.size(); i+=3 ) {
							auto& a = mesh.vertices[i+0].position;
							auto& b = mesh.vertices[i+1].position;
							auto& c = mesh.vertices[i+2].position;

							pod::Vector3f normal = uf::vector::normalize( uf::vector::cross( b - a, c - a ) );
							mesh.vertices[i+0].normal = normal;
							mesh.vertices[i+1].normal = normal;
							mesh.vertices[i+2].normal = normal;
						}
					} else {
						for ( size_t i = 0; i < mesh.indices.size(); i+=3 ) {
							auto& A = mesh.vertices[mesh.indices[i+0]];
							auto& B = mesh.vertices[mesh.indices[i+1]];
							auto& C = mesh.vertices[mesh.indices[i+2]];

							auto& a = A.position;
							auto& b = B.position;
							auto& c = C.position;

							pod::Vector3f normal = uf::vector::normalize( uf::vector::cross( b - a, c - a ) );
							
							A.normal = normal;
							B.normal = normal;
							C.normal = normal;
						}
					}
				}
				if ( graph.mode & ext::gltf::LoadMode::APPLY_TRANSFORMS ) {
					pod::Matrix4f model = uf::transform::model( transform );
					for ( auto& vertex : mesh.vertices ) {
						vertex.position = uf::matrix::multiply<float>( model, vertex.position );
					}
				}
				++id;
			}
			// setup collision
			if ( (graph.mode & ext::gltf::LoadMode::COLLISION) && !(graph.mode & ext::gltf::LoadMode::AABB) ) {
				auto* c = new uf::MeshCollider( transform );
				c->setPositions( mesh );
				collider.add(c);
			}
		}
		transform = uf::transform::initialize( transform );
		return newNode;
	}
}

pod::Graph ext::gltf::load( const std::string& filename, ext::gltf::load_mode_t mode ) {
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;

	std::string warn, err;
	std::string extension = uf::io::extension( filename );
	bool ret = extension == "glb" ? loader.LoadBinaryFromFile(&model, &err, &warn, filename) : loader.LoadASCIIFromFile(&model, &err, &warn, filename);

	pod::Graph graph;
	graph.mode = mode;

	if ( !warn.empty() ) std::cout << "glTF warning: " << warn << std::endl;
	if ( !err.empty() ) std::cout << "glTF error: " << err << std::endl;
	if ( !ret ) { std::cout << "glTF error: failed to parse file: " << filename << std::endl;
		return graph;
	}

	// load images
	{
		for ( auto& s : model.samplers ) {
			auto& sampler = graph.samplers.emplace_back();
			sampler.descriptor.filter.min = getVkFilterMode( s.minFilter );
			sampler.descriptor.filter.mag = getVkFilterMode( s.magFilter );
			sampler.descriptor.addressMode.u = getVkWrapMode( s.wrapS );
			sampler.descriptor.addressMode.v = getVkWrapMode( s.wrapT );
			sampler.descriptor.addressMode.w = sampler.descriptor.addressMode.v;
		}
		if ( !graph.atlas ) graph.atlas = new uf::Atlas;
		auto& images = graph.atlas->getImages();
		for ( auto& t : model.textures ) {
			auto& im = model.images[t.source];
			auto& image = images.emplace_back();
			image.loadFromBuffer( &im.image[0], {im.width, im.height}, 8, im.component, true );
		}
		if ( graph.mode & ext::gltf::LoadMode::RENDER && images.empty() ) {
			std::vector<uint8_t> pixels = { 
				255,   0, 255, 255,      0,   0,   0, 255,
				  0,   0,   0, 255,    255,   0, 255, 255,
			};
			auto& image = images.emplace_back();
			image.loadFromBuffer( &pixels[0], {2, 2}, 8, 4, true );
		}
		if ( !images.empty() ) graph.atlas->generate();
	}
	// load node information/meshes
	{
		const auto& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];
		auto& node = uf::graph::node();
		graph.node = &loadNodes( model, graph, scene.nodes, node );
	}
	// load skins
	{
		graph.skins.reserve( model.skins.size() );
		for ( auto& s : model.skins ) {
			auto& skin = graph.skins.emplace_back();
			skin.name = s.name;
		//	skin.root = uf::graph::find( graph, s.skeleton );
			
			if ( s.inverseBindMatrices > -1 ) {
				const tinygltf::Accessor& accessor = model.accessors[s.inverseBindMatrices];
				const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = model.buffers[view.buffer];
				const void* dataPtr = &buffer.data[accessor.byteOffset + view.byteOffset];
				const pod::Matrix4f* buf = static_cast<const pod::Matrix4f*>(dataPtr);
					
				skin.inverseBindMatrices.reserve(accessor.count);
				for ( size_t i = 0; i < accessor.count; ++i )
					skin.inverseBindMatrices.emplace_back( buf[i] );

			//	skin.inverseBindMatrices.resize(accessor.count);
			//	memcpy(skin.inverseBindMatrices.data(), &buffer.data[accessor.byteOffset + view.byteOffset], accessor.count * sizeof(pod::Matrix4f));
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
		//	auto& animation = graph.animations.emplace_back();
			auto& animation = graph.animations[a.name];
			animation.name = a.name;
		//	if ( graph.animation == "" ) graph.animation = a.name;

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