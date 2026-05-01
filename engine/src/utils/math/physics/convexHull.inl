namespace {
	bool hullGeneric( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		const auto& hull = a;
		const auto& body = b;

		const auto& bvh  = *hull.collider.convexHull.bvh;
		const auto& meshData = *hull.collider.convexHull.mesh;

		// transform to local space for BVH query
		auto bounds = ::transformAabbToLocal( body.bounds, ::getTransform( hull ) );
		static thread_local uf::stl::vector<pod::BVH::index_t> candidates;
		candidates.clear();
		::queryBVH( bvh, bounds, candidates );

		bool hit = false;
		// do collision per hull
		for ( auto hullID : candidates ) {
			auto hullView = ::physicsBodyHullView( hull, hullID );

			pod::Simplex simplex;
			if ( !::gjk( hullView, body, simplex ) ) continue;
			auto result = ::epa( hullView, body, simplex );
			if ( !::generateClippingManifold( hullView, body, result, manifold ) ) continue;
			hit = true;
		}
		return hit;
	}
}

namespace {
	bool hullAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( CONVEX_HULL, AABB );
		return hullGeneric(a, b, manifold, eps);
	}
	bool hullSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( CONVEX_HULL, SPHERE );
		return hullGeneric(a, b, manifold, eps);
	}
	bool hullPlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( CONVEX_HULL, PLANE );
		return hullGeneric(a, b, manifold, eps);
	}
	bool hullCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( CONVEX_HULL, CAPSULE );
		return hullGeneric(a, b, manifold, eps);
	}
	bool hullMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( CONVEX_HULL, MESH );
		REVERSE_COLLIDER( a, b, meshHull );
	}
	bool hullHull( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( CONVEX_HULL, CONVEX_HULL );
		const auto& bvhA = *a.collider.convexHull.bvh;
		const auto& bvhB = *b.collider.convexHull.bvh;

		auto tA = ::getTransform( a );
		auto tB = ::getTransform( b );
		auto relTransform = uf::transform::relative( tA, tB );

		static thread_local pod::BVH::pairs_t pairs;
		pairs.clear();
		::queryOverlaps( bvhA, bvhB, relTransform, pairs );

		bool hit = false;

		for (auto [ viewIdA, viewIdB ] : pairs ) {
			auto viewA = ::physicsBodyHullView( a, viewIdA );
			auto viewB = ::physicsBodyHullView( b, viewIdB );

			pod::Simplex simplex;
			if ( !::gjk( viewA, viewB, simplex ) ) continue;

			auto result = ::epa( viewA, viewB, simplex );
			if ( !::generateClippingManifold( viewA, viewB, result, manifold ) ) continue;
			hit = true;
		}
		return hit;
	}
}