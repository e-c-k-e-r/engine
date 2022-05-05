#if UF_USE_UNUSED
#include <uf/utils/math/collision/boundingbox.h>

UF_API uf::BoundingBox::BoundingBox( const pod::Vector3& origin, const pod::Vector3& corner ) {
	this->m_origin = origin;
	this->m_corner = corner;
}
const pod::Vector3& uf::BoundingBox::getOrigin() const {
	return this->m_origin;
}
const pod::Vector3& uf::BoundingBox::getCorner() const {
	return this->m_corner;
}
void uf::BoundingBox::setOrigin( const pod::Vector3& origin ) {
	this->m_origin = origin;
}
void uf::BoundingBox::setCorner( const pod::Vector3& corner ) {
	this->m_corner = corner;
}
pod::Vector3 uf::BoundingBox::min() const {
	pod::Vector3f position = this->getPosition() + this->m_origin;
	// const pod::Vector3f& position = this->m_origin;
	return position - this->m_corner;
}
pod::Vector3 uf::BoundingBox::max() const {
	pod::Vector3f position = this->getPosition() + this->m_origin;
	// const pod::Vector3f& position = this->m_origin;
	return position + this->m_corner;
}
pod::Vector3 uf::BoundingBox::closest( const pod::Vector3f& point ) const {
	pod::Vector3f position = this->getPosition() + this->m_origin;
	// const pod::Vector3f& position = this->m_origin;

	float distance = 9E9;
	pod::Vector3f vector = point;
	pod::Vector3f min = position - this->m_corner;
	pod::Vector3f max = position + this->m_corner;

	float test = 0;
	if ( (test = fabs(min.x - point.x)) < distance ) {
		distance = test;
		vector.x = min.x;
	}
	if ( (test = fabs(max.x - point.x)) < distance ) {
		distance = test;
		vector.x = max.x;
	}

	if ( (test = fabs(min.y - point.y)) < distance ) {
		distance = test;
		vector.y = min.y;
	}
	if ( (test = fabs(max.y - point.y)) < distance ) {
		distance = test;
		vector.y = max.y;
	}

	if ( (test = fabs(min.z - point.z)) < distance ) {
		distance = test;
		vector.z = min.z;
	}
	if ( (test = fabs(max.z - point.z)) < distance ) {
		distance = test;
		vector.z = max.z;
	}

	return vector;
}
uf::stl::string UF_API uf::BoundingBox::type() const { return "BoundingBox"; }
pod::Vector3* UF_API uf::BoundingBox::expand() const {
	pod::Vector3* raw = new pod::Vector3[8];
	raw[0] = pod::Vector3{  this->m_corner.x,  this->m_corner.y,  this->m_corner.z};
	raw[1] = pod::Vector3{  this->m_corner.x,  this->m_corner.y, -this->m_corner.z};
	raw[2] = pod::Vector3{ -this->m_corner.x,  this->m_corner.y, -this->m_corner.z};
	raw[3] = pod::Vector3{ -this->m_corner.x,  this->m_corner.y,  this->m_corner.z};
	raw[4] = pod::Vector3{ -this->m_corner.x, -this->m_corner.y,  this->m_corner.z};
	raw[5] = pod::Vector3{  this->m_corner.x, -this->m_corner.y,  this->m_corner.z};
	raw[6] = pod::Vector3{  this->m_corner.x, -this->m_corner.y, -this->m_corner.z};
	raw[7] = pod::Vector3{ -this->m_corner.x, -this->m_corner.y,  this->m_corner.z};

	pod::Vector3f position = this->getPosition() + this->m_origin;
	// const pod::Vector3f& position = this->m_origin;
	for ( uint i = 0; i < 8; i++ ) raw[0] += position;

	return raw;
}
pod::Vector3 UF_API uf::BoundingBox::support( const pod::Vector3& direction ) const {
	pod::Vector3f position = this->getPosition() + this->m_origin;
	// const pod::Vector3f& position = this->m_origin;
	pod::Vector3 res = {
		position.x + fabs(this->m_corner.x) * (direction.x > 0 ? 1 : -1),
		position.y + fabs(this->m_corner.y) * (direction.y > 0 ? 1 : -1),
		position.z + fabs(this->m_corner.z) * (direction.z > 0 ? 1 : -1),
	};
	return res;
}

pod::Collider::Manifold UF_API uf::BoundingBox::intersects( const uf::BoundingBox& b ) const {
	const uf::BoundingBox& a = *this;
	pod::Collider::Manifold manifold(a, b);

	BoundingBox difference(a.min() - b.max() + a.m_corner + b.m_corner, (a.m_corner + b.m_corner) * 2.0f);

	pod::Vector3f min = difference.m_origin - this->m_corner;
	pod::Vector3f max = difference.m_origin + this->m_corner;
	manifold.colliding = min.x <= 0 && max.x >= 0 && min.y <= 0 && max.y >= 0;

	if ( !manifold.colliding ) return manifold;

	pod::Vector3f penetration = difference.closest( {0, 0, 0} );
	manifold.depth = uf::vector::norm( penetration );
	manifold.normal = penetration / manifold.depth;

/*
	float a_left 	= position_a.x - a.m_corner.x;
	float a_right 	= position_a.x + a.m_corner.x;
	float a_bottom 	= position_a.y - a.m_corner.y;
	float a_top 	= position_a.y + a.m_corner.y;
	float a_back 	= position_a.z - a.m_corner.z;
	float a_front 	= position_a.z + a.m_corner.z;


	float b_left 	= position_b.x - b.m_corner.x;
	float b_right 	= position_b.x + b.m_corner.x;
	float b_bottom 	= position_b.y - b.m_corner.y;
	float b_top 	= position_b.y + b.m_corner.y;
	float b_back 	= position_b.z - b.m_corner.z;
	float b_front 	= position_b.z + b.m_corner.z;

	manifold.depth = 9E9;
	auto test = [&]( const pod::Vector3& axis, float minA, float maxA, float minB, float maxB )->bool{
		float axisLSqr = uf::vector::dot(axis, axis);
		if ( axisLSqr < 1E-8f ) return true;

		float d0 = maxB - minA;
		float d1 = maxA - minB;
		if ( d0 < 0 || d1 < 0 ) return false;
		float overlap = d0 < d1 ? d0 : -d1;
		pod::Vector3 sep = axis * ( overlap / axisLSqr );
		float sepLSqr = uf::vector::dot( sep, sep );
		if ( sepLSqr < manifold.depth ) {
			manifold.normal = sep;
			manifold.depth = sepLSqr;
		}
		return true;
	};

	if ( !test( {1, 0, 0}, a_left, a_right, b_left, b_right ) ) return manifold;
	if ( !test( {0, 1, 0}, a_bottom, a_top, b_bottom, b_top ) ) return manifold;
	if ( !test( {0, 0, 1}, a_back, a_front, b_back, b_front ) ) return manifold;

	manifold.normal = uf::vector::normalize( manifold.normal );
	manifold.depth = sqrt( manifold.depth );
//	manifold.depth = manifold.depth;
	manifold.colliding = true;
*/
	return manifold;
}
#endif