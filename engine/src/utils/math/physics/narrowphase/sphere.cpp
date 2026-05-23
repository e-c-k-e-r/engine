#include <uf/utils/math/physics/impl.h>
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

void impl::drawSphere( const pod::PhysicsBody& body ) {
	float radius = body.collider.sphere.radius;
	auto transform = impl::getTransform(body);

	const int segments = 16;
	const float PI = 3.14159265359f;
	const float angleIncrement = (2.0f * PI) / segments;

	for ( auto i = 0; i < segments; ++i ) {
		float theta1 = i * angleIncrement;
		float theta2 = (i + 1) * angleIncrement;

		float c1 = std::cos(theta1) * radius;
		float s1 = std::sin(theta1) * radius;
		float c2 = std::cos(theta2) * radius;
		float s2 = std::sin(theta2) * radius;

		pod::Vector3f xy1 = uf::quaternion::rotate(transform.orientation, pod::Vector3f{c1, s1, 0.0f});
		pod::Vector3f xy2 = uf::quaternion::rotate(transform.orientation, pod::Vector3f{c2, s2, 0.0f});

		pod::Vector3f xz1 = uf::quaternion::rotate(transform.orientation, pod::Vector3f{c1, 0.0f, s1});
		pod::Vector3f xz2 = uf::quaternion::rotate(transform.orientation, pod::Vector3f{c2, 0.0f, s2});

		pod::Vector3f yz1 = uf::quaternion::rotate(transform.orientation, pod::Vector3f{0.0f, c1, s1});
		pod::Vector3f yz2 = uf::quaternion::rotate(transform.orientation, pod::Vector3f{0.0f, c2, s2});

		impl::addLine( transform.position + xy1, transform.position + xy2 );
		impl::addLine( transform.position + xz1, transform.position + xz2 );
		impl::addLine( transform.position + yz1, transform.position + yz2 );
	}
}

pod::PhysicsBody& uf::physics::initialize( pod::PhysicsBody& body, const pod::Sphere& sphere ) {
	body.collider.type = pod::ShapeType::SPHERE;
	body.collider.sphere = sphere;
	body.bounds = impl::computeAABB( body );
	uf::physics::updateInertia( body );
	return body;
}