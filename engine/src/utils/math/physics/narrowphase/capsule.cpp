#include <uf/utils/math/physics/impl.h>
#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/narrowphase.h>

bool impl::capsuleCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( CAPSULE, CAPSULE );

	auto [ A1, A2 ] = impl::getCapsuleSegment( a );
	auto [ B1, B2 ] = impl::getCapsuleSegment( b );

	auto [ pA, pB ] = impl::closestSegmentSegment( A1, A2, B1, B2 );
	float r = a.collider.capsule.radius + b.collider.capsule.radius;
	
	auto delta = pB - pA;
	float dist2 = uf::vector::magnitude( delta );
	if ( dist2 > r * r ) return false;
	float dist = std::sqrt( dist2 );
	
	auto contact = ( pA + pB ) * 0.5f;
	auto normal = impl::normalizeDelta( delta, dist );
	float penetration = r - dist;

	manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
	return true;
}
bool impl::capsuleAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( CAPSULE, AABB );
	const auto& capsule = a;
	const auto& box = b;

	auto [ p1, p2 ] = impl::getCapsuleSegment( capsule );
	float r = capsule.collider.capsule.radius;

	auto closestPoint = impl::closestPointSegmentAabb( p1, p2, box.bounds );
	auto closestSegment = impl::closestPointOnSegment( closestPoint, p1, p2 );
	
	auto delta = closestPoint - closestSegment;
	float dist2 = uf::vector::magnitude( delta );
	if ( dist2 > r * r ) return false;
	float dist = std::sqrt( dist2 );

	float penetration = r - dist;
	auto normal = ( dist > EPS ) ? ( delta / dist ) : pod::Vector3f{0,1,0};
	auto contact = closestSegment + normal * ( r - penetration * 0.5f );

	manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
	return true;
}
bool impl::capsuleObb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( CAPSULE, OBB );
	REVERSE_COLLIDER( a, b, impl::obbCapsule );
}
bool impl::capsulePlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( CAPSULE, PLANE );
	const auto& capsule = a;
	const auto& plane = b;

	const auto& normal = plane.collider.plane.normal;
	float o = plane.collider.plane.offset;

	auto [ p1, p2 ] = impl::getCapsuleSegment( capsule );
	float r = capsule.collider.capsule.radius;

	// the "foot" is just whichever end of the capsule is closest to the normal
	auto foot = ( uf::vector::dot( normal, p1 ) < uf::vector::dot( normal, p2 ) ) ? p1 : p2;
	float dist = uf::vector::dot( normal, foot ) - o;
	if ( dist > r ) return false;

	auto contact = foot - (normal * r);
	float penetration = r - dist;
	manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });

	return true;
}
bool impl::capsuleSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( CAPSULE, SPHERE );
	const auto& capsule = a;
	const auto& sphere = b;

	auto [ p1, p2 ] = impl::getCapsuleSegment( capsule );

	auto sphereCenter = impl::getPosition( sphere );
	float r = capsule.collider.capsule.radius + sphere.collider.sphere.radius;

	// closest point on capsule segment to sphere center
	auto closest = impl::closestPointOnSegment( sphereCenter, p1, p2 );

	auto delta = sphereCenter - closest;
	float dist2 = uf::vector::magnitude( delta );
	if ( dist2 > r * r ) return false;
	float dist = std::sqrt( dist2 );

	float penetration = r - dist;
	auto normal = impl::normalizeDelta( delta, dist );
	auto contact = closest + normal * (capsule.collider.capsule.radius - penetration * 0.5f );

	manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
	return true;
}
bool impl::capsuleMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( CAPSULE, MESH );
	REVERSE_COLLIDER( a, b, impl::meshCapsule );
}
bool impl::capsuleHull( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( CAPSULE, CONVEX_HULL );
	REVERSE_COLLIDER( a, b, impl::hullCapsule );
}

void impl::drawCapsule( const pod::PhysicsBody& body ) {
	const auto& capsule  = body.collider.capsule;
	auto transform = impl::getTransform(body);
#if 1
	uf::debug::drawShape( capsule, transform );
#else
	auto [p1, p2] = impl::getCapsuleSegment(body);
	const int segments = 16;
	const float angleIncrement = (2.0f * M_PI) / segments;

	for ( auto i = 0; i < segments; ++i ) {
		float theta1 = i * angleIncrement;
		float theta2 = (i + 1) * angleIncrement;

		float c1 = std::cos(theta1) * capsule.radius;
		float s1 = std::sin(theta1) * capsule.radius;
		float c2 = std::cos(theta2) * capsule.radius;
		float s2 = std::sin(theta2) * capsule.radius;

		pod::Vector3f xy1 = uf::quaternion::rotate(transform.orientation, pod::Vector3f{c1, s1, 0.0f});
		pod::Vector3f xy2 = uf::quaternion::rotate(transform.orientation, pod::Vector3f{c2, s2, 0.0f});

		pod::Vector3f xz1 = uf::quaternion::rotate(transform.orientation, pod::Vector3f{c1, 0.0f, s1});
		pod::Vector3f xz2 = uf::quaternion::rotate(transform.orientation, pod::Vector3f{c2, 0.0f, s2});

		pod::Vector3f yz1 = uf::quaternion::rotate(transform.orientation, pod::Vector3f{0.0f, c1, s1});
		pod::Vector3f yz2 = uf::quaternion::rotate(transform.orientation, pod::Vector3f{0.0f, c2, s2});

		uf::debug::drawLine( p1 + xy1, p1 + xy2 );
		uf::debug::drawLine( p1 + xz1, p1 + xz2 );
		uf::debug::drawLine( p1 + yz1, p1 + yz2 );

		uf::debug::drawLine( p2 + xy1, p2 + xy2 );
		uf::debug::drawLine( p2 + xz1, p2 + xz2 );
		uf::debug::drawLine( p2 + yz1, p2 + yz2 );
	}

	pod::Vector3f rx = uf::quaternion::rotate(transform.orientation, pod::Vector3f{capsule.radius, 0.0f, 0.0f});
	pod::Vector3f ry = uf::quaternion::rotate(transform.orientation, pod::Vector3f{0.0f, capsule.radius, 0.0f});
	pod::Vector3f rz = uf::quaternion::rotate(transform.orientation, pod::Vector3f{0.0f, 0.0f, capsule.radius});

	uf::debug::drawLine( p1 + rx, p2 + rx );
	uf::debug::drawLine( p1 - rx, p2 - rx );

	uf::debug::drawLine( p1 + ry, p2 + ry );
	uf::debug::drawLine( p1 - ry, p2 - ry );

	uf::debug::drawLine( p1 + rz, p2 + rz );
	uf::debug::drawLine( p1 - rz, p2 - rz );
#endif
}

pod::PhysicsBody& uf::physics::initialize( pod::PhysicsBody& body, const pod::Capsule& capsule ) {
	body.collider.type = pod::ShapeType::CAPSULE;
	body.collider.capsule = capsule;
	uf::physics::update( body );
	return body;
}