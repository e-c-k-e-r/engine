#pragma once

#include "../structs.h"

namespace impl {
	bool planeAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool planeObb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool planeSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool planePlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool planeCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool planeMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool planeHull( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );

	void drawPlane( const pod::PhysicsBody& body );
}

namespace uf {
	namespace physics {
		pod::PhysicsBody& UF_API initialize( pod::PhysicsBody& body, const pod::Plane& plane );
	}
}