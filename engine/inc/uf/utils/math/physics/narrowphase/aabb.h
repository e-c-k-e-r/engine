#pragma once

#include "../impl.h"

namespace impl {
	bool aabbAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool aabbSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool aabbPlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool aabbCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool aabbMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool aabbHull( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
}