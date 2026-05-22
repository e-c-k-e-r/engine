#pragma once

#include "../impl.h"

namespace impl {
	bool triangleTriangle( const pod::TriangleWithNormal& a, const pod::TriangleWithNormal& b, pod::Manifold& manifold );
	bool triangleAabb( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold );
	bool triangleObb( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold );
	bool triangleSphere( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold );
	bool trianglePlane( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold );
	bool triangleCapsule( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold );
	bool triangleHull( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold );

	bool triangleTriangle( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool triangleAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool triangleObb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool triangleSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool trianglePlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool triangleCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );
	bool triangleHull( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold );

	void drawTriangle( const pod::PhysicsBody& body );
}

namespace uf {
	namespace physics {
		pod::PhysicsBody& UF_API create( uf::Object& object, const pod::TriangleWithNormal& tri, float mass = 0.0f, const pod::Vector3f& = {} );
		pod::PhysicsBody& UF_API create( pod::World&, uf::Object& object, const pod::TriangleWithNormal& tri, float mass = 0.0f, const pod::Vector3f& = {} );
	}
}