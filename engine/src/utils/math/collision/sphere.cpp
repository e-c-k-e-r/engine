#include <uf/utils/math/collision/sphere.h>

UF_API uf::SphereCollider::SphereCollider( float r, const pod::Vector3& origin ) {
	this->m_radius = r;
	this->m_origin = origin;
}
uf::stl::string UF_API uf::SphereCollider::type() const { return "Sphere"; }
float UF_API uf::SphereCollider::getRadius() const {
	return this->m_radius;
}
const pod::Vector3& UF_API uf::SphereCollider::getOrigin() const {
	return this->m_origin;
}

void UF_API uf::SphereCollider::setRadius( float r ) {
	this->m_radius = r;
}
void UF_API uf::SphereCollider::setOrigin( const pod::Vector3& origin ) {
	this->m_origin = origin;
}

pod::Vector3* UF_API uf::SphereCollider::expand() const {
	return NULL;
}
pod::Vector3 UF_API uf::SphereCollider::support( const pod::Vector3& direction ) const {
	pod::Vector3f position = this->getPosition() + this->m_origin;
//	const pod::Vector3f& position = this->m_origin;
	return position + direction * (this->m_radius/uf::vector::magnitude(direction));
}
pod::Collider::Manifold uf::SphereCollider::intersects( const uf::SphereCollider& b ) const {
	const uf::SphereCollider& a = *this;
	pod::Collider::Manifold manifold(a, b);

	pod::Vector3f position_a = a.getPosition() + a.m_origin;
	pod::Vector3f position_b = b.getPosition() + b.m_origin;
	// const pod::Vector3f& position_a = a.m_origin;
	// const pod::Vector3f& position_b = b.m_origin;

	float distanceSquared = uf::vector::distanceSquared(position_a, position_b);
	float sum = a.m_radius + b.m_radius;
	float sumSquared = sum * sum;

	if ( distanceSquared >= sumSquared ) return manifold;

	manifold.depth = fabs(b.m_radius - a.m_radius);
	manifold.normal = position_b - position_a;
	manifold.colliding = true;
	return manifold;
}