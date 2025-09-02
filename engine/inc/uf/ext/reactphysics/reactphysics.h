#pragma once

#include <uf/config.h>
#include <uf/engine/object/object.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/engine/graph/graph.h>
#include <uf/utils/math/shapes.h>

#if UF_USE_REACTPHYSICS
#include <reactphysics3d/reactphysics3d.h>

namespace rp3d = reactphysics3d;

namespace pod {
	typedef uint32_t CollisionMask;

	struct PhysicsMaterial {
		float restitution = 0.2f;
		float staticFriction = 0.5f;
		float dynamicFriction = 0.3f;
	};

	struct UF_API Collider {
		pod::CollisionMask mask;

		rp3d::RigidBody* body = NULL;	
		rp3d::CollisionShape* shape = NULL;	
	};

	struct UF_API PhysicsBody {
		rp3d::PhysicsWorld* world = NULL;
		uf::Object* object = NULL;
		
		pod::Transform<> transform = {};
		pod::Vector3f offset = {};

		bool isStatic = false;

		float mass = 1.0f;
		float inverseMass = 1.0f;

		pod::Vector3f velocity = {};
		pod::Vector3f forceAccumulator = {};

		pod::Vector3f angularVelocity = {};
		pod::Vector3f torqueAccumulator = {};

		pod::Vector3f inertiaTensor = { 0, 0, 0 };
		pod::Vector3f inverseInertiaTensor = { 0, 0, 0 };

		pod::Vector3f gravity = { 0.0f, -9.81f, 0.0f }; // an invalid gravity will fallback to world gravity

		pod::Collider collider;
		pod::PhysicsMaterial material;


		// to-do: do something about this
		struct {
			struct {
				pod::Transform<> transform = {};
				pod::Vector3f velocity;
				pod::Vector3f acceleration;
				pod::Quaternion<> angularVelocity;
				pod::Quaternion<> angularAcceleration;
			} current, previous;
		} internal;
	};

	struct Contact {
		pod::Vector3f contact = {};
		pod::Vector3f normal = {};
		float penetration = 0;
	};

	struct RayQuery {
		bool hit = false;
		const pod::PhysicsBody* body;
		pod::Contact contact = { pod::Vector3f{}, pod::Vector3f{}, FLT_MAX };
	};
}

namespace ext {
	namespace reactphysics {
		void UF_API initialize();
		void UF_API initialize( uf::Object& );
		void UF_API tick( float = 0 );
		void UF_API tick( uf::Object&, float = 0 );
		void UF_API terminate();
		void UF_API terminate( uf::Object& );

		extern UF_API float timescale;
		extern UF_API bool interpolate;
		extern UF_API bool shared;
		extern UF_API bool globalStorage;

		typedef rp3d::PhysicsWorld* WorldState;

		namespace gravity {
			enum Mode {
				DEFAULT,
				PER_OBJECT,
				UNIVERSAL
			};

			extern UF_API Mode mode;
			extern UF_API float constant;
		}

		namespace debugDraw {
			extern UF_API bool enabled;
			extern UF_API float rate;
			extern UF_API uf::stl::string layer;
			extern UF_API float lineWidth;
		}

		// base collider creation
		pod::PhysicsBody& UF_API create( uf::Object&, float mass = 0.0f, const pod::Vector3f& = {} );

		void UF_API destroy( uf::Object& );
		void UF_API destroy( pod::PhysicsBody& );

		void UF_API attach( pod::PhysicsBody& );
		void UF_API detach( pod::PhysicsBody& );

		// collider for mesh (static or dynamic)
		pod::PhysicsBody& UF_API create( uf::Object&, const uf::Mesh&, float mass = 0.0f, const pod::Vector3f& = {} );
		// collider for boundingbox
		pod::PhysicsBody& UF_API create( uf::Object&, const pod::AABB&, float mass = 0.0f, const pod::Vector3f& = {} );
		// collider for sphere
		pod::PhysicsBody& UF_API create( uf::Object&, const pod::Sphere&, float mass = 0.0f, const pod::Vector3f& = {} );
		// collider for plane
		pod::PhysicsBody& UF_API create( uf::Object&, const pod::Plane&, float mass = 0.0f, const pod::Vector3f& = {} );
		// collider for capsule
		pod::PhysicsBody& UF_API create( uf::Object&, const pod::Capsule&, float mass = 0.0f, const pod::Vector3f& = {} );

		// update mesh
		void UF_API update( pod::PhysicsBody&, const uf::Mesh&, bool );

		// synchronize engine transforms to bullet transforms
		void UF_API syncTo( ext::reactphysics::WorldState& );
		// synchronize bullet transforms to engine transforms
		void UF_API syncFrom( ext::reactphysics::WorldState&, float = 1 );
		// apply impulse
		void UF_API setImpulse( pod::PhysicsBody&, const pod::Vector3f& = {} );
		void UF_API applyImpulse( pod::PhysicsBody&, const pod::Vector3f& );
		// directly move a transform
		void UF_API applyMovement( pod::PhysicsBody&, const pod::Vector3f& );
		// directly apply a velocity
		void UF_API setVelocity( pod::PhysicsBody&, const pod::Vector3f& );
		void UF_API applyVelocity( pod::PhysicsBody&, const pod::Vector3f& );
		// directly rotate a transform
		void UF_API applyRotation( pod::PhysicsBody&, const pod::Quaternion<>& );
		void UF_API applyRotation( pod::PhysicsBody&, const pod::Vector3f&, float );

		// ray casting
		pod::RayQuery UF_API rayCast( const pod::Ray& ray, const pod::PhysicsBody& body, float max );

		// allows noclip
		void UF_API activateCollision( pod::PhysicsBody&, bool = true );

		// 
		float UF_API getMass( pod::PhysicsBody& );
		void UF_API setMass( pod::PhysicsBody&, float );
	}
}
namespace uf {
	namespace physics {
		namespace impl = ext::reactphysics;
	}
}
#endif