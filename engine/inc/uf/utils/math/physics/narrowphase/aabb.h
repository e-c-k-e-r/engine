#pragma once

#include "../impl.h"

namespace impl {
	bool aabbAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool aabbObb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool aabbSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool aabbPlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool aabbCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool aabbMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool aabbHull( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );

	void drawAabb( const pod::PhysicsBody& body );
}

namespace uf {
	namespace physics {
		pod::PhysicsBody& UF_API create( uf::Object& object, const pod::AABB& aabb, float mass = 0.0f, const pod::Vector3f& = {} );
		pod::PhysicsBody& UF_API create( pod::World&, uf::Object& object, const pod::AABB& aabb, float mass = 0.0f, const pod::Vector3f& = {} );
	}
}