#pragma once

#include "../structs.h"

namespace impl {
	bool obbAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool obbObb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool obbSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool obbPlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool obbCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool obbMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool obbHull( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );

	void drawObb( const pod::PhysicsBody& body );
}

namespace uf {
	namespace physics {
		pod::PhysicsBody& UF_API initialize( pod::PhysicsBody& body, const pod::OBB& obb );
	}
}