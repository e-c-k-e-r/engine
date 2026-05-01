#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/narrowphase.h>
#include <uf/utils/math/physics/broadphase/bvh.h>

namespace impl {
	bool hullGeneric( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
		const auto& hull = a;
		const auto& body = b;

		const auto& bvh  = *hull.collider.convexHull.bvh;
		const auto& meshData = *hull.collider.convexHull.mesh;

		// transform to local space for BVH query
		auto bounds = impl::transformAabbToLocal( body.bounds, impl::getTransform( hull ) );
		static thread_local uf::stl::vector<pod::BVH::index_t> candidates;
		candidates.clear();
		impl::queryBVH( bvh, bounds, candidates );

		bool hit = false;
		// do collision per hull
		for ( auto hullID : candidates ) {
			auto hullView = impl::physicsBodyHullView( hull, hullID );

			pod::Simplex simplex;
			if ( !impl::gjk( hullView, body, simplex ) ) continue;
			auto result = impl::epa( hullView, body, simplex );
			if ( !impl::generateClippingManifold( hullView, body, result, manifold ) ) continue;
			hit = true;
		}
		return hit;
	}
}

bool impl::hullAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( CONVEX_HULL, AABB );
	return hullGeneric(a, b, manifold );
}
bool impl::hullSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( CONVEX_HULL, SPHERE );
	return hullGeneric(a, b, manifold );
}
bool impl::hullPlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( CONVEX_HULL, PLANE );
	return hullGeneric(a, b, manifold );
}
bool impl::hullCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( CONVEX_HULL, CAPSULE );
	return hullGeneric(a, b, manifold );
}
bool impl::hullMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( CONVEX_HULL, MESH );
	REVERSE_COLLIDER( a, b, impl::meshHull );
}
bool impl::hullHull( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( CONVEX_HULL, CONVEX_HULL );
	const auto& bvhA = *a.collider.convexHull.bvh;
	const auto& bvhB = *b.collider.convexHull.bvh;

	auto tA = impl::getTransform( a );
	auto tB = impl::getTransform( b );
	auto relTransform = uf::transform::relative( tA, tB );

	static thread_local pod::BVH::pairs_t pairs;
	pairs.clear();
	impl::queryOverlaps( bvhA, bvhB, relTransform, pairs );

	bool hit = false;

	for (auto [ viewIdA, viewIdB ] : pairs ) {
		auto viewA = impl::physicsBodyHullView( a, viewIdA );
		auto viewB = impl::physicsBodyHullView( b, viewIdB );

		pod::Simplex simplex;
		if ( !impl::gjk( viewA, viewB, simplex ) ) continue;

		auto result = impl::epa( viewA, viewB, simplex );
		if ( !impl::generateClippingManifold( viewA, viewB, result, manifold ) ) continue;
		hit = true;
	}
	return hit;
}