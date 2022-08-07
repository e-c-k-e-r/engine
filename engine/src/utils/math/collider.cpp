#if UF_USE_UNUSED
#include <uf/utils/math/collision.h>
#include <uf/utils/math/collision/gjk.h>
#include <uf/utils/math/collision/boundingbox.h>
#include <uf/utils/math/collision/sphere.h>
#include <uf/utils/math/collision/mesh.h>
#include <uf/utils/math/collision/modular.h>
#include <iostream>

uf::Collider::~Collider() {
	this->clear();
}
void uf::Collider::clear() {
	for ( pod::Collider* pointer : this->m_container ) delete pointer;
	this->m_container.clear();
}

void uf::Collider::add( pod::Collider* pointer ) {
	this->m_container.push_back(pointer);
}
uf::Collider::container_t& uf::Collider::getContainer() {
	return this->m_container;
}
const uf::Collider::container_t& uf::Collider::getContainer() const {
	return this->m_container;
}

std::size_t uf::Collider::getSize() const {
	return this->m_container.size();
}

uf::stl::vector<pod::Collider::Manifold> uf::Collider::intersects( const uf::Collider& body, bool smart ) const {
	uf::stl::vector<pod::Collider::Manifold> manifolds;
	manifolds.reserve( this->m_container.size() * body.m_container.size() );
	for ( const pod::Collider* pointer : body.m_container ) {
		uf::stl::vector<pod::Collider::Manifold> result = this->intersects( *pointer, smart );
		manifolds.insert( manifolds.end(), result.begin(), result.end() );
	}
	return manifolds;
}

uf::stl::vector<pod::Collider::Manifold> uf::Collider::intersects( const pod::Collider& body, bool smart ) const {
	// smart = false;
	uf::stl::vector<pod::Collider::Manifold> manifolds;
	for ( const pod::Collider* pointer : this->m_container ) {
		pod::Collider::Manifold& manifold = manifolds.emplace_back();
		if ( !pointer ) continue;
		if ( smart && pointer->type() == body.type() ) {
			if ( pointer->type() == "BoundingBox" ) {
				const uf::BoundingBox& a = *((const uf::BoundingBox*) pointer);
				const uf::BoundingBox& b = *((const uf::BoundingBox*) &body);
				manifold = a.intersects(b);
			} else if ( pointer->type() == "Sphere" ) {
				const uf::SphereCollider& a = *((const uf::SphereCollider*) pointer);
				const uf::SphereCollider& b = *((const uf::SphereCollider*) &body);
				manifold = a.intersects(b);
			}
			else
				manifold = pointer->intersects( body );
		} else manifold = pointer->intersects( body );
	}
	return manifolds;
}
#endif