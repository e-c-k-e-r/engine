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
		pod::PhysicsBody& UF_API create( uf::Object&, const uf::Mesh& mesh, float mass = 0.0f, const pod::Vector3f& = {}, bool convex = false );
		pod::PhysicsBody& UF_API create( pod::World&, uf::Object&, const uf::Mesh& mesh, float mass = 0.0f, const pod::Vector3f& = {}, bool convex = false );
	}
}