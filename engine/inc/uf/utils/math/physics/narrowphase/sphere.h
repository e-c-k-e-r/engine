#pragma once

#include "../impl.h"

namespace impl {
	bool sphereSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool sphereAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool spherePlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool sphereCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool sphereMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool sphereHull( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
}