#pragma once

#include "../impl.h"

namespace impl {
	bool capsuleAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool capsuleObb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool capsuleSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool capsulePlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool capsuleCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool capsuleMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool capsuleHull( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );

	void drawCapsule( const pod::PhysicsBody& body );
}

namespace uf {
	namespace physics {
		pod::PhysicsBody& UF_API create( uf::Object& object, const pod::Capsule& capsule, float mass = 0.0f, const pod::Vector3f& = {} );
		pod::PhysicsBody& UF_API create( pod::World&, uf::Object& object, const pod::Capsule& capsule, float mass = 0.0f, const pod::Vector3f& = {} );
	}
}