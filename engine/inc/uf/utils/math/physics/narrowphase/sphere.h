#pragma once

#include "../structs.h"

namespace impl {
	bool sphereAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool sphereObb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool sphereSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool spherePlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool sphereCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool sphereMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool sphereHull( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );

	void drawSphere( const pod::PhysicsBody& body );
}

namespace uf {
	namespace physics {
		pod::PhysicsBody& UF_API initialize( pod::PhysicsBody& body, const pod::Sphere& sphere );
	}
}