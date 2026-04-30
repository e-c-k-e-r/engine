namespace {
	bool meshAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( MESH, AABB );

		const auto& mesh = a;
		const auto& aabb = b;

		const auto& bvh  = *mesh.collider.mesh.bvh;
		const auto& meshData = *mesh.collider.mesh.mesh;

		// transform to local space for BVH query
		auto bounds = ::transformAabbToLocal( aabb.bounds, ::getTransform( mesh ) );
		static thread_local uf::stl::vector<pod::BVH::index_t> candidates;
		candidates.clear();
		::queryBVH( bvh, bounds, candidates );

		bool hit = false;
		// do collision per triangle
		for ( auto triID : candidates ) {
			auto tri = ::fetchTriangle( meshData, triID, mesh ); // transform triangle to world space
			if ( !::triangleAabb( tri, aabb, manifold, eps ) ) continue;
			hit = true;
		}
		return hit;
	}
	bool meshSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( MESH, SPHERE );

		const auto& mesh = a;
		const auto& sphere = b;

		const auto& bvh  = *mesh.collider.mesh.bvh;
		const auto& meshData = *mesh.collider.mesh.mesh;

		// transform to local space for BVH query
		auto bounds = ::transformAabbToLocal( sphere.bounds, ::getTransform( mesh ) );		
		static thread_local uf::stl::vector<pod::BVH::index_t> candidates;
		candidates.clear();
		::queryBVH( bvh, bounds, candidates );

		bool hit = false;
		// do collision per triangle
		for ( auto triID : candidates ) {
			auto tri = ::fetchTriangle( meshData, triID, mesh ); // transform triangle to world space
			if ( !::triangleSphere( tri, sphere, manifold, eps ) ) continue;
			hit = true;
		}

		return hit;
	}
	// to-do
	bool meshPlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( MESH, PLANE );

		const auto& mesh = a;
		const auto& plane = b;

		const auto& bvh  = *mesh.collider.mesh.bvh;
		const auto& meshData = *mesh.collider.mesh.mesh;

		// transform to local space for BVH query
		auto bounds = ::transformAabbToLocal( plane.bounds, ::getTransform( mesh ) );		
		static thread_local uf::stl::vector<pod::BVH::index_t> candidates;
		candidates.clear();
		::queryBVH( bvh, bounds, candidates );

		bool hit = false;
		// do collision per triangle
		for ( auto triID : candidates ) {
			auto tri = ::fetchTriangle( meshData, triID, mesh ); // transform triangle to world space
			if ( !::trianglePlane( tri, plane, manifold, eps ) ) continue;
			hit = true;
		}

		return hit;
	}
	bool meshCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( MESH, CAPSULE );

		const auto& mesh = a;
		const auto& capsule = b;

		const auto& bvh  = *mesh.collider.mesh.bvh;
		const auto& meshData = *mesh.collider.mesh.mesh;

		// transform to local space for BVH query
		auto bounds = ::transformAabbToLocal( capsule.bounds, ::getTransform( mesh ) );		
		static thread_local uf::stl::vector<pod::BVH::index_t> candidates;
		candidates.clear();
		::queryBVH( bvh, bounds, candidates );

		bool hit = false;
		// do collision per triangle
		for ( auto triID : candidates ) {
			auto tri = ::fetchTriangle( meshData, triID, mesh ); // transform triangle to world space
			if ( !::triangleCapsule( tri, capsule, manifold, eps ) ) continue;
			hit = true;
		}

		return hit;
	}

	bool meshMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( MESH, MESH );

		const auto& bvhA = *a.collider.mesh.bvh;
		const auto& meshA = *a.collider.mesh.mesh;
		
		const auto& bvhB = *b.collider.mesh.bvh;
		const auto& meshB = *b.collider.mesh.mesh;

		auto tA = ::getTransform( a );
		auto tB = ::getTransform( b );
		auto relTransform = uf::transform::relative( tA, tB );

		// compute overlaps between one BVH and another BVH
		static thread_local pod::BVH::pairs_t pairs;
		pairs.clear();

		::queryOverlaps( bvhA, bvhB, relTransform, pairs );

		bool hit = false;
		// do collision per triangle
		for (auto [idA, idB] : pairs ) {
			auto triA = ::fetchTriangle( meshA, idA, a ); // transform triangles to world space
			auto triB = ::fetchTriangle( meshB, idB, b );

			bool collides = ::triangleTriangle( triA, triB, manifold, eps );
			if ( !collides ) continue;
			hit = true;
		}
		return hit;
	}

	bool meshHull( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( MESH, CONVEX_HULL );

		const auto& mesh = a;
		const auto& hull = b;

		const auto& bvhA = *a.collider.mesh.bvh;
		const auto& meshA = *a.collider.mesh.mesh;
		
		const auto& bvhB = *b.collider.convexHull.bvh;
		const auto& meshB = *b.collider.convexHull.mesh;

		auto tA = ::getTransform( a );
		auto tB = ::getTransform( b );
		auto relTransform = uf::transform::relative( tA, tB );

		// compute overlaps between one BVH and another BVH
		static thread_local pod::BVH::pairs_t pairs;
		pairs.clear();
		::queryOverlaps( bvhA, bvhB, relTransform, pairs );

		bool hit = false;
		// do collision per hull and triangle
		for (auto [ triID, hullID ] : pairs ) {
			auto triView = ::physicsBodyTriView( mesh, triID );
			auto hullView = ::physicsBodyHullView( hull, hullID );

			bool collides = ::triangleHull( triView, hullView, manifold, eps );

			if ( !collides ) continue;
			hit = true;
		}
		return hit;
	}
}