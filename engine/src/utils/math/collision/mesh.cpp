#include <uf/utils/math/collision/mesh.h>

uf::MeshCollider::MeshCollider( const pod::Transform<>& transform, const std::vector<pod::Vector3>& positions ) : m_positions(positions) {
	this->setTransform(transform);
}
std::string UF_API uf::MeshCollider::type() const { return "Mesh"; }

std::vector<pod::Vector3>& uf::MeshCollider::getPositions() {
	return this->m_positions;
}

const std::vector<pod::Vector3>& uf::MeshCollider::getPositions() const {
	return this->m_positions;
}

void uf::MeshCollider::setPositions( const std::vector<pod::Vector3>& positions ) {
	this->m_positions = positions;
}

pod::Vector3* uf::MeshCollider::expand() const {
	return (pod::Vector3*) &this->m_positions[0];
}
pod::Vector3 uf::MeshCollider::support( const pod::Vector3& direction ) const {
	size_t len = this->m_positions.size();
	pod::Vector3* points = this->expand();
	pod::Matrix4f model = uf::transform::model( this->m_transform );

	struct {
		size_t i = 0;
		float dot = 0;
	} best;

	for ( size_t i = 0; i < len; ++i ) {
		float dot = uf::vector::dot( uf::matrix::multiply<float>( model, points[i] ), direction);
	//	float dot = uf::vector::dot( points[i], direction);
		if ( i == 0 || dot > best.dot ) {
			best.i = i;
			best.dot = dot;
		}
	}
	return points[best.i];
}