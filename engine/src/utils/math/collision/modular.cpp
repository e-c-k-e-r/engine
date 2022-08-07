#if UF_USE_UNUSED
#include <uf/utils/math/collision/modular.h>

uf::ModularCollider::ModularCollider( uint len, pod::Vector3* container, bool should_free, const uf::ModularCollider::function_expand_t& expand, const uf::ModularCollider::function_support_t& support  ) {
	this->m_len = len;
	this->m_container = container;
	this->m_should_free = should_free;
	this->m_function_expand = expand;
	this->m_function_support = support;
}
uf::ModularCollider::~ModularCollider() {
	if ( this->m_container != NULL && this->m_should_free ) delete[] this->m_container;
}
uf::stl::string uf::ModularCollider::type() const { return "Modular"; }
void uf::ModularCollider::setExpand( const uf::ModularCollider::function_expand_t& expand ) {
	this->m_function_expand = expand;
}
void uf::ModularCollider::setSupport( const uf::ModularCollider::function_support_t& support ) {
	this->m_function_support = support;
}

pod::Vector3* uf::ModularCollider::getContainer() {
	return this->m_container;
}
uint uf::ModularCollider::getSize() const {
	return this->m_len;
}
void uf::ModularCollider::setContainer( pod::Vector3* container, uint len ) {
	this->m_container = container;
	this->m_len = len;
}

pod::Vector3* uf::ModularCollider::expand() const {
	return this->m_function_expand ? this->m_function_expand() : this->m_container;
}
pod::Vector3 uf::ModularCollider::support( const pod::Vector3& direction ) const {
	if ( this->m_function_support ) return this->m_function_support(direction);
	
	pod::Vector3* points = this->expand();
	uint len = this->m_len;
	
	struct {
		size_t i = 0;
		float dot = 0;
	} best;

	for ( size_t i = 0; i < len; ++i ) {
	//	float dot = uf::vector::dot( uf::matrix::multiply<float>( model, points[i] ), direction);
		float dot = uf::vector::dot( points[i], direction);
		if ( i == 0 || dot > best.dot ) {
			best.i = i;
			best.dot = dot;
		}
	}
	return points[best.i];
}
#endif