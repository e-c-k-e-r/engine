#include <uf/utils/math/physics/impl.h>
#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/narrowphase.h>

bool impl::aabbAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES(AABB, AABB);

	const auto& A = a.bounds;
	const auto& B = b.bounds;

	if ( !impl::aabbOverlap( A, B ) ) return false;

	// calculate overlap extents
	auto overlaps = uf::vector::min( A.max, B.max ) - uf::vector::max( A.min, B.min );

	// determine collision axis = smallest overlap
	auto axis = -1;
	float minOverlap = FLT_MAX;
	FOR_EACH(3, {
		if ( overlaps[i] < minOverlap ) {
			minOverlap = overlaps[i];
			axis = i;
		}
	});

	pod::Vector3f delta = impl::getPosition( b ) - impl::getPosition( a );
	pod::Vector3f normal{0,0,0};
	normal[axis] = (delta[axis] < 0 ? -1.0f : 1.0f);

	// build manifold contacts: overlap region corners on the separating axis
	auto Min = uf::vector::max( A.min, B.min );
	auto Max = uf::vector::min( A.max, B.max );

	// on chosen axis, clamp to overlapped rectangle -> 4 potential points
	if ( axis == 0 ) { // x-axis separation, so face-on overlap in YZ plane
		manifold.points.emplace_back(pod::Contact{ { (normal.x > 0 ? A.max.x : A.min.x), Min.y, Min.z }, normal, minOverlap });
		manifold.points.emplace_back(pod::Contact{ { (normal.x > 0 ? A.max.x : A.min.x), Min.y, Max.z }, normal, minOverlap });
		manifold.points.emplace_back(pod::Contact{ { (normal.x > 0 ? A.max.x : A.min.x), Max.y, Min.z }, normal, minOverlap });
		manifold.points.emplace_back(pod::Contact{ { (normal.x > 0 ? A.max.x : A.min.x), Max.y, Max.z }, normal, minOverlap });
	}
	else if ( axis == 1 ) { // y-axis separation, overlap in XZ plane
		manifold.points.emplace_back(pod::Contact{ { Min.x, (normal.y > 0 ? A.max.y : A.min.y), Min.z }, normal, minOverlap });
		manifold.points.emplace_back(pod::Contact{ { Min.x, (normal.y > 0 ? A.max.y : A.min.y), Max.z }, normal, minOverlap });
		manifold.points.emplace_back(pod::Contact{ { Max.x, (normal.y > 0 ? A.max.y : A.min.y), Min.z }, normal, minOverlap });
		manifold.points.emplace_back(pod::Contact{ { Max.x, (normal.y > 0 ? A.max.y : A.min.y), Max.z }, normal, minOverlap });
	}
	else if (axis == 2) { // z-axis separation, overlap in XY plane
		manifold.points.emplace_back(pod::Contact{ { Min.x, Min.y, (normal.z > 0 ? A.max.z : A.min.z) }, normal, minOverlap });
		manifold.points.emplace_back(pod::Contact{ { Min.x, Max.y, (normal.z > 0 ? A.max.z : A.min.z) }, normal, minOverlap });
		manifold.points.emplace_back(pod::Contact{ { Max.x, Min.y, (normal.z > 0 ? A.max.z : A.min.z) }, normal, minOverlap });
		manifold.points.emplace_back(pod::Contact{ { Max.x, Max.y, (normal.z > 0 ? A.max.z : A.min.z) }, normal, minOverlap });
	}

	return true;
}
bool impl::aabbObb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( AABB, OBB );
	REVERSE_COLLIDER( a, b, impl::obbAabb );
}
bool impl::aabbSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( AABB, SPHERE );
	REVERSE_COLLIDER( a, b, impl::sphereAabb );
}
bool impl::aabbPlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( AABB, PLANE );
	REVERSE_COLLIDER( a, b, impl::planeAabb );
}
bool impl::aabbCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( AABB, CAPSULE );
	REVERSE_COLLIDER( a, b, impl::capsuleAabb );
}
bool impl::aabbMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( AABB, MESH );
	REVERSE_COLLIDER( a, b, impl::meshAabb );
}
bool impl::aabbHull( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( AABB, CONVEX_HULL );
	REVERSE_COLLIDER( a, b, impl::hullAabb );
}

void impl::drawAabb( const pod::PhysicsBody& body ) {
	const auto& aabb = body.bounds;
	auto transform = impl::getTransform( body );
#if 1
	uf::debug::drawShape( aabb, transform );
#else
	pod::Vector3f corners[8];
	impl::getCorners( aabb, corners );

	uf::debug::drawLine( corners[0], corners[1] ); uf::debug::drawLine( corners[1], corners[2] );
	uf::debug::drawLine( corners[2], corners[3] ); uf::debug::drawLine( corners[3], corners[0] );

	uf::debug::drawLine( corners[4], corners[5] ); uf::debug::drawLine( corners[5], corners[6] );
	uf::debug::drawLine( corners[6], corners[7] ); uf::debug::drawLine( corners[7], corners[4] );

	uf::debug::drawLine( corners[0], corners[4] ); uf::debug::drawLine( corners[1], corners[5] );
	uf::debug::drawLine( corners[2], corners[6] ); uf::debug::drawLine( corners[3], corners[7] );
#endif
}

pod::PhysicsBody& uf::physics::initialize( pod::PhysicsBody& body, const pod::AABB& aabb ) {
	body.collider.type = pod::ShapeType::AABB;
	body.collider.aabb = aabb;
	body.bounds = impl::computeAABB( body );

	uf::physics::updateInertia( body );
	return body;
}