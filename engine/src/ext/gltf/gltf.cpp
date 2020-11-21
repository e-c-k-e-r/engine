#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <gltf/tiny_gltf.h>

#include <uf/ext/gltf/gltf.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/graphic/mesh.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/math/collision.h>
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

namespace legacy {
	void loadNode( uf::Object& entity, const tinygltf::Model& model, const tinygltf::Node& node, ext::gltf::load_mode_t mode ) {
		auto& transform = entity.getComponent<pod::Transform<>>();
		auto& atlas = entity.getComponent<uf::Atlas>();

		if ( node.translation.size() == 3 ) {
			transform.position.x = node.translation[0];
			transform.position.y = node.translation[1];
			transform.position.z = node.translation[2];
		}
		if ( node.rotation.size() == 4 ) {
			transform.orientation.x = node.rotation[0];
			transform.orientation.y = node.rotation[1];
			transform.orientation.z = node.rotation[2];
			transform.orientation.w = node.rotation[3];
		}
		if ( node.scale.size() == 3 ) {
			transform.scale.x = node.scale[0];
			transform.scale.y = node.scale[1];
			transform.scale.z = node.scale[2];
		}
	// 	if ( node.matrix.size() == 16 ) {}
		if ( node.mesh > -1 ) {
			auto& m = model.meshes[node.mesh];
			std::vector<ext::vulkan::Sampler> samplers;
			std::vector<uf::Image> _images;
			bool useAtlas = (mode & ext::gltf::LoadMode::USE_ATLAS) && (mode & ext::gltf::LoadMode::SEPARATE_MESHES);
			auto& images = useAtlas ? atlas.getImages() : _images;

			for ( auto& s : model.samplers ) {
				auto& sampler = samplers.emplace_back();
				sampler.descriptor.filter.min = getVkFilterMode( s.minFilter );
				sampler.descriptor.filter.mag = getVkFilterMode( s.magFilter );
				sampler.descriptor.addressMode.u = getVkWrapMode( s.wrapS );
				sampler.descriptor.addressMode.v = getVkWrapMode( s.wrapT );
				sampler.descriptor.addressMode.w = sampler.descriptor.addressMode.v;
			}
			for ( auto& t : model.textures ) {
				auto& im = model.images[t.source];
				uf::Image& image = images.emplace_back();
				image.loadFromBuffer( &im.image[0], {im.width, im.height}, 8, im.component, true );
			}
			auto fillMesh = [&]( ext::gltf::mesh_t& mesh, const tinygltf::Primitive& primitive, uf::Collider& collider, size_t id ) {
				size_t verticesStart = mesh.vertices.size();
				size_t indicesStart = mesh.indices.size();

				size_t vertices = 0;
				struct Attribute {
					std::string name = "";
					size_t components = 1;
					std::vector<float> buffer;
				};
				std::unordered_map<std::string, Attribute> attributes = {
					{"POSITION", {}},
					{"TEXCOORD_0", {}},
					{"NORMAL", {}},
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

						if ( (mode & ext::gltf::LoadMode::COLLISION) && (mode & ext::gltf::LoadMode::AABB) ) {
							auto* box = new uf::BoundingBox( origin, size );
							collider.add(box);
						}
					}
					{
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
						if ( !attributes[name].buffer.empty() ) { \
							for ( size_t j = 0; j < attributes[name].components; ++j )\
								vertex.member[j] = attributes[name].buffer[i * attributes[name].components + j];\
						}

					auto& vertex = mesh.vertices.emplace_back();
					ITERATE_ATTRIBUTE("POSITION", position);
					ITERATE_ATTRIBUTE("TEXCOORD_0", uv);
					ITERATE_ATTRIBUTE("NORMAL", normal);

					#undef ITERATE_ATTRIBUTE
					if ( !(mode & ext::gltf::LoadMode::SEPARATE_MESHES) && (mode & ext::gltf::LoadMode::USE_ATLAS) ) {
						vertex.uv = atlas.mapUv( vertex.uv, id );
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
				if ( mode & ext::gltf::LoadMode::GENERATE_NORMALS ) {
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
				if ( mode & ext::gltf::LoadMode::APPLY_TRANSFORMS ) {
					pod::Matrix4f model = uf::transform::model( transform );
					for ( auto& vertex : mesh.vertices ) {
						vertex.position = uf::matrix::multiply<float>( model, vertex.position );
					}
				}
			};
			if ( mode & ext::gltf::LoadMode::SEPARATE_MESHES ) {
				auto sampler = samplers.begin();
				auto image = images.begin();
				size_t id = 0;
				for ( auto& primitive : m.primitives ) {
					uf::Object* child = new uf::Object;
					
					//ext::gltf::mesh_t mesh;
					auto& mesh = child->getComponent<ext::gltf::mesh_t>();
					auto& cTransform = child->getComponent<pod::Transform<>>();
					cTransform = transform;

					auto& collider = child->getComponent<uf::Collider>();
					fillMesh( mesh, primitive, collider, id++ );

					if ( (mode & ext::gltf::LoadMode::COLLISION) && !(mode & ext::gltf::LoadMode::AABB) ) {
						auto* box = new uf::MeshCollider( cTransform );
						box->setPositions( mesh );
						collider.add(box);
					}
					if ( mode & ext::gltf::LoadMode::RENDER ) {
						auto& graphic = child->getComponent<uf::Graphic>();
						if ( mode & ext::gltf::LoadMode::DEFER_INIT ) {
							graphic.process = false;
						} else {
							graphic.initialize();
							graphic.initializeGeometry( mesh );

							graphic.material.attachShader("./data/shaders/gltf.stereo.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
							graphic.material.attachShader("./data/shaders/gltf.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

							auto& shader = graphic.material.shaders.back();
							struct SpecializationConstant {
								uint32_t textures = 1;
							};
							auto* specializationConstants = (SpecializationConstant*) &shader.specializationConstants[0];
							specializationConstants->textures = graphic.material.textures.size();
							for ( auto& binding : shader.descriptorSetLayoutBindings ) {
								if ( binding.descriptorCount > 1 )
									binding.descriptorCount = specializationConstants->textures;
							}
						}
						
						if ( image != images.end() ) graphic.material.textures.emplace_back().loadFromImage( *(image++) );
						if ( sampler != samplers.end() ) graphic.material.samplers.push_back( *(sampler++) );
					}
					for ( auto* collider : collider.getContainer() ) collider->getTransform().reference = &cTransform;

					entity.addChild( *child );
					child->initialize();
					
					if ( mode & ext::gltf::LoadMode::APPLY_TRANSFORMS ) cTransform = {};
				}
			} else {	
				auto& mesh = entity.getComponent<ext::gltf::mesh_t>();
				auto& collider = entity.getComponent<uf::Collider>();
				size_t id = 0;
				for ( auto& primitive : m.primitives ) {
					fillMesh( mesh, primitive, collider, id++ );
				}
			
				if ( (mode & ext::gltf::LoadMode::COLLISION) && !(mode & ext::gltf::LoadMode::AABB) ) {
					auto* c = new uf::MeshCollider( transform );
					c->setPositions( mesh );
					collider.add(c);
				}

				for ( auto* collider : collider.getContainer() ) collider->getTransform().reference = &transform;

				if ( mode & ext::gltf::LoadMode::RENDER ) {
					auto& graphic = entity.getComponent<uf::Graphic>();
					if ( mode & ext::gltf::LoadMode::DEFER_INIT ) {
						graphic.process = false;
					} else {
						graphic.initialize();
						graphic.initializeGeometry( mesh );

						graphic.material.attachShader("./data/shaders/gltf.stereo.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
						graphic.material.attachShader("./data/shaders/gltf.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

						auto& shader = graphic.material.shaders.back();
						struct SpecializationConstant {
							uint32_t textures = 1;
						};
						auto* specializationConstants = (SpecializationConstant*) &shader.specializationConstants[0];
						specializationConstants->textures = graphic.material.textures.size();
						for ( auto& binding : shader.descriptorSetLayoutBindings ) {
							if ( binding.descriptorCount > 1 )
								binding.descriptorCount = specializationConstants->textures;
						}
					}
					if ( useAtlas ) {
						atlas.generate();
						graphic.material.textures.emplace_back().loadFromImage( atlas.getAtlas() );
					} else {
						for ( auto& image : images ) graphic.material.textures.emplace_back().loadFromImage( image );
					}
					for ( auto& sampler : samplers ) graphic.material.samplers.push_back(sampler);
				}
				if ( mode & ext::gltf::LoadMode::APPLY_TRANSFORMS ) transform = {};
			}
		}

		for ( auto i : node.children ) {
			uf::Object* child = new uf::Object;
			entity.addChild( *child );
			legacy::loadNode( *child, model, model.nodes[i], mode );
			child->initialize();
		}
	}

	void process( uf::Object& entity, const tinygltf::Model& model, ext::gltf::load_mode_t mode ) {
		const auto& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];
		for ( auto i : scene.nodes ) legacy::loadNode( entity, model, model.nodes[i], mode );

		auto& transform = entity.getComponent<pod::Transform<>>();
		std::function<void(uf::Entity*)> filter = [&]( uf::Entity* child ) {
			// add default render behavior
			if ( child->hasComponent<uf::Graphic>() ) uf::instantiator::bind("RenderBehavior", *child);

			// parent transform
			if ( child == &entity ) return;
			if ( !child->hasComponent<pod::Transform<>>() ) return;
			child->getComponent<pod::Transform<>>().reference = &transform;
		};
		entity.process(filter);
	}
}

namespace {
	struct Node {
		typedef ext::gltf::skinned_mesh_t Mesh;

		uint32_t index = -1;

		Node* parent = NULL;
		std::vector<Node*> children;

		int32_t skin = -1;
		Mesh mesh;
		uf::Collider collider;

		pod::Transform<> transform;
		pod::Matrix4f local() {
			return 
				uf::matrix::translate( uf::matrix::identity(), transform.position ) *
				uf::quaternion::matrix(transform.orientation) *
				uf::matrix::scale( uf::matrix::identity(), transform.scale ) *
				transform.model;
		}
		Node* find( int32_t index ) {
			if ( parent && parent->index == index ) return parent;
			if ( this->index == index ) return this;

			Node* node = NULL;
			for ( auto& child : children )
				if ( (node = child->find(index)) ) break;
			return node;	
		}
	};
	Node& node() {
		Node* pointer = NULL;
		if ( uf::MemoryPool::global.size() > 0 )
			pointer = &uf::MemoryPool::global.alloc<Node>();
		else
			pointer = new Node;
		return *pointer;
	}

	struct Skin {
		std::string name;
		Node* root = NULL;
		std::vector<Node*> joints;
		std::vector<pod::Matrix4f> inverseBindMatrices;
	};
	struct Animation {
		struct Sampler {
			std::string interpolator;
			std::vector<float> inputs;
			std::vector<pod::Vector4f> outputs;
		};
		struct Channel {
			std::string path;
			Node* node;
			uint32_t sampler;
		};

		std::string name;
		std::vector<Sampler> samplers;
		std::vector<Channel> channels;
		float start = std::numeric_limits<float>::max();
		float end = std::numeric_limits<float>::min();
		float cur = 0;
	};
}
namespace ext {
	namespace gltf {
		struct Graph {
			Node* node;
			uf::Atlas atlas;
			std::vector<ext::vulkan::Sampler> samplers;
			std::vector<Skin> skins;
			std::vector<Animation> animations;
		};
	}
}
namespace {
	Node& loadNode( ext::gltf::Graph& graph, const tinygltf::Model& model, int32_t nodeIndex, Node& parentNode, ext::gltf::load_mode_t mode );
	Node& loadNodes( ext::gltf::Graph& graph, const tinygltf::Model& model, const std::vector<int>& nodes, Node& node, ext::gltf::load_mode_t mode );

	Node& loadNode( ext::gltf::Graph& graph, const tinygltf::Model& model, int32_t nodeIndex, Node& parentNode, ext::gltf::load_mode_t mode ) {
		auto& newNode = ::node();
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
		}
		if ( node.rotation.size() == 4 ) {
			transform.orientation.x = node.rotation[0];
			transform.orientation.y = node.rotation[1];
			transform.orientation.z = node.rotation[2];
			transform.orientation.w = node.rotation[3];
		}
		if ( node.scale.size() == 3 ) {
			transform.scale.x = node.scale[0];
			transform.scale.y = node.scale[1];
			transform.scale.z = node.scale[2];
		}
		if ( node.matrix.size() == 16 ) {
			memcpy( &transform.model[0], node.matrix.data(), node.matrix.size() * sizeof(float) );
		}

		if ( node.children.size() > 0 ) {
			loadNodes( graph, model, node.children, newNode, mode );
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

						if ( (mode & ext::gltf::LoadMode::COLLISION) && (mode & ext::gltf::LoadMode::AABB) ) {
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
					if ( !(mode & ext::gltf::LoadMode::SEPARATE_MESHES) && (mode & ext::gltf::LoadMode::USE_ATLAS) ) {
						vertex.uv = graph.atlas.mapUv( vertex.uv, id );
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
				if ( mode & ext::gltf::LoadMode::GENERATE_NORMALS ) {
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
				if ( mode & ext::gltf::LoadMode::APPLY_TRANSFORMS ) {
					pod::Matrix4f model = uf::transform::model( transform );
					for ( auto& vertex : mesh.vertices ) {
						vertex.position = uf::matrix::multiply<float>( model, vertex.position );
					}
				}
				++id;
			}
			// setup collision
			if ( (mode & ext::gltf::LoadMode::COLLISION) && !(mode & ext::gltf::LoadMode::AABB) ) {
				auto* c = new uf::MeshCollider( transform );
				c->setPositions( mesh );
				collider.add(c);
			}
		}
		return newNode;
	}
	Node& loadNodes( ext::gltf::Graph& graph, const tinygltf::Model& model, const std::vector<int>& nodes, Node& node, ext::gltf::load_mode_t mode ) {
		for ( auto i : nodes ) {
			node.children.emplace_back(&loadNode( graph, model, i, node, mode ));
		}
		return node;
	}

	void postProcessEntity( uf::Object& entity, ext::gltf::Graph& graph, ext::gltf::load_mode_t mode ) {
		auto& mesh = entity.getComponent<ext::gltf::mesh_t>();
		auto& transform = entity.getComponent<pod::Transform<>>();
		auto& collider = entity.getComponent<uf::Collider>();
		
		// setup graphic
		if ( !(mode & ext::gltf::LoadMode::RENDER) ) return;

		auto& graphic = entity.getComponent<uf::Graphic>();

		if ( !(mode & ext::gltf::LoadMode::SEPARATE_MESHES) ) {
			for ( auto& image : graph.atlas.getImages() ) {
				auto& texture = graphic.material.textures.emplace_back();
				texture.loadFromImage( image );
			}

			for ( auto& sampler : graph.samplers )
				graphic.material.samplers.emplace_back( sampler );
		}

		graphic.initialize();
		graphic.initializeGeometry( mesh );
		if ( mode & ext::gltf::LoadMode::DEFER_INIT ) {
			graphic.process = false;
		} else {
			graphic.material.attachShader("./data/shaders/gltf.stereo.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
			graphic.material.attachShader("./data/shaders/gltf.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

			auto& shader = graphic.material.shaders.back();
			struct SpecializationConstant {
				uint32_t textures = 1;
			};
			auto* specializationConstants = (SpecializationConstant*) &shader.specializationConstants[0];
			specializationConstants->textures = graphic.material.textures.size();
			for ( auto& binding : shader.descriptorSetLayoutBindings ) {
				if ( binding.descriptorCount > 1 )
					binding.descriptorCount = specializationConstants->textures;
			}
		}
	}
	void processNode( uf::Object& parent, ext::gltf::Graph& graph, Node& node, ext::gltf::load_mode_t mode, size_t id = 0 ) {
		// create child if requested

		uf::Object* pointer = &parent;
		if ( mode & ext::gltf::LoadMode::SEPARATE_MESHES ) {
			pointer = new uf::Object;
			parent.addChild(*pointer);
		}
		uf::Object& entity = *pointer;

		// copy transform
		auto& transform = entity.getComponent<pod::Transform<>>();
		transform = node.transform;
		// move colliders
		auto& collider = entity.getComponent<uf::Collider>();
		collider.getContainer().insert( collider.getContainer().end(), node.collider.getContainer().begin(), node.collider.getContainer().end() );
		node.collider.getContainer().clear();
		// copy mesh
		auto& mesh = entity.getComponent<ext::gltf::mesh_t>();
		struct {
			size_t vertices = 0;
			size_t indices = 0;
		} start = {
			mesh.vertices.size(),
			mesh.indices.size(),
		};
		mesh.vertices.reserve( node.mesh.vertices.size() + start.vertices );
		mesh.indices.reserve( node.mesh.indices.size() + start.indices );
		for ( auto& v : node.mesh.vertices ) {
			auto& vertex = mesh.vertices.emplace_back( v );
		}
		for ( auto& i : node.mesh.indices ) mesh.indices.emplace_back( i + start.indices );
		// copy image + sampler
		if ( mode & ext::gltf::LoadMode::RENDER ) {
			auto& graphic = entity.getComponent<uf::Graphic>();
			if ( mode & ext::gltf::LoadMode::SEPARATE_MESHES ) {
				auto& images = graph.atlas.getImages();
				auto& image = images.front();
				auto& texture = graphic.material.textures.emplace_back();
				texture.loadFromImage( image );
				images.erase( images.begin() );

				auto& sampler = graph.samplers.front();
				graphic.material.samplers.emplace_back( sampler );
				graph.samplers.erase( graph.samplers.begin() );

				postProcessEntity( entity, graph, mode );
			}
		}

		// go through child nodes
		for ( auto* child : node.children ) processNode( entity, graph, *child, mode, ++id );
	}
	void processEntity( uf::Object& entity, ext::gltf::load_mode_t mode ) {
		auto& graph = entity.getComponent<ext::gltf::Graph>();
		// load vertices into entity for each node
		// if separate meshes is requested, each node will have a corresponding entity
		processNode( entity, graph, *graph.node, mode );
		// initialize graphic
		postProcessEntity( entity, graph, mode );
	/*
		// print node tree
		std::function<void(Node&, size_t)> print = [&]( Node& node, size_t indent ){
			for ( size_t i = 0; i < indent; ++i ) uf::iostream << "\t";
			uf::iostream << "Node: " << node.index << ":\n";
			for ( auto* child : node.children ) 
				print( *child, indent + 1 );
		};
		print( *graph.node, 0 );
		uf::iostream << "\n";
		uf::iostream << "Textures: " << graph.atlas.getImages().size() << "\n";
	*/
	}

	void process( uf::Object& entity, const tinygltf::Model& model, ext::gltf::load_mode_t mode ) {
		auto& graph = entity.getComponent<ext::gltf::Graph>();

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
		//	auto& images = graph.images;
			auto& images = graph.atlas.getImages();
			for ( auto& t : model.textures ) {
				auto& im = model.images[t.source];
			//	auto& image = graph.images.emplace_back();
				auto& image = images.emplace_back();
				image.loadFromBuffer( &im.image[0], {im.width, im.height}, 8, im.component, true );
			}
			graph.atlas.generate();
		}
		// load node information/meshes
		{
			const auto& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];
			auto& node = ::node();
			graph.node = &loadNodes( graph, model, scene.nodes, node, mode );
		}
		// load skins
		{
			graph.skins.reserve( model.skins.size() );
			for ( auto& s : model.skins ) {
				auto& skin = graph.skins.emplace_back();
				skin.name = s.name;
				skin.root = graph.node->find(s.skeleton);
				
				if ( s.inverseBindMatrices > -1 ) {
					const tinygltf::Accessor& accessor = model.accessors[s.inverseBindMatrices];
					const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = model.buffers[view.buffer];
					skin.inverseBindMatrices.resize(accessor.count);
					memcpy(skin.inverseBindMatrices.data(), &buffer.data[accessor.byteOffset + view.byteOffset], accessor.count * sizeof(pod::Matrix4f));
				}

				for ( auto& joint : s.joints ) {
					auto* node = graph.node->find( joint );
					if ( node ) skin.joints.emplace_back( node );
				}
			}
		}
		// load animations
		{
			graph.animations.reserve( model.animations.size() );
			for ( auto& a : model.animations ) {
				auto& animation = graph.animations.emplace_back();
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
						
						for ( size_t i = 0; i < accessor.count; ++i )
							sampler.inputs.push_back( buf[i] );

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
						switch ( accessor.type ) {
							case TINYGLTF_TYPE_VEC3: {
								const pod::Vector3f* buf = static_cast<const pod::Vector3f*>(dataPtr);
								for ( size_t i = 0; i < accessor.count; ++i ) {
									sampler.outputs.emplace_back(pod::Vector4f{
										buf[i].x,
										buf[i].y,
										buf[i].z,
										0
									});
								}
							} break;
							case TINYGLTF_TYPE_VEC4: {
								const pod::Vector4f* buf = static_cast<const pod::Vector4f*>(dataPtr);
								for ( size_t i = 0; i < accessor.count; ++i ) {
									sampler.outputs.emplace_back( buf[i] );
								}
							} break;
						}
					}
				}

				// load channels
				animation.channels.reserve( a.channels.size() );
				for ( auto& c : a.channels ) {
					auto& channel = animation.channels.emplace_back();
					channel.path = c.target_path;
					channel.sampler = c.sampler;
					channel.node = graph.node->find(c.target_node);
				}
			}
		}

		processEntity( entity, mode );

		auto& transform = entity.getComponent<pod::Transform<>>();
		std::function<void(uf::Entity*)> filter = [&]( uf::Entity* child ) {
			// add default render behavior
			if ( child->hasComponent<uf::Graphic>() ) uf::instantiator::bind("RenderBehavior", *child);

			// parent transform
			if ( child == &entity ) return;
			if ( !child->hasComponent<pod::Transform<>>() ) return;
			child->getComponent<pod::Transform<>>().reference = &transform;
		};
		entity.process(filter);
	}
}

bool ext::gltf::load( uf::Object& entity, const std::string& filename, ext::gltf::load_mode_t mode ) {
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;

	std::string warn, err;
	std::string extension = uf::io::extension( filename );
	bool ret;
	if ( extension == "glb" ) 
		ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename); // for binary glTF(.glb)
	else
		ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename);

	if ( !warn.empty() ) std::cout << "glTF warning: " << warn << std::endl;
	if ( !err.empty() ) std::cout << "glTF error: " << err << std::endl;
	if ( !ret ) { std::cout << "glTF error: failed to parse file: " << filename << std::endl;
		return false;
	}

//	legacy::
		process( entity, model, mode );
	
	return true;
}