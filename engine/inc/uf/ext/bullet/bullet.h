#pragma once

#include <uf/config.h>
#include <uf/engine/object/object.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/utils/math/collision.h>

#if UF_USE_BULLET
#include <btBulletDynamicsCommon.h>
#endif
namespace pod {
	struct UF_API Bullet {
		size_t uid = 0;
		uf::Object* pointer = NULL;
		float mass = 0.0f;
		pod::Vector3f inertia = {};
		pod::Transform<> transform;

		bool shared = false; // share control of the transform both in-engine and bullet, set to true if you're directly modifying the transform
	#if UF_USE_BULLET
		btRigidBody* body = NULL;
		btCollisionShape* shape = NULL;
	#else
		void* body = NULL;
		void* shape = NULL;
	#endif
	};
}
#if UF_USE_BULLET
namespace ext {
	namespace bullet {
		extern UF_API bool debugDrawEnabled;
		extern UF_API float debugDrawRate;
		extern UF_API size_t iterations;
		extern UF_API size_t substeps;
		extern UF_API float timescale;
		
		extern UF_API size_t defaultMaxCollisionAlgorithmPoolSize;
		extern UF_API size_t defaultMaxPersistentManifoldPoolSize;

		void UF_API initialize();
		void UF_API tick( float = 0 );
		void UF_API terminate();

		// base collider creation
		pod::Bullet& create( uf::Object& );
		void destroy( uf::Object& );

		void UF_API attach( pod::Bullet& );
		void UF_API detach( pod::Bullet& );

		// collider from mesh
	#if 0
		template<typename T, typename U>
		pod::Bullet& create( uf::Object&, const uf::Mesh<T, U>& mesh, bool );
	#endif

		// collider for mesh (static or dynamic)
		pod::Bullet& create( uf::Object&, const uf::Mesh&, bool );
		// collider for boundingbox
		pod::Bullet& UF_API create( uf::Object&, const pod::Vector3f&, float );
		// collider for capsule
		pod::Bullet& UF_API create( uf::Object&, float, float, float );

		// synchronize engine transforms to bullet transforms
		void UF_API syncToBullet();
		// synchronize bullet transforms to engine transforms
		void UF_API syncFromBullet();
		// apply impulse
		void UF_API applyImpulse( pod::Bullet&, const pod::Vector3f& );
		// directly move a transform
		void UF_API applyMovement( pod::Bullet&, const pod::Vector3f& );
		// directly apply a velocity
		void UF_API applyVelocity( pod::Bullet&, const pod::Vector3f& );
		// directly rotate a transform
		void UF_API applyRotation( pod::Bullet&, const pod::Quaternion<>& );
		void UF_API applyRotation( pod::Bullet&, const pod::Vector3f&, float );

		// picks an appropriate movement option
		void UF_API move( pod::Bullet&, const pod::Vector3f&, bool = false );

		// allows noclip
		void UF_API activateCollision( pod::Bullet&, bool = true );

		// allows showing collision models
		void UF_API debugDraw( uf::Object& );
	}
}

#include "bullet.inl"
#endif