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
void UF_API uf::Collider::clear() {
	for ( pod::Collider* pointer : this->m_container ) delete pointer;
	this->m_container.clear();
}

void UF_API uf::Collider::add( pod::Collider* pointer ) {
	this->m_container.push_back(pointer);
}
uf::Collider::container_t& UF_API uf::Collider::getContainer() {
	return this->m_container;
}
const uf::Collider::container_t& UF_API uf::Collider::getContainer() const {
	return this->m_container;
}

std::size_t UF_API uf::Collider::getSize() const {
	return this->m_container.size();
}

std::vector<pod::Collider::Manifold> UF_API uf::Collider::intersects( const uf::Collider& body, bool smart ) const {
	std::vector<pod::Collider::Manifold> manifolds;
	manifolds.reserve( this->m_container.size() * body.m_container.size() );
	for ( const pod::Collider* pointer : body.m_container ) {
		std::vector<pod::Collider::Manifold> result = this->intersects( *pointer, smart );
		manifolds.insert( manifolds.end(), result.begin(), result.end() );
	}
	return manifolds;
}

std::vector<pod::Collider::Manifold> UF_API uf::Collider::intersects( const pod::Collider& body, bool smart ) const {
	// smart = false;
	std::vector<pod::Collider::Manifold> manifolds;
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