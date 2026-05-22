#include <uf/utils/math/physics/impl.h>
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

void impl::drawPlane( const pod::PhysicsBody& body ) {
	auto transform = impl::getTransform(body);

	pod::Vector3f right = uf::quaternion::rotate(transform.orientation, pod::Vector3f{1, 0, 0});
	pod::Vector3f forward = uf::quaternion::rotate(transform.orientation, pod::Vector3f{0, 0, 1});

	float size = 10.0f;
	pod::Vector3f p0 = transform.position + (right * size) + (forward * size);
	pod::Vector3f p1 = transform.position - (right * size) + (forward * size);
	pod::Vector3f p2 = transform.position - (right * size) - (forward * size);
	pod::Vector3f p3 = transform.position + (right * size) - (forward * size);

	impl::addLine( p0, p1 );
	impl::addLine( p1, p2 );
	impl::addLine( p2, p3 );
	impl::addLine( p3, p0 );

	impl::addLine( p0, p2 );
	impl::addLine( p1, p3 );
}

pod::PhysicsBody& uf::physics::create( pod::World& world, uf::Object& object, const pod::Plane& plane, float mass, const pod::Vector3f& offset ) {
	auto& body = uf::physics::create( world, object, mass, offset );
	body.collider.type = pod::ShapeType::PLANE;
	body.collider.plane = plane;
	body.bounds = impl::computeAABB( body );
	uf::physics::updateInertia( body );
	return body;
}

pod::PhysicsBody& uf::physics::create( uf::Object& object, const pod::Plane& plane, float mass, const pod::Vector3f& offset ) {
	return create( uf::physics::getWorld(), object, plane, mass, offset );
}