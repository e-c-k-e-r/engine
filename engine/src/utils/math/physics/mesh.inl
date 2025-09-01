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
	bool meshAabb( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( MESH, AABB );

		const auto& mesh = a;
		const auto& aabb = b;

		const auto& meshData = *mesh.collider.u.mesh.mesh;
		const auto& bvh  = *mesh.collider.u.mesh.bvh;

		// transform to local space for BVH query
		auto bounds = ::transformAabbToLocal( aabb.bounds, ::getTransform( mesh ) );
		uf::stl::vector<int> candidates;
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
	bool meshSphere( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( MESH, SPHERE );

		const auto& mesh = a;
		const auto& sphere = b;

		const auto& meshData = *mesh.collider.u.mesh.mesh;
		const auto& bvh  = *mesh.collider.u.mesh.bvh;

		// transform to local space for BVH query
		auto bounds = ::transformAabbToLocal( sphere.bounds, ::getTransform( mesh ) );		
		uf::stl::vector<int> candidates;
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
	bool meshPlane( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( MESH, PLANE );

		const auto& mesh = a;
		const auto& plane = b;

		const auto& meshData = *mesh.collider.u.mesh.mesh;
		const auto& bvh  = *mesh.collider.u.mesh.bvh;

		// transform to local space for BVH query
		auto bounds = ::transformAabbToLocal( plane.bounds, ::getTransform( mesh ) );		
		uf::stl::vector<int> candidates;
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
	bool meshCapsule( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( MESH, CAPSULE );

		const auto& mesh = a;
		const auto& capsule = b;

		const auto& meshData = *mesh.collider.u.mesh.mesh;
		const auto& bvh  = *mesh.collider.u.mesh.bvh;

		// transform to local space for BVH query
		auto bounds = ::transformAabbToLocal( capsule.bounds, ::getTransform( mesh ) );		
		uf::stl::vector<int> candidates;
		::queryBVH( bvh, bounds, candidates );

		UF_MSG_DEBUG("candidates={}", candidates.size());

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
	bool meshMesh( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( MESH, MESH );

		const auto& bvhA = *a.collider.u.mesh.bvh;
		const auto& meshA = *a.collider.u.mesh.mesh;
		
		const auto& meshB = *b.collider.u.mesh.mesh;
		const auto& bvhB = *b.collider.u.mesh.bvh;

		// compute overlaps between one BVH and another BVH
		pod::BVH::pair_t pairs;
		::traverseNodePair(bvhA, 0, bvhB, 0, pairs);

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