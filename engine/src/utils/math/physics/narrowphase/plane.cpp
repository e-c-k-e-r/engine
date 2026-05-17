#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/narrowphase.h>

bool impl::planeAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( PLANE, AABB );

	const auto& plane = a;
	const auto& aabb = b;

	auto normal = uf::vector::normalize( plane.collider.plane.normal );
	float offset = plane.collider.plane.offset;

	auto center = impl::aabbCenter( aabb.bounds ); // center
	auto extent = impl::aabbExtent( aabb.bounds ); // half extents
	float r = fabs(extent.x * normal.x) + fabs(extent.y * normal.y) + fabs(extent.z * normal.z); // effective projection radius of box onto plane normal

	float dist = uf::vector::dot( normal, center ) - offset;
	if ( dist > r ) return false;

	pod::Vector3f contact = center - normal * dist;
	float penetration = r - dist;

	manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
	return true;
}
bool impl::planeObb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( PLANE, OBB );
	REVERSE_COLLIDER( a, b, impl::obbPlane );
}
bool impl::planeSphere(const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold) {
	ASSERT_COLLIDER_TYPES(PLANE, SPHERE);

	const auto& plane = a;
	const auto& sphere = b;

	auto& normal = plane.collider.plane.normal;
	float offset = plane.collider.plane.offset;
	
	auto center = impl::getPosition( sphere );
	float r = sphere.collider.sphere.radius;

	float dist = uf::vector::dot( normal, center ) - offset;
	if ( dist > r ) return false;

	float penetration = r - dist;
	auto contact = center - normal * r;

	manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
	return true;
}
bool impl::planePlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( PLANE, PLANE );
	return false;
}
bool impl::planeCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( PLANE, CAPSULE );
	REVERSE_COLLIDER( a, b, impl::capsulePlane );
}
bool impl::planeMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( PLANE, MESH );
	REVERSE_COLLIDER( a, b, impl::meshPlane );
}
bool impl::planeHull( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( PLANE, CONVEX_HULL );
	REVERSE_COLLIDER( a, b, impl::hullPlane );
}