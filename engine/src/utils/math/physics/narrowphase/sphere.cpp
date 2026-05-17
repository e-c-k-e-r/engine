#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/narrowphase.h>

bool impl::sphereSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( SPHERE, SPHERE );

	auto delta = impl::getPosition( b ) - impl::getPosition( a );
	float r = a.collider.sphere.radius + b.collider.sphere.radius;

	float dist2 = uf::vector::magnitude( delta );
	if ( dist2 > r * r ) return false;
	float dist = std::sqrt( dist2 );
	
	float penetration = r - dist;
	auto normal = impl::normalizeDelta( delta, dist );
	auto contact = impl::getPosition( a ) + (normal * a.collider.sphere.radius);

	manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
	return true;
}
bool impl::sphereAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( SPHERE, AABB );

	const auto& sphere = a;
	const auto& aabb = b;

	auto center = impl::getPosition( sphere );
	float r = sphere.collider.sphere.radius;

	auto& bounds = aabb.bounds;
	auto closest = pod::Vector3f{
		std::max(bounds.min.x, std::min(center.x, bounds.max.x)),
		std::max(bounds.min.y, std::min(center.y, bounds.max.y)),
		std::max(bounds.min.z, std::min(center.z, bounds.max.z)),
	};

	auto delta = center - closest;
	float dist2 = uf::vector::magnitude( delta );
	if ( dist2 > r * r ) return false;
	float dist = std::sqrt( dist2 );

	float penetration = r - dist;
	auto normal = impl::normalizeDelta( delta, dist );
	auto contact = center - normal * r;
	
	manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
	return true;
}
bool impl::sphereObb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( SPHERE, OBB );
	REVERSE_COLLIDER( a, b, impl::obbSphere );
}
bool impl::spherePlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( SPHERE, PLANE );
	REVERSE_COLLIDER( a, b, impl::planeSphere );
}
bool impl::sphereCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( SPHERE, CAPSULE );
	REVERSE_COLLIDER( a, b, impl::capsuleSphere );
}
bool impl::sphereMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( SPHERE, MESH );
	REVERSE_COLLIDER( a, b, impl::meshSphere );
}
bool impl::sphereHull( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( SPHERE, CONVEX_HULL );
	REVERSE_COLLIDER( a, b, impl::hullSphere );
}