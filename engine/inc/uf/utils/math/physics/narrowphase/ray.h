#pragma once

#include "../impl.h"

namespace impl {
	bool rayTriangleIntersect( const pod::Ray& ray, const pod::Triangle& tri, float& t, float& u, float& v );
	bool rayAabbIntersect( const pod::Ray& ray, const pod::AABB& box, float& tMin, float& tMax );

	bool rayAabb( const pod::Ray& ray, const pod::PhysicsBody& body, pod::RayQuery& rayHit );
	bool rayObb( const pod::Ray& ray, const pod::PhysicsBody& body, pod::RayQuery& rayHit );
	bool raySphere( const pod::Ray& ray, const pod::PhysicsBody& body, pod::RayQuery& rayHit );
	bool rayPlane( const pod::Ray& ray, const pod::PhysicsBody& body, pod::RayQuery& rayHit );
	bool rayCapsule( const pod::Ray& ray, const pod::PhysicsBody& body, pod::RayQuery& rayHit );
	bool rayMesh( const pod::Ray& r, const pod::PhysicsBody& body, pod::RayQuery& rayHit );
	bool rayHull( const pod::Ray& r, const pod::PhysicsBody& body, pod::RayQuery& rayHit );

	void drawRay( const pod::Ray& r, const pod::RayQuery& query );
}