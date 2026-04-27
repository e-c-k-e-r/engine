/*
// transform capsule line segments to local space
		p1 = uf::transform::applyInverse( mesh.transform, p1 );
		p2 = uf::transform::applyInverse( mesh.transform, p2 );

*/
/*

contact = uf::transform::apply( mesh.transform, contact );
normal  = uf::quaternion::rotate( mesh.transform.orientation, normal );

*/

// 
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
	// to-do
	bool meshMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( MESH, MESH );

		const auto& bvhA = *a.collider.mesh.bvh;
		const auto& meshA = *a.collider.mesh.mesh;
		
		const auto& bvhB = *b.collider.mesh.bvh;
		const auto& meshB = *b.collider.mesh.mesh;

		// compute overlaps between one BVH and another BVH
		static thread_local pod::BVH::pairs_t pairs;
		pairs.clear();


		//UF_TIMER_MULTITRACE_START("Colliding {} ({} indices) <=> {} ({} indices)", a.object->getName(), bvhA.indices.size(), b.object->getName(), bvhB.indices.size());
		//UF_TIMER_MULTITRACE("Querying overlaps...");
		::queryOverlaps( bvhA, bvhB, pairs );
		//UF_TIMER_MULTITRACE("Queried overlaps.");

		bool hit = false;
		// do collision per triangle
		//UF_TIMER_MULTITRACE("Colliding triangles (pairs={})...", pairs.size());
		for (auto [idA, idB] : pairs ) {
			auto tA = ::fetchTriangle( meshA, idA, a ); // transform triangles to world space
			auto tB = ::fetchTriangle( meshB, idB, b );

			bool collides = ::triangleTriangle( tA, tB, manifold, eps );
			if ( !collides ) continue;
			hit = true;
		}
		//UF_TIMER_MULTITRACE("Collided triangles.");
		//UF_TIMER_MULTITRACE_END("Collided mesh.");
		return hit;
	}
}