#pragma once

#include "../structs.h"

namespace impl {
	bool meshAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool meshObb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool meshSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool meshPlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool meshCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool meshMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool meshHull( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );

	void drawMesh( const pod::PhysicsBody& body );
}

namespace uf {
	namespace physics {
		pod::PhysicsBody& UF_API initialize( pod::PhysicsBody& body, const uf::Mesh& mesh, bool convex = false );
		pod::PhysicsBody& UF_API initialize( pod::PhysicsBody& body, const uf::Mesh& mesh, pod::BVH&, bool convex = false );
	}
}