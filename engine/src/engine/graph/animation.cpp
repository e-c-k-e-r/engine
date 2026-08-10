#include <uf/engine/graph/graph.h>
#include <uf/ext/gltf/gltf.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/math/physics/constraints.h>
#include <uf/utils/math/physics/common.h>
#include <uf/utils/mesh/grid.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/string/base64.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/memory/map.h>
#include <uf/ext/xatlas/xatlas.h>
#include <uf/ext/ffx/fsr.h>

#include <uf/engine/ext.h>

namespace {
	uf::stl::string keyedID( size_t id ) {
		return FMT_FORMAT("{}", id);
	}

	// lazy load animations if requested
	void loadAnimation( pod::Graph& graph, const uf::stl::string& name ) {
		auto& storage = uf::graph::getStorage( graph );

		if ( storage.animations.map.count(name) == 0 ) return;
		if ( graph.streams.animations.count(name) == 0 ) return;

		auto& animation = storage.animations.map[name];
		auto& animStream = graph.streams.animations[name];


		uf::asset::Read::container_t queue;
		for ( size_t i = 0; i < animation.samplers.size(); ++i ) {
			auto& sampler = animation.samplers[i];
			auto& stream = animStream.samplers[i];

			if ( !sampler.inputs.empty() ) continue;

			if ( stream.inputs.length > 0 ) {
				sampler.inputs.resize( stream.inputs.length / sizeof(float) );
				uf::asset::read( queue, stream.inputs.filename, stream.inputs.offset, stream.inputs.length, (uint8_t*)(sampler.inputs.data()) );
			}

			if ( stream.outputs.length > 0 ) {
				sampler.outputs.resize( stream.outputs.length / sizeof(pod::Vector4f) );
				uf::asset::read( queue, stream.outputs.filename, stream.outputs.offset, stream.outputs.length, (uint8_t*)(sampler.outputs.data()) );
			}
		}

		uf::asset::processIO( queue );
	}

	void unloadAnimation( pod::Graph& graph, const uf::stl::string& name ) {
		auto& storage = uf::graph::getStorage( graph );

		if ( storage.animations.map.count(name) == 0 ) return;
		auto& animation = storage.animations.map[name];

		for ( auto& sampler : animation.samplers ) {
			sampler.inputs.clear();
			sampler.outputs.clear();
		#if UF_ENV_DREAMCAST
			sampler.inputs.shrink_to_fit();
			sampler.outputs.shrink_to_fit();
		#endif
		}
	}

	pod::Matrix4f localMatrix( const pod::Graph& graph, int32_t index ) {
		auto& node = 0 < index && index <= graph.nodes.size() ? graph.nodes[index] : graph.root;
		auto& transform = node.transform;
		return
			uf::matrix::translate( uf::matrix::identity(), transform.position ) *
			uf::quaternion::matrix(transform.orientation) *
			uf::matrix::scale( uf::matrix::identity(), transform.scale );
	}
	pod::Matrix4f worldMatrix( const pod::Graph& graph, int32_t index ) {
		pod::Matrix4f matrix = ::localMatrix( graph, index );
		auto& node = 0 < index && index <= graph.nodes.size() ? graph.nodes[index] : graph.root;
		int32_t parent = node.parent;
		while ( 0 < parent && parent <= graph.nodes.size() ) {
			matrix = ::localMatrix( graph, parent ) * matrix;
			parent = graph.nodes[parent].parent;
		}
		return matrix;
	}
}

pod::Node* uf::graph::find( pod::Graph& graph, int32_t index ) {
	return 0 <= index && index < graph.nodes.size() ? &graph.nodes[index] : NULL;
}
pod::Node* uf::graph::find( pod::Graph& graph, const uf::stl::string& name ) {
	for ( auto& node : graph.nodes ) if ( node.name == name ) return &node;
	return NULL;
}

void uf::graph::override( pod::Graph& graph ) {
	auto& storage = uf::graph::getStorage( graph );

	graph.settings.animations.override.a = 0;
	graph.settings.animations.override.map.clear();
	bool toNeutralPose = graph.sequence.empty();
	// store every node's current transform
	for ( auto& node : graph.nodes ) {
		graph.settings.animations.override.map[node.index].first = node.transform;
		graph.settings.animations.override.map[node.index].second = node.transform;
		if ( toNeutralPose ) {
			graph.settings.animations.override.map[node.index].second.position = { 0, 0, 0 };
			graph.settings.animations.override.map[node.index].second.orientation = { 0, 0, 0, 1 };
			graph.settings.animations.override.map[node.index].second.scale = { 1, 1, 1 };
		}
	}
	// set our destination transform per node
	if ( !toNeutralPose ) {
		uf::stl::string name = graph.sequence.front();
		pod::Animation& animation = storage.animations.map[name];

		// load animation data
		// if ( animation.channels.empty() || animation.samplers.empty() ) ::loadAnimation( graph, name );
		if ( !animation.samplers.empty() && animation.samplers[0].inputs.empty() ) {
			::loadAnimation( graph, name );
		}

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

void uf::graph::animate( pod::Graph& graph, const uf::stl::string& _name, float speed, bool immediate ) {
	auto& storage = uf::graph::getStorage( graph );

	if ( !(graph.metadata["renderer"]["skinned"].as<bool>()) ) return;
	uf::stl::string key = graph.metadata["key"].as<uf::stl::string>("");
	if ( key != "" ) key += ":";
	uf::stl::string name = key + _name;

	if ( storage.animations.map.count( name ) == 0 ) {
		::loadAnimation( graph, name );
	}
	if ( storage.animations.map.count( name ) > 0 ) {
		auto& animation = storage.animations.map[name];

		if ( !animation.samplers.empty() && animation.samplers[0].inputs.empty() ) {
			::loadAnimation( graph, name );
		}

		// if already playing, ignore it
		if ( !graph.sequence.empty() && graph.sequence.front() == name ) return;
		if ( immediate ) {
			while ( !graph.sequence.empty() ) {
				// unload
				if ( graph.settings.stream.animations ) ::unloadAnimation( graph, graph.sequence.front() );
				graph.sequence.pop();
			}
		}
		bool empty = graph.sequence.empty();
		graph.sequence.emplace(name);
		if ( empty ) uf::graph::override( graph );
		graph.settings.animations.speed = speed;
	}
	updateAnimation( graph, 0 );
}

void uf::graph::updateAnimation( pod::Graph& graph, float delta ) {
	// update our instances
	auto& storage = uf::graph::getStorage( graph );

	// no skins
	if ( !(graph.metadata["renderer"]["skinned"].as<bool>()) ) {
		return;
	}

	if ( graph.sequence.empty() ) goto UPDATE;
	if ( graph.settings.animations.override.a >= 0 ) goto OVERRIDE;
	{
		uf::stl::string name = graph.sequence.front();
		pod::Animation* animation = &storage.animations.map[name]; // &graph.animations[name];
		animation->cur += delta * graph.settings.animations.speed; // * graph.settings.animations.override.speed;
		if ( animation->end < animation->cur ) {
			animation->cur = graph.settings.animations.loop ? animation->cur - animation->end : 0;
			// go-to next animation
			if ( !graph.settings.animations.loop ) {
				// unload
				if ( graph.settings.stream.animations ) ::unloadAnimation( graph, graph.sequence.front() );
				graph.sequence.pop();

				// out of animations, set to neutral pose
				if ( graph.sequence.empty() ) {
					uf::graph::override( graph );
					goto OVERRIDE;
				}
				name = graph.sequence.front();
				animation = &storage.animations.map[name]; // &graph.animations[name];
			}
		}

		// load animation data
		// if ( animation->channels.empty() || animation->samplers.empty() ) ::loadAnimation( graph, name );

		for ( auto& channel : animation->channels ) {
			auto& sampler = animation->samplers[channel.sampler];
			if ( sampler.interpolator != "LINEAR" ) continue;
			for ( size_t i = 0; i < sampler.inputs.size() - 1; ++i ) {
				if ( !(animation->cur >= sampler.inputs[i] && animation->cur <= sampler.inputs[i+1]) ) continue;
				float a = (animation->cur - sampler.inputs[i]) / (sampler.inputs[i+1] - sampler.inputs[i]);
				auto& transform = graph.nodes[channel.node].transform;
				if ( channel.path == "translation" ) {
					transform.position = uf::vector::mix( sampler.outputs[i], sampler.outputs[i+1], a );
				} else if ( channel.path == "rotation" ) {
					transform.orientation = uf::quaternion::normalize( uf::quaternion::slerp(sampler.outputs[i], sampler.outputs[i+1], a) );
				} else if ( channel.path == "scale" ) {
					transform.scale = uf::vector::mix( sampler.outputs[i], sampler.outputs[i+1], a );
				}
			}
		}
		goto UPDATE;
	}
OVERRIDE:
	for ( auto pair : graph.settings.animations.override.map ) {
		graph.nodes[pair.first].transform.position = uf::vector::mix( pair.second.first.position, pair.second.second.position, graph.settings.animations.override.a );
		graph.nodes[pair.first].transform.orientation = uf::quaternion::normalize( uf::quaternion::slerp(pair.second.first.orientation, pair.second.second.orientation, graph.settings.animations.override.a) );
		graph.nodes[pair.first].transform.scale = uf::vector::mix( pair.second.first.scale, pair.second.second.scale, graph.settings.animations.override.a );
	}
	// finished our overrided interpolation, clear it
	if ( (graph.settings.animations.override.a += delta * graph.settings.animations.override.speed) >= 1 ) {
		graph.settings.animations.override.a = -std::numeric_limits<float>::max();
		graph.settings.animations.override.map.clear();
	}
UPDATE:
	for ( auto& node : graph.nodes ) uf::graph::updateAnimation( graph, node );
}
void uf::graph::updateAnimation( pod::Graph& graph, pod::Node& node ) {
	auto& storage = uf::graph::getStorage( graph );

	if ( 0 <= node.skin && node.skin < graph.skins.size() ) {
		pod::Matrix4f nodeMatrix = ::worldMatrix( graph, node.index );
		pod::Matrix4f inverseTransform = uf::matrix::inverse( nodeMatrix );

		pod::Matrix4f invArmatureMatrix = uf::matrix::identity();
		if ( graph.root.entity && graph.root.entity->hasComponent<pod::Transform<>>() ) {
			auto armTf = uf::transform::flatten( graph.root.entity->getComponent<pod::Transform<>>() );
			invArmatureMatrix = uf::matrix::inverse( uf::transform::model( armTf ) );
		}

		auto& skinName = graph.skins[node.skin];
		auto& skin = storage.skins[skinName];
		auto objectKeyName = ::keyedID(node.object);
		auto& joints = storage.joints[objectKeyName];
		joints.resize( skin.joints.size() );
		for ( size_t i = 0; i < skin.joints.size(); ++i ) {
			auto nodeID = skin.joints[i];
			auto& node = graph.nodes[nodeID];

			pod::Matrix4f matrix = uf::matrix::identity();
			bool ragdolled = node.entity && node.entity->hasComponent<pod::PhysicsBody>();
			if ( ragdolled ) {
				auto transform = uf::transform::flatten( node.entity->getComponent<pod::Transform<>>() );
				matrix = invArmatureMatrix * uf::transform::model( transform );
			} else if ( graph.settings.animations.override.a >= 0 || !graph.sequence.empty() ) {
				matrix = ::worldMatrix(graph, nodeID);
			} else {
				joints[i] = matrix;
				continue;
			}
			
			joints[i] = inverseTransform * matrix * skin.inverseBindMatrices[i];
		}
	}
}

// separate function in the event something later might need it
uf::stl::vector<pod::Bone> uf::graph::collectBones( const pod::Graph& graph, const pod::Node& node ) {
	auto& storage = uf::graph::getStorage( graph );
	auto& name = graph.skins[node.skin];
	auto& skin = storage.skins[name];

	uf::stl::vector<pod::Bone> bones( graph.nodes.size() );
	for ( auto i = 0; i < skin.joints.size(); ++i ) {
		auto nodeID = skin.joints[i];
		auto& node = graph.nodes[nodeID];
		if ( node.parent < 0 ) continue;
		auto& parent = graph.nodes[node.parent];

		auto tA = uf::transform::flatten( parent.transform );
		auto tB = uf::transform::flatten( node.transform );

		bones[nodeID] = pod::Bone{ tA.position, tB.position };
	}
	return bones;
}
uf::stl::vector<pod::OBB> uf::graph::obbFromSkin( const pod::Graph& graph, const pod::Node& node ) {
	const float wThresold = 0.15f;

	auto& storage = uf::graph::getStorage( graph );
	auto& meshName = graph.meshes[node.mesh];
	auto& skinName = graph.skins[node.skin];
	
	auto& skin = storage.skins[skinName];
	auto& mesh = storage.meshes[meshName];

	uf::stl::vector<pod::AABB> aabbs(skin.joints.size(), {
		pod::Vector3f{ FLT_MAX, FLT_MAX, FLT_MAX },
		pod::Vector3f{-FLT_MAX,-FLT_MAX,-FLT_MAX }
	});

	// iterate through mesh to fetch attributes
	for ( const auto& view : mesh.buffer_views ) {
		auto posView	= view["position"];
		auto jointsView = view["joints"];
		auto weightView = view["weights"];

		for ( auto i = 0; i < view.vertex.count; ++i ) {
			auto pos = uf::mesh::fetchVertex( view, posView, i );
			auto joints = uf::mesh::fetchVertexAttribute<pod::Vector4us>( view, jointsView, i );
			auto weights = uf::mesh::fetchVertexAttribute<pod::Vector4f>( view, weightView, i );

			int bestW = -1;
			float maxWeight = 0.0f;
			for ( auto w = 0; w < 4; ++w ) {
				if ( weights[w] > maxWeight ) {
					maxWeight = weights[w];
					bestW = w;
				}
			}

			if ( bestW != -1 ) {
				float weight = weights[bestW];
				uint16_t jointID = joints[bestW];

				// transform from mesh => bone space
				pod::Vector3f localPos = uf::matrix::multiply( skin.inverseBindMatrices[jointID], pos );

				aabbs[jointID].min = uf::vector::min( aabbs[jointID].min, localPos );
				aabbs[jointID].max = uf::vector::max( aabbs[jointID].max, localPos );
			}
		}
	}

	uf::stl::vector<pod::OBB> bounds(skin.joints.size());
	for ( auto i = 0; i < skin.joints.size(); ++i ) {
		auto& aabb = aabbs[i];
		bounds[i] = pod::OBB{
			.center = (aabb.max + aabb.min) * 0.5f,
			.extent = uf::vector::abs(aabb.max - aabb.min) * 0.5f,
		};
	}
	return bounds;
}

void uf::graph::rigRagdoll( pod::Graph& graph, pod::Node& node ) {
	auto& storage = uf::graph::getStorage( graph );
	auto& name = graph.skins[node.skin];
	auto& skin = storage.skins[name];

	auto bounds = uf::graph::obbFromSkin( graph, node );
	auto bones = uf::graph::collectBones( graph, node );
	auto armatureTransform = uf::transform::flatten( graph.root.entity->getComponent<pod::Transform<>>() );

	// create physics bodies
	uf::stl::vector<pod::PhysicsBody*> bodies( graph.nodes.size(), NULL );
	uf::stl::vector<bool> isJoint( graph.nodes.size(), false );
	for ( auto i = 0; i < skin.joints.size(); ++i ) isJoint[skin.joints[i]] = true;

	const float density = 1000.0f;
	for ( auto i = 0; i < skin.joints.size(); ++i ) {
		auto nodeID = skin.joints[i];
		auto& node = graph.nodes[nodeID];
		if ( node.parent < 0 ) continue; // root node, skip
		auto& entity = *node.entity;
		auto transform = uf::transform::flatten( entity.getComponent<pod::Transform<>>() );
		// copy to modify in-place
		auto bone = bones[nodeID];
		auto obb = bounds[i];
		// invalid bounds
		bool useBone = obb.extent.x > 0;

		// skip leaf bones if they aren't used in the mesh
		if ( !useBone && node.children.empty() ) continue;
		// useBone = false; // disable for now
		auto shapeType = pod::ShapeType::CAPSULE; // default to capsules
		// transform bone into world space
		bone.start = uf::transform::apply( armatureTransform, bone.start );
		bone.end = uf::transform::apply( armatureTransform, bone.end );
		float length = uf::vector::distance( bone.start, bone.end ); // bone length in world-space
		float thickness = useBone ? MAX( obb.extent.x, obb.extent.z ) : length * 0.15f; // limb thickness
		// transform into node space
		auto start = uf::transform::applyInverse( transform, bone.start );
		auto end = uf::transform::applyInverse( transform, bone.end );
		auto dir = uf::vector::normalize( end - start );

		auto offset = pod::Vector3f{};
		auto orientation = uf::quaternion::identity();

		float mass = 0.0f;
		if ( false && useBone ) {
			shapeType = pod::ShapeType::OBB;
		//	volume = obb.extent.x * obb.extent.y * obb.extent.z * 8.0f;
		} else {
			offset = uf::vector::lerp( start, end, 0.5f );
		//	auto up = pod::Vector3f{0, 1, 0};
		//	orientation = uf::quaternion::unitVectors( up, dir );
		//	dir = up;
		}


		// create body
		auto& body = uf::physics::create( entity, mass, offset, orientation );
		bodies[nodeID] = &body;

		// bone unused, just mark it as non-colliding
		if ( !useBone ) uf::physics::setColliderCategory( body, pod::Collider::CATEGORY_NONE );

		switch ( shapeType ) {
			case pod::ShapeType::CAPSULE: {
				float height = length - (thickness * 2); // subtract end-caps
				uf::physics::initialize( body, pod::Capsule{ thickness, dir * height * 0.5f } );
			} break;
			case pod::ShapeType::AABB:
			case pod::ShapeType::OBB: {
				uf::physics::initialize( body, obb );
			} break;
			// should probably add a NONE shape
			default:
			case pod::ShapeType::SPHERE: {
				uf::physics::initialize( body, pod::Sphere{ 0.01f } );
			} break;
		}
	}

	// create constraints
	for ( auto i = 0; i < skin.joints.size(); ++i ) {
		auto nodeID = skin.joints[i];
		auto& node = graph.nodes[nodeID];
		if ( !bodies[node.parent] ) continue; // no parent body, skip constraint
		if ( !bodies[nodeID] ) continue; // no node body, skip constraint
		
		auto& bodyA = *bodies[node.parent];
		auto& bodyB = *bodies[nodeID];

		auto tA = uf::transform::flatten( *bodyA.transform ); // bone start
		auto tB = uf::transform::flatten( *bodyB.transform ); // bone end

		auto pivot = tA.position;
		auto axis = uf::vector::normalize( tB.position - tA.position );

		auto& constraint = uf::physics::constrain( bodyA, bodyB );
		uf::physics::constrainConeTwist( constraint, pivot, axis, M_PI, M_PI );
	}

	// works only pre-init and for no scaling
#if 0
	for ( auto i = 0; i < skin.joints.size(); ++i ) {
		auto nodeID = skin.joints[i];
		auto& node = graph.nodes[nodeID];
		if ( node.parent < 0 ) continue;
		auto& entity = *node.entity;

		auto boneStart = uf::transform::applyInverse( node.transform, pod::Vector3f{0, 0, 0} );
		auto offset = uf::vector::lerp( boneStart, pod::Vector3f{}, 0.5f );
		auto orientation = uf::quaternion::identity();
		// float length = ...

		float mass = 0.0f;
		auto& body = uf::physics::create( entity, mass, offset, orientation );
		uf::physics::initialize( body, pod::Sphere{ 0.5f } );

		bodies[nodeID] = &body;
	}

	// create constraints
	for ( auto i = 0; i < skin.joints.size(); ++i ) {
		auto nodeID = skin.joints[i];
		auto& node = graph.nodes[nodeID];
		if ( node.parent < 0 ) continue;
		auto& parent = graph.nodes[node.parent];
		if ( bodies.count( node.parent ) == 0 ) continue;
		if ( bodies.count( nodeID ) == 0 ) continue;
		
		auto& bodyA = bodies[node.parent];
		auto& bodyB = bodies[nodeID];

		auto tA = uf::transform::flatten( parent.transform );
		auto tB = uf::transform::flatten( node.transform );

		auto pivot = tA.position;
		auto axis = uf::vector::normalize( tB.position - tA.position );

		auto& constraint = uf::physics::constrain( *bodyA, *bodyB );
		uf::physics::constrainConeTwist( constraint, pivot, axis );
	}
#endif
}