#pragma once

#include <uf/config.h>

#include "./collision/gjk.h"
#include "./collision/boundingbox.h"
#include "./collision/sphere.h"
#include "./collision/mesh.h"
#include <vector>

namespace uf {
	class UF_API Collider {
	public:
		typedef std::vector<pod::Collider*> container_t;
	protected:
		uf::Collider::container_t m_container;
	public:
		~Collider();
		void clear();

		void add( pod::Collider* );
		uf::Collider::container_t& getContainer();
		const uf::Collider::container_t& getContainer() const;
		
		std::size_t getSize() const;

		std::vector<pod::Collider::Manifold> intersects( const uf::Collider&, bool = false ) const;
		std::vector<pod::Collider::Manifold> intersects( const pod::Collider&, bool = false ) const;
	};
}