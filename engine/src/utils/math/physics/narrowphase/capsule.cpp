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
	float radius = body.collider.capsule.radius;
	auto transform = impl::getTransform(body);
	auto [p1, p2] = impl::getCapsuleSegment(body);

	pod::Vector3f up = uf::quaternion::rotate(transform.orientation, pod::Vector3f{0, 1, 0});
	pod::Vector3f right = uf::vector::normalize(impl::computeTangent(up));
	pod::Vector3f forward = uf::vector::cross(up, right);

	pod::Vector3f rightOffset = right * radius;
	pod::Vector3f forwardOffset = forward * radius;

	impl::addLine( p1 + rightOffset, p2 + rightOffset );
	impl::addLine( p1 - rightOffset, p2 - rightOffset );
	impl::addLine( p1 + forwardOffset, p2 + forwardOffset );
	impl::addLine( p1 - forwardOffset, p2 - forwardOffset );

	impl::addLine( p1 + rightOffset, p1 - rightOffset );
	impl::addLine( p1 + forwardOffset, p1 - forwardOffset );
	impl::addLine( p2 + rightOffset, p2 - rightOffset );
	impl::addLine( p2 + forwardOffset, p2 - forwardOffset );
}

pod::PhysicsBody& uf::physics::create( pod::World& world, uf::Object& object, const pod::Capsule& capsule, float mass, const pod::Vector3f& offset ) {
	auto& body = uf::physics::create( world, object, mass, offset );
	body.collider.type = pod::ShapeType::CAPSULE;
	body.collider.capsule = capsule;
	body.bounds = impl::computeAABB( body );
	uf::physics::updateInertia( body );
	return body;
}

pod::PhysicsBody& uf::physics::create( uf::Object& object, const pod::Capsule& capsule, float mass, const pod::Vector3f& offset ) {
	return create( uf::physics::getWorld(), object, capsule, mass, offset );
}