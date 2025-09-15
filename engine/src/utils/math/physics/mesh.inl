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
		thread_local uf::stl::vector<pod::BVH::index_t> candidates;
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
		thread_local uf::stl::vector<pod::BVH::index_t> candidates;
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
		thread_local uf::stl::vector<pod::BVH::index_t> candidates;
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
		thread_local uf::stl::vector<pod::BVH::index_t> candidates;
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
		
		const auto& meshB = *b.collider.mesh.mesh;
		const auto& bvhB = *b.collider.mesh.bvh;

		// compute overlaps between one BVH and another BVH
		thread_local pod::BVH::pairs_t pairs;
		pairs.clear();
		::queryOverlaps( bvhA, bvhB, pairs );

		bool hit = false;
		// do collision per triangle
		for (auto [ idA, idB] : pairs ) {
			auto tA = ::fetchTriangle( meshA, idA, a ); // transform triangles to world space
			auto tB = ::fetchTriangle( meshB, idB, b );

			if ( !::triangleTriangle( tA, tB, manifold, eps ) ) continue;
			hit = true;
		}
		return hit;
	}
}