#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <gltf/tiny_gltf.h>

#include <uf/ext/gltf/gltf.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/graphic/mesh.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/math/collision.h>

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

	auto fillMesh = [&]( uf::BaseMesh<pod::Vertex_3F2F3F, uint32_t>& mesh, const tinygltf::Primitive& primitive, uf::Collider& collider ) {
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
			attribute.buffer.reserve( accessor.count * attribute.components );
			attribute.buffer.insert( attribute.buffer.end(), &buffer[0], &buffer[accessor.count * attribute.components] );
		}

		mesh.vertices.reserve( vertices + verticesStart );

		for ( size_t i = 0; i < vertices; ++i ) {
			auto& vertex = mesh.vertices.emplace_back();

			#define ITERATE_ATTRIBUTE( name, member )\
				if ( !attributes[name].buffer.empty() ) { \
					for ( size_t j = 0; j < attributes[name].components; ++j )\
						vertex.member[j] = attributes[name].buffer[i * attributes[name].components + j];\
				}

			ITERATE_ATTRIBUTE("POSITION", position);
			ITERATE_ATTRIBUTE("TEXCOORD_0", uv);
			ITERATE_ATTRIBUTE("NORMAL", normal);

			#undef ITERATE_ATTRIBUTE
		}


		if ( primitive.indices > -1 ) {
			auto& accessor = model.accessors[primitive.indices];
			auto& view = model.bufferViews[accessor.bufferView];
			auto& buffer = model.buffers[view.buffer];

			mesh.indices.reserve( accessor.count + indicesStart );

			const void* pointer = &(buffer.data[accessor.byteOffset + view.byteOffset]);
			switch (accessor.componentType) {
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
					auto* buf = static_cast<const uint32_t*>( pointer );
					for (size_t index = 0; index < accessor.count; index++) mesh.indices.push_back(buf[index] + verticesStart );
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
					auto* buf = static_cast<const uint16_t*>( pointer );
					for (size_t index = 0; index < accessor.count; index++) mesh.indices.push_back(buf[index] + verticesStart );
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
					auto* buf = static_cast<const uint8_t*>( pointer );
					for (size_t index = 0; index < accessor.count; index++) mesh.indices.push_back(buf[index] + verticesStart );
					break;
				}
			}

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
		} else {
			// recalc normals
			if ( mode & ext::gltf::LoadMode::GENERATE_NORMALS ) {
				// bool invert = false;
				for ( size_t i = 0; i < mesh.vertices.size(); i+=3 ) {
				//	auto& b = mesh.vertices[i+(invert ? 2 : 1)].position;
				//	auto& c = mesh.vertices[i+(invert ? 1 : 2)].position;
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
		std::vector<uf::Image> images;
	/*
		std::vector<uf::Object> lights;
		for ( auto& l : model.lights ) {
			auto& light = lights.emplace_back();
			auto& metadata = light.getComponent<uf::Serializer>();
			metadata["light"]["color"][0] = l.color[0];
			metadata["light"]["color"][1] = l.color[1];
			metadata["light"]["color"][2] = l.color[2];
			
			metadata["light"]["radius"] = l.range;
			metadata["light"]["power"] = l.intensity;

			std::cout << metadata << std::endl;
		}
	*/
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
		//	std::cout << "Loading image: " << im.width << ", " << im.height << std::endl;
			image.loadFromBuffer( &im.image[0], {im.width, im.height}, 8, im.component, true );
		}
	//	if ( model.textures.size() > 1 && m.primitives.size() == model.textures.size() ) {
		if ( mode & ext::gltf::LoadMode::SEPARATE_MESHES ) {
			auto sampler = samplers.begin();
			auto image = images.begin();
			for ( auto& primitive : m.primitives ) {
				uf::Object* child = new uf::Object;
				
				uf::BaseMesh<pod::Vertex_3F2F3F, uint32_t> mesh;
				auto& cTransform = child->getComponent<pod::Transform<>>();
				cTransform = transform;

				auto& collider = child->getComponent<uf::Collider>();
				fillMesh( mesh, primitive, collider );

				if ( (mode & ext::gltf::LoadMode::COLLISION) && !(mode & ext::gltf::LoadMode::AABB) ) {
					auto* box = new uf::MeshCollider( cTransform );
					box->setPositions( mesh );
					collider.add(box);
				}
				if ( mode & ext::gltf::LoadMode::RENDER ) {
					auto& graphic = child->getComponent<uf::Graphic>();
				//	graphic.descriptor.cullMode = VK_CULL_MODE_NONE;
					graphic.initialize();
					graphic.initializeGeometry( mesh );

					graphic.material.attachShader("./data/shaders/base.stereo.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
					graphic.material.attachShader("./data/shaders/base.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
					
					if ( image != images.end() ) graphic.material.textures.emplace_back().loadFromImage( *(image++) );
					if ( sampler != samplers.end() ) graphic.material.samplers.push_back( *(sampler++) );
				}
				for ( auto* collider : collider.getContainer() ) collider->getTransform().reference = &cTransform;

				entity.addChild( *child );
				child->initialize();
				
				if ( mode & ext::gltf::LoadMode::APPLY_TRANSFORMS ) cTransform = {};
			}
		} else {		
			uf::BaseMesh<pod::Vertex_3F2F3F, uint32_t> mesh;
			auto& collider = entity.getComponent<uf::Collider>();

			for ( auto& primitive : m.primitives ) fillMesh( mesh, primitive, collider );
		
			if ( (mode & ext::gltf::LoadMode::COLLISION) && !(mode & ext::gltf::LoadMode::AABB) ) {
				auto* c = new uf::MeshCollider( transform );
				c->setPositions( mesh );
				collider.add(c);
			}

			for ( auto* collider : collider.getContainer() ) collider->getTransform().reference = &transform;

			if ( mode & ext::gltf::LoadMode::RENDER ) {
				auto& graphic = entity.getComponent<uf::Graphic>();

				graphic.initialize();
				graphic.initializeGeometry( mesh );

				graphic.material.attachShader("./data/shaders/base.stereo.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
				graphic.material.attachShader("./data/shaders/base.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

				for ( auto& image : images ) graphic.material.textures.emplace_back().loadFromImage( image );
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
	std::string extension = uf::string::extension( filename );
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

	const auto& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];
	for ( auto i : scene.nodes ) {
		loadNode( entity, model, model.nodes[i], mode );
	}

	// parent transform
	auto& transform = entity.getComponent<pod::Transform<>>();
	std::function<void(uf::Entity*)> filter = [&]( uf::Entity* child ) {
		if ( child == &entity ) return;
		if ( !child->hasComponent<pod::Transform<>>() ) return;
		child->getComponent<pod::Transform<>>().reference = &transform;
	};
	entity.process(filter);

	return true;
}