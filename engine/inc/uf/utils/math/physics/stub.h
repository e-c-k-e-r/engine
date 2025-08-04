// enough things to get it to compile without a physics lib
namespace uf {
	struct Mesh;
}

namespace pod {
	struct UF_API Physics {
		size_t uid = 0;
		uf::Object* object = NULL;
		
		bool shared = false; // share control of the transform both in-engine and bullet, set to true if you're directly modifying the transform
		void* body = NULL;	
		void* shape = NULL;	

		void* world = NULL;
	
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

namespace uf {
	namespace physics {
		namespace impl {
			typedef void* WorldState;
			inline void UF_API initialize() {}
			inline void UF_API initialize( uf::Object& ) {}
			inline void UF_API tick( float = 0 ) {}
			inline void UF_API tick( uf::Object&, float = 0 ) {}
			inline void UF_API terminate() {}
			inline void UF_API terminate( uf::Object& ) {}

			// base collider creation
			inline pod::PhysicsState& UF_API create( uf::Object& ) { static pod::PhysicsState null; return null; }

			inline void UF_API destroy( uf::Object& ) { return; }
			inline void UF_API destroy( pod::PhysicsState& ) { return; }

			inline void UF_API attach( pod::PhysicsState& ) { return; }
			inline void UF_API detach( pod::PhysicsState& ) { return; }

			// collider for mesh (static or dynamic)
			inline pod::PhysicsState& create( uf::Object&, const uf::Mesh&, bool ) { static pod::PhysicsState null; return null; }
			// collider for boundingbox
			inline pod::PhysicsState& UF_API create( uf::Object&, const pod::Vector3f& ) { static pod::PhysicsState null; return null; }
			// collider for capsule
			inline pod::PhysicsState& UF_API create( uf::Object&, float, float ) { static pod::PhysicsState null; return null; }
		/*
			// synchronize engine transforms to bullet transforms
			inline void UF_API syncTo( ext::reactphysics::WorldState& ) { return; }
			// synchronize bullet transforms to engine transforms
			inline void UF_API syncFrom( ext::reactphysics::WorldState&, float = 1 ) { return; }
		*/
			// apply impulse
			inline void UF_API setImpulse( pod::PhysicsState&, const pod::Vector3f& = {} ) { return; }
			inline void UF_API applyImpulse( pod::PhysicsState&, const pod::Vector3f& ) { return; }
			// directly move a transform
			inline void UF_API applyMovement( pod::PhysicsState&, const pod::Vector3f& ) { return; }
			// directly apply a velocity
			inline void UF_API setVelocity( pod::PhysicsState&, const pod::Vector3f& ) { return; }
			inline void UF_API applyVelocity( pod::PhysicsState&, const pod::Vector3f& ) { return; }
			// directly rotate a transform
			inline void UF_API applyRotation( pod::PhysicsState&, const pod::Quaternion<>& ) { return; }
			inline void UF_API applyRotation( pod::PhysicsState&, const pod::Vector3f&, float ) { return; }

			// ray casting
			inline uf::Object* UF_API rayCast( const pod::Vector3f&, const pod::Vector3f& ) { return NULL; }
			inline uf::Object* UF_API rayCast( pod::PhysicsState&, const pod::Vector3f&, const pod::Vector3f& ) { return NULL; }
			inline uf::Object* UF_API rayCast( pod::PhysicsState&, const pod::Vector3f&, const pod::Vector3f&, float& ) { return NULL; }
			inline uf::Object* UF_API rayCast( const pod::Vector3f&, const pod::Vector3f&, uf::Object*, float& ) { return NULL; }

			// allows noclip
			inline void UF_API activateCollision( pod::PhysicsState&, bool = true ) { return; }

			// 
			inline float UF_API getMass( pod::PhysicsState& ) { return {}; }
			inline void UF_API setMass( pod::PhysicsState&, float ) { return; }
		}
	}
}