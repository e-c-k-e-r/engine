#include <uf/ext/gltf/graph.h>
#include <uf/ext/bullet/bullet.h>
#include <uf/utils/math/physics.h>

pod::Node& uf::graph::node() {
	pod::Node* pointer = uf::MemoryPool::global.size() > 0 ? &uf::MemoryPool::global.alloc<pod::Node>() : new pod::Node;
	return *pointer;
}
pod::Matrix4f uf::graph::local( const pod::Node& node ) {
	return
		uf::matrix::translate( uf::matrix::identity(), node.transform.position ) *
		uf::quaternion::matrix(node.transform.orientation) *
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

pod::Node* uf::graph::find( pod::Node* node, const std::string& name ) {
	return node ? find( *node, name ) : NULL;
}
pod::Node* uf::graph::find( const pod::Graph& graph, const std::string& name ) {
	return find( graph.node, name );
}
pod::Node* uf::graph::find( const pod::Node& node, const std::string& name ) {
	if ( node.parent && node.parent->name == name ) return node.parent;
	if ( node.name == name ) return const_cast<pod::Node*>(&node);

	pod::Node* target = NULL;
	for ( auto& child : node.children )
		if ( (target = uf::graph::find(*child, name)) ) break;
	return target;
}

void uf::graph::process( uf::Object& entity ) {
	auto& graph = entity.getComponent<pod::Graph>();
	return process( graph, *graph.node, entity );
}
void uf::graph::process( pod::Graph& graph ) {
	if ( !graph.entity ) graph.entity = new uf::Object;
	process( graph, *graph.node, *graph.entity );

	// add lights
	for ( auto& l : graph.lights ) {
	//	std::cout << l.name << ": " << uf::string::toString( l.transform.position ) << " " << uf::string::toString( l.color ) << "\t" << l.intensity << "\t" << l.range << std::endl;
		auto& light = graph.entity->loadChild("/light.json", false);
		auto& metadata = light.getComponent<uf::Serializer>();
		metadata["light"]["radius"][0] = 0.001;
		metadata["light"]["radius"][1] = l.range <= 0.001f ? 128.0f : l.range;
		metadata["light"]["power"] = l.intensity / 100.0f;
		metadata["light"]["color"][0] = l.color.x;
		metadata["light"]["color"][1] = l.color.y;
		metadata["light"]["color"][2] = l.color.z;
		metadata["light"]["shadows"]["enabled"] = false;
		light.initialize();

		auto& transform = light.getComponent<pod::Transform<>>();
		transform = l.transform;
	}

	graph.entity->process([&]( uf::Entity* entity ) {
		if ( !entity->hasComponent<ext::gltf::mesh_t>() ) return;
		
		auto& mesh = entity->getComponent<ext::gltf::mesh_t>();
		if ( entity->hasComponent<uf::Graphic>() ) {
			auto& graphic = entity->getComponent<uf::Graphic>();
			graphic.initialize();
			graphic.initializeGeometry( mesh );
		}
		if ( graph.mode & ext::gltf::LoadMode::COLLISION ) {
			bool applyTransform = false; //!(graph.mode & ext::gltf::LoadMode::TRANSFORM);
			auto& collider = ext::bullet::create( entity->as<uf::Object>(), mesh, applyTransform, 1 );
			if ( !applyTransform ) {
				btBvhTriangleMeshShape* triangleMeshShape = (btBvhTriangleMeshShape*) collider.shape;
				btTriangleInfoMap* triangleInfoMap = new btTriangleInfoMap();
				triangleInfoMap->m_edgeDistanceThreshold = 0.01f;
			    triangleInfoMap->m_maxEdgeAngleThreshold = SIMD_HALF_PI*0.25;
				if ( applyTransform ) {
					btGenerateInternalEdgeInfo(triangleMeshShape, triangleInfoMap);
				}
				collider.body->setCollisionFlags(collider.body->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
			}
		}
	});
}
void uf::graph::process( pod::Graph& graph, pod::Node& node, uf::Object& parent ) {
	// create child if requested

	uf::Object* pointer = &parent;
	if ( graph.mode & ext::gltf::LoadMode::SEPARATE ) {
		pointer = new uf::Object;
		parent.addChild(*pointer);
	}
	uf::Object& entity = *pointer;
	node.entity = &entity;

	// reference transform to parent
	if ( graph.mode & ext::gltf::LoadMode::SEPARATE ) {
		auto& transform = entity.getComponent<pod::Transform<>>();
		transform = node.transform;
		// is a child
		if ( node.parent != &node ) {
			auto& parent = entity.getParent().getComponent<pod::Transform<>>();
			transform.reference = &parent;
		}
	}
	// set name
	if ( entity.getName() != "Entity" ) {
		entity.setName( node.name );
	}
	// move colliders
/*
	if ( !node.collider.getContainer().empty() ) {
		auto& collider = entity.getComponent<uf::Collider>();
		collider.getContainer().insert( collider.getContainer().end(), node.collider.getContainer().begin(), node.collider.getContainer().end() );
		node.collider.getContainer().clear();
	}
*/
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
		for ( auto& v : node.mesh.vertices ) mesh.vertices.emplace_back( v );
		for ( auto& i : node.mesh.indices ) mesh.indices.emplace_back( i + start.indices );
		// attach collider if requested
	//	if ( (graph.mode & ext::gltf::LoadMode::COLLISION) && !(graph.mode & ext::gltf::LoadMode::AABB) ) {
		// copy image + sampler
		if ( graph.mode & ext::gltf::LoadMode::RENDER ) {
			auto& graphic = entity.getComponent<uf::Graphic>();
			graphic.device = &uf::renderer::device;
			graphic.material.device = &uf::renderer::device;
			graphic.descriptor.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		
			if ( !(graph.mode & ext::gltf::LoadMode::INVERT) ){
				graphic.descriptor.cullMode = VK_CULL_MODE_BACK_BIT;
			} else {
				graphic.descriptor.cullMode = VK_CULL_MODE_FRONT_BIT;
			}
		//	graphic.descriptor.cullMode = VK_CULL_MODE_NONE;
			uf::instantiator::bind("RenderBehavior", entity);
		/*
			if ( graph.mode & ext::gltf::LoadMode::SEPARATE ) {
				if ( graph.mode & ext::gltf::LoadMode::ATLAS ) {
					auto& atlas = *graph.atlas;
					auto& texture = graphic.material.textures.emplace_back();
					texture.loadFromImage( atlas.getAtlas() );
				} else {	
					if ( !graph.images.empty() ) {
						auto& image = graph.images.front();
						auto& texture = graphic.material.textures.emplace_back();
						texture.loadFromImage( image );
						graph.images.erase( graph.images.begin() );
					}
					if ( !graph.samplers.empty() ) {
						auto& sampler = graph.samplers.front();
						graphic.material.samplers.emplace_back( sampler );
						graph.samplers.erase( graph.samplers.begin() );
					}
				}
				graphic.initialize();
				graphic.initializeGeometry( mesh );

				if ( graph.mode & ext::gltf::LoadMode::COLLISION ) {
					auto& collider = ext::bullet::create( entity, mesh, 0 );
				}
			} else 
		*/
			{
				if ( graph.mode & ext::gltf::LoadMode::ATLAS ) {
					auto& atlas = *graph.atlas;
					auto& texture = graphic.material.textures.emplace_back();
					texture.loadFromImage( atlas.getAtlas() );
				} else {
					for ( auto& image : graph.images ) {
						auto& texture = graphic.material.textures.emplace_back();
						texture.loadFromImage( image );
					}
					for ( auto& sampler : graph.samplers ) {
						graphic.material.samplers.emplace_back( sampler );
					}
				}
			}
			if ( graph.mode & ext::gltf::LoadMode::LOAD ) {
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
				auto& skin = graph.skins[node.skin];
				node.jointBufferIndex = graphic.initializeBuffer(
					(void*) skin.inverseBindMatrices.data(),
					skin.inverseBindMatrices.size() * sizeof(pod::Matrix4f),
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					true
				);
			}
			{
				// update mappings
				std::vector<pod::Material::Storage> materials( graph.materials.size() );
				for ( size_t i = 0; i < graph.materials.size(); ++i ) {
					materials[i] = graph.materials[i].storage;
					materials[i].indexMappedTarget = i;
				//	graph.materials[i].id.mappedTarget = i;
				}
				node.materialBufferIndex = graphic.initializeBuffer(
					(void*) materials.data(),
					materials.size() * sizeof(pod::Material::Storage),
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					true
				);
			}
		}
	}

	for ( auto* child : node.children ) uf::graph::process( graph, *child, entity );
}

void uf::graph::override( pod::Graph& graph ) {
	graph.settings.animations.override.a = 0;
	graph.settings.animations.override.map.clear();
	bool toNeutralPose = graph.sequence.empty();
	// store every node's current transform
	{
		std::function<void(pod::Node&)> process = [&]( pod::Node& node ){
			graph.settings.animations.override.map[&node].first = node.transform;
			graph.settings.animations.override.map[&node].second = node.transform;
			if ( toNeutralPose ) {
				graph.settings.animations.override.map[&node].second.position = { 0, 0, 0 };
				graph.settings.animations.override.map[&node].second.orientation = { 0, 0, 0, 1 };
				graph.settings.animations.override.map[&node].second.scale = { 1, 1, 1 };
			}
			for ( auto* child : node.children ) process( *child );
		};
		process( *graph.node );
	}
	// set our destination transform per node
	if ( !toNeutralPose ) {
		std::string name = graph.sequence.front();
		pod::Animation& animation = graph.animations[name];
		for ( auto& channel : animation.channels ) {
			auto& override = graph.settings.animations.override.map[channel.node];
			auto& sampler = animation.samplers[channel.sampler];
			if ( sampler.interpolator != "LINEAR" ) continue;
			for ( size_t i = 0; i < sampler.inputs.size() - 1; ++i ) {
				if ( !(animation.start >= sampler.inputs[i] && animation.start <= sampler.inputs[i+1]) ) continue;
				if ( channel.path == "translation" ) {
					override.second.position = sampler.outputs[i];
				} else if ( channel.path == "rotation" ) {
					override.second.orientation = uf::quaternion::normalize( sampler.outputs[i] );
				} else if ( channel.path == "scale" ) {
					override.second.scale = sampler.outputs[i];
				}
			}
		}
	}
}

void uf::graph::animate( pod::Graph& graph, const std::string& name, float speed, bool immediate ) {
	if ( graph.animations.count( name ) > 0 ) {
		// if already playing, ignore it
		if ( !graph.sequence.empty() && graph.sequence.front() == name ) return;
		if ( immediate ) {
			while ( !graph.sequence.empty() ) graph.sequence.pop();
		}
		bool empty = graph.sequence.empty();
		graph.sequence.emplace(name);
		if ( empty ) uf::graph::override( graph );
		graph.settings.animations.speed = speed;
	}
	update( graph, 0 );
}
void uf::graph::update( pod::Graph& graph ) {
	return update( graph, uf::physics::time::delta );
}
void uf::graph::update( pod::Graph& graph, float delta ) {
	// no override
	if ( graph.sequence.empty() ) goto UPDATE;
	if ( graph.settings.animations.override.a >= 0 ) goto OVERRIDE;
	{
		std::string name = graph.sequence.front();
		pod::Animation* animation = &graph.animations[name];
	//	std::cout << "ANIMATION: " << name << "\t" << animation->cur << std::endl;
		animation->cur += delta * graph.settings.animations.speed; // * graph.settings.animations.override.speed;
		if ( animation->end < animation->cur ) {
			animation->cur = graph.settings.animations.loop ? animation->cur - animation->end : 0;
			// go-to next animation
			if ( !graph.settings.animations.loop ) {
				graph.sequence.pop();
				// out of animations, set to neutral pose
				if ( graph.sequence.empty() ) {
					uf::graph::override( graph );
					goto OVERRIDE;
				}
				name = graph.sequence.front();
				animation = &graph.animations[name];
			}
		}
		for ( auto& channel : animation->channels ) {
			auto& sampler = animation->samplers[channel.sampler];
			if ( sampler.interpolator != "LINEAR" ) continue;
			for ( size_t i = 0; i < sampler.inputs.size() - 1; ++i ) {
				if ( !(animation->cur >= sampler.inputs[i] && animation->cur <= sampler.inputs[i+1]) ) continue;
				float a = (animation->cur - sampler.inputs[i]) / (sampler.inputs[i+1] - sampler.inputs[i]);
				if ( channel.path == "translation" ) {
					channel.node->transform.position = uf::vector::mix( sampler.outputs[i], sampler.outputs[i+1], a );
				} else if ( channel.path == "rotation" ) {
					channel.node->transform.orientation = uf::quaternion::normalize( uf::quaternion::slerp(sampler.outputs[i], sampler.outputs[i+1], a) );
				} else if ( channel.path == "scale" ) {
					channel.node->transform.scale = uf::vector::mix( sampler.outputs[i], sampler.outputs[i+1], a );
				}
			}
		}
		goto UPDATE;
	}
OVERRIDE:
	// std::cout << "OVERRIDED: " << graph.settings.animations.override.a << "\t" << -std::numeric_limits<float>::max() << std::endl;
	for ( auto pair : graph.settings.animations.override.map ) {
		pair.first->transform.position = uf::vector::mix( pair.second.first.position, pair.second.second.position, graph.settings.animations.override.a );
		pair.first->transform.orientation = uf::quaternion::normalize( uf::quaternion::slerp(pair.second.first.orientation, pair.second.second.orientation, graph.settings.animations.override.a) );
		pair.first->transform.scale = uf::vector::mix( pair.second.first.scale, pair.second.second.scale, graph.settings.animations.override.a );
	}
	// finished our overrided interpolation, clear it
	if ( (graph.settings.animations.override.a += delta * graph.settings.animations.override.speed) >= 1 ) {
		graph.settings.animations.override.a = -std::numeric_limits<float>::max();
		graph.settings.animations.override.map.clear();
	}
UPDATE:
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
		if ( graph.settings.animations.override.a >= 0 || !graph.sequence.empty() ) {
			for ( size_t i = 0; i < skin.joints.size(); ++i ) {
				joints[i] = inverseTransform * (matrix(*skin.joints[i]) * skin.inverseBindMatrices[i]);
			}
		}
		if ( node.entity && node.entity->hasComponent<uf::Graphic>() ) {
			auto& graphic = node.entity->getComponent<uf::Graphic>();

			auto& buffer = graphic.buffers.at(node.jointBufferIndex);
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