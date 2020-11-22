#include <uf/ext/gltf/graph.h>
#include <uf/utils/math/physics.h>

namespace {
	std::string toString( const pod::Vector3f& vector ) {
		std::stringstream ss;
		ss << vector.x << ", " << vector.y << ", " << vector.z;
		return ss.str();
	}
	std::string toString( const pod::Vector4f& vector ) {
		std::stringstream ss;
		ss << vector.x << ", " << vector.y << ", " << vector.z << ", " << vector.w;
		return ss.str();
	}
	std::string toString( const pod::Matrix4f& matrix, size_t indent = 0 ) {
		std::stringstream ss;
		for ( size_t i = 0; i < 16; ++i ) {
			for ( size_t _ = 0; _ < indent; ++_ ) ss << "\t";
			ss << matrix[i] << ", ";
			if ( (i+1)%4 == 0 )  ss << std::endl;
		}
		return ss.str();
	}
}

pod::Node& uf::graph::node() {
	pod::Node* pointer = uf::MemoryPool::global.size() > 0 ? &uf::MemoryPool::global.alloc<pod::Node>() : new pod::Node;
	return *pointer;
}
pod::Matrix4f uf::graph::local( const pod::Node& node ) {
	return
		uf::matrix::translate( uf::matrix::identity(), node.transform.position ) *
		uf::quaternion::matrix(node.transform.orientation) *
	//	uf::matrix::inverse(uf::quaternion::matrix(node.transform.orientation)) *
		uf::matrix::scale( uf::matrix::identity(), node.transform.scale ) *
		node.transform.model;
}
pod::Matrix4f uf::graph::matrix( const pod::Node& node ) {
/*
	if ( !node.parent ) return local(node);
	return matrix( *node.parent ) * local(node);
*/
	pod::Matrix4f matrix = local( node );
	pod::Node* parent = node.parent;
	while ( parent ) {
		matrix = local( *parent ) * matrix;
		parent = parent->parent;
	}
	return matrix;
}

pod::Node* uf::graph::find( pod::Node* node, int32_t index ) {
	return node ? find( *node, index ) : NULL;
}
pod::Node* uf::graph::find( const pod::Graph& graph, int32_t index ) {
	return find( graph.node, index );
}
pod::Node* uf::graph::find( const pod::Node& node, int32_t index ) {
	if ( node.parent && node.parent->index == index ) return node.parent;
	if ( node.index == index ) return const_cast<pod::Node*>(&node);

	pod::Node* target = NULL;
	for ( auto& child : node.children )
		if ( (target = uf::graph::find(*child, index)) ) break;
	return target;
}

void uf::graph::process( uf::Object& entity ) {
	auto& graph = entity.getComponent<pod::Graph>();
	return process( graph, *graph.node, entity );
}
void uf::graph::process( pod::Graph& graph ) {
	if ( !graph.entity ) graph.entity = new uf::Object;
	process( graph, *graph.node, *graph.entity );

	if ( !(graph.mode & ext::gltf::LoadMode::SEPARATE_MESHES) ) {
		graph.entity->process([&]( uf::Entity* entity ) {
			if ( entity->hasComponent<uf::Graphic>() ) {
				auto& graphic = entity->getComponent<uf::Graphic>();
				auto& mesh = entity->getComponent<ext::gltf::mesh_t>();
				graphic.initialize();
				graphic.initializeGeometry( mesh );
			}
		});
	}
}
void uf::graph::process( pod::Graph& graph, pod::Node& node, uf::Object& parent ) {
	// create child if requested

	uf::Object* pointer = &parent;
	if ( graph.mode & ext::gltf::LoadMode::SEPARATE_MESHES ) {
		pointer = new uf::Object;
		parent.addChild(*pointer);
	}
	uf::Object& entity = *pointer;
	node.entity = &entity;

	// copy transform
#if 0
	{
		auto& transform = entity.getComponent<pod::Transform<>>();
		transform = node.transform;
	}
#endif
	// move colliders
	if ( !node.collider.getContainer().empty() ) {
		auto& collider = entity.getComponent<uf::Collider>();
		collider.getContainer().insert( collider.getContainer().end(), node.collider.getContainer().begin(), node.collider.getContainer().end() );
		node.collider.getContainer().clear();
	}
	// copy mesh
	if ( !node.mesh.vertices.empty() ) {
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
		if ( graph.mode & ext::gltf::LoadMode::RENDER ) {
			auto& graphic = entity.getComponent<uf::Graphic>();
			uf::instantiator::bind("RenderBehavior", entity);
			if ( graph.mode & ext::gltf::LoadMode::SEPARATE_MESHES ) {
				auto& images = graph.atlas->getImages();
				if ( !images.empty() ) {
					auto& image = images.front();
					auto& texture = graphic.material.textures.emplace_back();
					texture.loadFromImage( image );
					images.erase( images.begin() );
				}

				if ( !graph.samplers.empty() ) {
					auto& sampler = graph.samplers.front();
					graphic.material.samplers.emplace_back( sampler );
					graph.samplers.erase( graph.samplers.begin() );
				}

				graphic.initialize();
				graphic.initializeGeometry( mesh );
			} else {
				for ( auto& image : graph.atlas->getImages() ) {
					auto& texture = graphic.material.textures.emplace_back();
					texture.loadFromImage( image );
				}

				for ( auto& sampler : graph.samplers )
					graphic.material.samplers.emplace_back( sampler );
			}

			if ( graph.mode & ext::gltf::LoadMode::DEFAULT_LOAD ) {
				if ( graph.mode & ext::gltf::LoadMode::SKINNED ) {
					graphic.material.attachShader("./data/shaders/gltf.stereo.skinned.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
				} else {
					graphic.material.attachShader("./data/shaders/gltf.stereo.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
				}
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
			} else {
				graphic.process = false;
			}

			if ( graph.mode & ext::gltf::LoadMode::SKINNED && node.skin >= 0 ) {
				graphic.device = &uf::renderer::device;
				auto& skin = graph.skins[node.skin];
				size_t index = graphic.initializeBuffer(
					(void*) skin.inverseBindMatrices.data(),
					skin.inverseBindMatrices.size() * sizeof(pod::Matrix4f),
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					true
				);
				node.bufferIndex = index;
			}
		}
	}

	for ( auto* child : node.children ) uf::graph::process( graph, *child, entity );
}

void uf::graph::animate( pod::Graph& graph, const std::string& name ) {
	graph.animation = name;
	update( graph, 0 );
}
void uf::graph::update( pod::Graph& graph ) {
	return update( graph, uf::physics::time::delta );
}
void uf::graph::update( pod::Graph& graph, float delta ) {
	if ( graph.animations.count( graph.animation ) > 0 ) {
		auto& animation = graph.animations[graph.animation];
		animation.cur += delta;
		if ( animation.cur > animation.end ) animation.cur -= animation.end;
		for ( auto& channel : animation.channels ) {
			auto& sampler = animation.samplers[channel.sampler];
			if ( sampler.interpolator != "LINEAR" ) continue;
			for ( size_t i = 0; i < sampler.inputs.size() - 1; ++i ) {
				auto samplerInputCurrent = sampler.inputs[i];
				auto samplerInputNext = sampler.inputs[i+1];
				auto samplerOutputCurrent = sampler.outputs[i];
				auto samplerOutputNext = sampler.outputs[i+1];

				if ( !(animation.cur >= samplerInputCurrent && animation.cur <= samplerInputNext) ) continue;

				float a = (animation.cur - samplerInputCurrent) / (samplerInputNext - samplerInputCurrent);
				if ( channel.path == "translation" ) {
					channel.node->transform.position = uf::vector::mix( samplerOutputCurrent, samplerOutputNext, a );
				} else if ( channel.path == "rotation" ) {
				//	channel.node->transform.orientation = uf::quaternion::normalize( uf::quaternion::slerp(samplerOutputCurrent, samplerOutputNext, a) );
					channel.node->transform.orientation = uf::quaternion::normalize( uf::quaternion::slerp(samplerOutputCurrent, samplerOutputNext, a) );
				//	channel.node->transform.orientation.w *= -1;
				} else if ( channel.path == "scale" ) {
					channel.node->transform.scale = uf::vector::mix( samplerOutputCurrent, samplerOutputNext, a );
				}
			}
		}
	}
	// update joint matrices
	update( graph, *graph.node );
}
void uf::graph::update( pod::Graph& graph, pod::Node& node ) {
	if ( node.skin >= 0 ) {
		pod::Matrix4f nodeMatrix = matrix( node );
		pod::Matrix4f inverseTransform = uf::matrix::inverse( nodeMatrix );
		auto& skin = graph.skins[node.skin];
		std::vector<pod::Matrix4f> joints;
		joints.reserve( skin.joints.size() );
		for ( size_t i = 0; i < skin.joints.size(); ++i ) {
			joints.emplace_back( uf::matrix::identity() );
		}
		if ( graph.animations.count( graph.animation ) > 0 ) {
			for ( size_t i = 0; i < skin.joints.size(); ++i ) {
				joints[i] = inverseTransform * (matrix(*skin.joints[i]) * skin.inverseBindMatrices[i]);
			}
		}
		if ( node.entity && node.entity->hasComponent<uf::Graphic>() ) {
			auto& graphic = node.entity->getComponent<uf::Graphic>();

			auto& buffer = graphic.buffers.at(node.bufferIndex);
			graphic.updateBuffer( (void*) joints.data(), joints.size() * sizeof(pod::Matrix4f), buffer, false );
		}
	}
	for ( auto child : node.children ) update( graph, *child );
}
void uf::graph::destroy( pod::Graph& graph ) {
	{
		std::function<void(pod::Node&)> traverse = [&]( pod::Node& node ) {
			for ( auto* child : node.children ) traverse( *child );
			if ( uf::MemoryPool::global.size() > 0 ) uf::MemoryPool::global.free( &node, sizeof(node) );
			else delete &node;
		};
		traverse( *graph.node );
	/*
		std::vector<pod::Node*> nodes;
		std::function<void(pod::Node&,size_t)> traverse = [&]( pod::Node& node ) {
			nodes.emplace_back( &node );
			for ( auto* child : node.children ) traverse( *child );
		};
		traverse( *graph.node );
		for ( auto& node : nodes ) {
			if ( uf::MemoryPool::global.size() > 0 )
				uf::MemoryPool::global.free( &node, sizeof(node) );
			else
				delete node;
		}
	*/
	}
	delete graph.atlas;
}