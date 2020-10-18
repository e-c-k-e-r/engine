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

void loadNode( uf::Object& entity, const tinygltf::Model& model, const tinygltf::Node& node, uint8_t mode ) {
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

	auto fillMesh = [&]( ext::gltf::mesh_t& mesh, const tinygltf::Primitive& primitive, uf::Collider& collider, size_t num ) {
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
			auto* buffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
			
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
			attribute.components = accessor.ByteStride(view) / sizeof(float);
			size_t len = accessor.count * attribute.components;
			attribute.buffer.reserve( len );
			attribute.buffer.insert( attribute.buffer.end(), &buffer[0], &buffer[len] );
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
				vertex.uv = atlas.mapUv( vertex.uv, num );
			}
			vertex.id = num;
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

			if ( mode & ext::gltf::LoadMode::GENERATE_NORMALS ) {
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

			// validate incides
			size_t maxIndex = mesh.vertices.size();
			for ( size_t i = 0; i < mesh.indices.size(); ++i ) {
				size_t index = mesh.indices[i];
				if ( index >= maxIndex ) {
					std::cout << "mesh.indices["<< i <<"] = " << index << " >= " << maxIndex << std::endl;
				}
			}
		} else {
			// recalc normals
			if ( mode & ext::gltf::LoadMode::GENERATE_NORMALS ) {
				// bool invert = false;
				for ( size_t i = 0; i < mesh.vertices.size(); i+=3 ) {
					auto& a = mesh.vertices[i+0].position;
					auto& b = mesh.vertices[i+1].position;
					auto& c = mesh.vertices[i+2].position;

					pod::Vector3f normal = uf::vector::normalize( uf::vector::cross( b - a, c - a ) );
					mesh.vertices[i+0].normal = normal;
					mesh.vertices[i+1].normal = normal;
					mesh.vertices[i+2].normal = normal;
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
		/*
			{
				static int i = 0;
				auto& pixels = image.getPixels();
				std::string hash = uf::string::sha256( pixels );
				image.save("./textures/" + std::to_string(i++) + "."+hash+".png");
			}
		*/
		}
		if ( mode & ext::gltf::LoadMode::SEPARATE_MESHES ) {
			auto sampler = samplers.begin();
			auto image = images.begin();
			size_t num = 0;
			for ( auto& primitive : m.primitives ) {
				uf::Object* child = new uf::Object;
				
				//ext::gltf::mesh_t mesh;
				auto& mesh = child->getComponent<ext::gltf::mesh_t>();
				auto& cTransform = child->getComponent<pod::Transform<>>();
				cTransform = transform;

				auto& collider = child->getComponent<uf::Collider>();
				fillMesh( mesh, primitive, collider, num++ );

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

			size_t num = 0;
			for ( auto& primitive : m.primitives ) {
				fillMesh( mesh, primitive, collider, num++ );
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
		loadNode( *child, model, model.nodes[i], mode );
		child->initialize();
	}
}

}

bool ext::gltf::load( uf::Object& entity, const std::string& filename, uint8_t mode ) {
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

	auto& metadata = entity.getComponent<uf::Serializer>();
	const auto& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];
	for ( auto i : scene.nodes ) {
		loadNode( entity, model, model.nodes[i], mode );
	}

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
	return true;
}