#pragma once

#include "../structs.h"

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

namespace uf {
	namespace physics {
		pod::RayQuery UF_API rayCast( const pod::Ray&, const pod::PhysicsBody&, float = FLT_MAX );
		pod::RayQuery UF_API rayCast( const pod::Ray&, const pod::World&, float = FLT_MAX );
		pod::RayQuery UF_API rayCast( const pod::Ray&, const pod::World&, const pod::PhysicsBody*, float = FLT_MAX );

		float UF_API occlusion( const pod::Vector3f& to, const pod::Vector3f& from );
		uf::stl::string UF_API getRayMaterialName( const pod::RayQuery& query );
		pod::AcousticBounce UF_API acousticReflection( const pod::Vector3f& sourcePos, const pod::Vector3f& rayDirection, const pod::Vector3f& listenerPos, float maxDistance );
	}
}