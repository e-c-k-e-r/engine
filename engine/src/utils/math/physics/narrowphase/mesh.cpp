#include <uf/utils/math/physics/impl.h>
#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/narrowphase.h>
#include <uf/utils/math/physics/broadphase/bvh.h>

bool impl::meshAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( MESH, AABB );

	const auto& mesh = a;
	const auto& aabb = b;

	const auto& bvh  = *mesh.collider.mesh.bvh;
	const auto& meshData = *mesh.collider.mesh.mesh;

	// transform to local space for BVH query
	auto bounds = impl::transformAabbToLocal( aabb.bounds, impl::getTransform( mesh ) );
	STATIC_THREAD_LOCAL(uf::stl::vector<pod::BVH::index_t>, candidates);
	impl::queryBVH( bvh, bounds, candidates );

	bool hit = false;
	// do collision per triangle
	for ( auto triID : candidates ) {
		auto tri = impl::fetchTriangle( meshData, triID, mesh ); // transform triangle to world space
		if ( !impl::triangleAabb( tri, aabb, manifold ) ) continue;
		hit = true;
	}
	return hit;
}
bool impl::meshObb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( MESH, OBB );

	const auto& mesh = a;
	const auto& obb = b;

	const auto& bvh  = *mesh.collider.mesh.bvh;
	const auto& meshData = *mesh.collider.mesh.mesh;

	// transform to local space for BVH query
	auto bounds = impl::transformAabbToLocal( obb.bounds, impl::getTransform( mesh ) );
	STATIC_THREAD_LOCAL(uf::stl::vector<pod::BVH::index_t>, candidates);
	impl::queryBVH( bvh, bounds, candidates );

	bool hit = false;
	// do collision per triangle
	for ( auto triID : candidates ) {
		auto tri = impl::fetchTriangle( meshData, triID, mesh ); // transform triangle to world space
		if ( !impl::triangleObb( tri, obb, manifold ) ) continue;
		hit = true;
	}
	return hit;
}
bool impl::meshSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( MESH, SPHERE );

	const auto& mesh = a;
	const auto& sphere = b;

	const auto& bvh  = *mesh.collider.mesh.bvh;
	const auto& meshData = *mesh.collider.mesh.mesh;

	// transform to local space for BVH query
	auto bounds = impl::transformAabbToLocal( sphere.bounds, impl::getTransform( mesh ) );		
	STATIC_THREAD_LOCAL(uf::stl::vector<pod::BVH::index_t>, candidates);
	impl::queryBVH( bvh, bounds, candidates );

	bool hit = false;
	// do collision per triangle
	for ( auto triID : candidates ) {
		auto tri = impl::fetchTriangle( meshData, triID, mesh ); // transform triangle to world space
		if ( !impl::triangleSphere( tri, sphere, manifold ) ) continue;
		hit = true;
	}

	return hit;
}
// to-do
bool impl::meshPlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( MESH, PLANE );

	const auto& mesh = a;
	const auto& plane = b;

	const auto& bvh  = *mesh.collider.mesh.bvh;
	const auto& meshData = *mesh.collider.mesh.mesh;

	// transform to local space for BVH query
	auto bounds = impl::transformAabbToLocal( plane.bounds, impl::getTransform( mesh ) );		
	STATIC_THREAD_LOCAL(uf::stl::vector<pod::BVH::index_t>, candidates);
	impl::queryBVH( bvh, bounds, candidates );

	bool hit = false;
	// do collision per triangle
	for ( auto triID : candidates ) {
		auto tri = impl::fetchTriangle( meshData, triID, mesh ); // transform triangle to world space
		if ( !impl::trianglePlane( tri, plane, manifold ) ) continue;
		hit = true;
	}

	return hit;
}
bool impl::meshCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( MESH, CAPSULE );

	const auto& mesh = a;
	const auto& capsule = b;

	const auto& bvh  = *mesh.collider.mesh.bvh;
	const auto& meshData = *mesh.collider.mesh.mesh;

	// transform to local space for BVH query
	auto bounds = impl::transformAabbToLocal( capsule.bounds, impl::getTransform( mesh ) );		
	STATIC_THREAD_LOCAL(uf::stl::vector<pod::BVH::index_t>, candidates);
	impl::queryBVH( bvh, bounds, candidates );

	bool hit = false;
	// do collision per triangle
	for ( auto triID : candidates ) {
		auto tri = impl::fetchTriangle( meshData, triID, mesh ); // transform triangle to world space
		if ( !impl::triangleCapsule( tri, capsule, manifold ) ) continue;
		hit = true;
	}

	return hit;
}

bool impl::meshMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( MESH, MESH );

	const auto& bvhA = *a.collider.mesh.bvh;
	const auto& meshA = *a.collider.mesh.mesh;
	
	const auto& bvhB = *b.collider.mesh.bvh;
	const auto& meshB = *b.collider.mesh.mesh;

	auto tA = impl::getTransform( a );
	auto tB = impl::getTransform( b );
	auto relTransform = uf::transform::relative( tA, tB );

	// compute overlaps between one BVH and another BVH
	STATIC_THREAD_LOCAL(pod::BVH::pairs_t, pairs);

	impl::queryOverlaps( bvhA, bvhB, relTransform, pairs );

	bool hit = false;
	// do collision per triangle
	for (auto [idA, idB] : pairs ) {
		auto triA = impl::fetchTriangle( meshA, idA, a ); // transform triangles to world space
		auto triB = impl::fetchTriangle( meshB, idB, b );

		bool collides = impl::triangleTriangle( triA, triB, manifold );
		if ( !collides ) continue;
		hit = true;
	}
	return hit;
}

bool impl::meshHull( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( MESH, CONVEX_HULL );

	const auto& mesh = a;
	const auto& hull = b;

	const auto& bvhA = *a.collider.mesh.bvh;
	const auto& meshA = *a.collider.mesh.mesh;
	
	const auto& bvhB = *b.collider.convexHull.bvh;
	const auto& meshB = *b.collider.convexHull.mesh;

	auto tA = impl::getTransform( a );
	auto tB = impl::getTransform( b );
	auto relTransform = uf::transform::relative( tA, tB );

	// compute overlaps between one BVH and another BVH
	STATIC_THREAD_LOCAL(pod::BVH::pairs_t, pairs);
	impl::queryOverlaps( bvhA, bvhB, relTransform, pairs );

	bool hit = false;
	// do collision per hull and triangle
	for (auto [ triID, hullID ] : pairs ) {
		auto triView = impl::physicsBodyTriView( mesh, triID );
		auto hullView = impl::physicsBodyHullView( hull, hullID );

		bool collides = impl::triangleHull( triView, hullView, manifold );

		if ( !collides ) continue;
		hit = true;
	}
	return hit;
}

void impl::drawMesh( const pod::PhysicsBody& body ) {

	const uf::Mesh* meshData = body.collider.mesh.mesh;
	auto transform = impl::getTransform( body );
	if ( !meshData ) return;
	if ( body.inverseMass == 0.0f ) return;

	size_t totalTriangles = 0;
	for ( const auto& view : meshData->buffer_views ) totalTriangles += view.index.count / 3;

	for ( size_t i = 0; i < totalTriangles; ++i ) {
		auto tri = uf::mesh::fetchTriangle( *meshData, i );
	#if 1
		uf::debug::drawShape( tri, transform );
	#else
		auto v0 = impl::apply( transform, tri.points[0] );
		auto v1 = impl::apply( transform, tri.points[1] );
		auto v2 = impl::apply( transform, tri.points[2] );

		uf::debug::drawLine( v0, v1 );
		uf::debug::drawLine( v1, v2 );
		uf::debug::drawLine( v2, v0 );
	#endif
	}
}

pod::PhysicsBody& uf::physics::initialize( pod::PhysicsBody& body, const uf::Mesh& mesh, bool convex ) {
	if ( !convex ) {
		body.collider.type = pod::ShapeType::MESH;
		body.collider.mesh.mesh = &mesh;
		body.collider.mesh.bvh = new pod::BVH;
		
		auto& bvh = *body.collider.mesh.bvh;
		impl::buildMeshBVH( bvh, mesh, uf::physics::settings.meshBvhCapacity );
	} else {
		body.collider.type = pod::ShapeType::CONVEX_HULL;
		body.collider.convexHull.mesh = &mesh;
		body.collider.convexHull.bvh = new pod::BVH;
		
		auto& bvh = *body.collider.convexHull.bvh;
		impl::buildConvexHullBVH( bvh, mesh/*, uf::physics::settings.meshBvhCapacity*/ );
	}

	body.bounds = impl::computeAABB( body );
	uf::physics::updateInertia( body );
	return body;
}