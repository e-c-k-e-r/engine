#pragma once

#include <uf/config.h>
#include <uf/engine/object/object.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/utils/math/collision.h>
#include <uf/engine/graph/graph.h>

#if UF_USE_REACTPHYSICS
#include <reactphysics3d/reactphysics3d.h>

namespace rp3d = reactphysics3d;

namespace pod {
	struct UF_API Physics {
		size_t uid = 0;
		uf::Object* object = NULL;
		
		bool shared = false; // share control of the transform both in-engine and bullet, set to true if you're directly modifying the transform
		rp3d::RigidBody* body = NULL;	
		rp3d::CollisionShape* shape = NULL;	
	
		pod::Transform<> transform = {};
		
		struct Linear {
			pod::Vector3f velocity;
			pod::Vector3f acceleration;
		} linear;
		struct Angular {
			pod::Quaternion<> velocity;
			pod::Quaternion<> acceleration;
		} rotational;

		struct {
			struct {
				pod::Transform<> transform = {};
				Linear linear;
				Angular rotational;
			} current, previous;
		} internal;


		struct {
			uint32_t flags = 0;
			float mass = 0.0f;
			float friction = 0.8f;
			float restitution = 0.0f;
			pod::Vector3f inertia = {0, 0, 0};
			pod::Vector3f gravity = {0, 0, 0};
		} stats;
	};

	typedef Physics PhysicsState;
}

namespace ext {
	namespace reactphysics {
		void UF_API initialize();
		void UF_API tick( float = 0 );
		void UF_API terminate();

		extern UF_API float timescale;
		extern UF_API bool interpolate;
		extern UF_API bool shared;

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
		pod::PhysicsState& UF_API create( uf::Object& );

		void UF_API destroy( uf::Object& );
		void UF_API destroy( pod::PhysicsState& );

		void UF_API attach( pod::PhysicsState& );
		void UF_API detach( pod::PhysicsState& );

		// collider for mesh (static or dynamic)
		pod::PhysicsState& create( uf::Object&, const uf::Mesh&, bool );
		// collider for boundingbox
		pod::PhysicsState& UF_API create( uf::Object&, const pod::Vector3f& );
		// collider for capsule
		pod::PhysicsState& UF_API create( uf::Object&, float, float );

		// synchronize engine transforms to bullet transforms
		void UF_API syncTo();
		// synchronize bullet transforms to engine transforms
		void UF_API syncFrom( float = 1 );
		// apply impulse
		void UF_API setImpulse( pod::PhysicsState&, const pod::Vector3f& = {} );
		void UF_API applyImpulse( pod::PhysicsState&, const pod::Vector3f& );
		// directly move a transform
		void UF_API applyMovement( pod::PhysicsState&, const pod::Vector3f& );
		// directly apply a velocity
		void UF_API setVelocity( pod::PhysicsState&, const pod::Vector3f& );
		void UF_API applyVelocity( pod::PhysicsState&, const pod::Vector3f& );
		// directly rotate a transform
		void UF_API applyRotation( pod::PhysicsState&, const pod::Quaternion<>& );
		void UF_API applyRotation( pod::PhysicsState&, const pod::Vector3f&, float );

		// ray casting
		uf::Object* UF_API rayCast( const pod::Vector3f&, const pod::Vector3f& );		
		uf::Object* UF_API rayCast( pod::PhysicsState&, const pod::Vector3f&, const pod::Vector3f& );		
		uf::Object* UF_API rayCast( pod::PhysicsState&, const pod::Vector3f&, const pod::Vector3f&, float& );
		uf::Object* UF_API rayCast( const pod::Vector3f&, const pod::Vector3f&, uf::Object*, float& );
//		float UF_API rayCast( const pod::Vector3f&, const pod::Vector3f&, uf::Object*, uf::Object*& );

		// allows noclip
		void UF_API activateCollision( pod::PhysicsState&, bool = true );

		// 
		float UF_API getMass( pod::PhysicsState& );
		void UF_API setMass( pod::PhysicsState&, float );
	}
}
namespace uf {
	namespace physics {
		namespace impl = ext::reactphysics;
	}
}
#endif