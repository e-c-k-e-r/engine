#pragma once

#include <uf/config.h>
#if UF_USE_UNUSED
#include "./collision/gjk.h"
#include "./collision/boundingbox.h"
#include "./collision/sphere.h"
#include "./collision/mesh.h"
#include <uf/utils/memory/vector.h>

namespace uf {
	class UF_API Collider {
	public:
		typedef uf::stl::vector<pod::Collider*> container_t;
	protected:
		uf::Collider::container_t m_container;
	public:
		~Collider();
		void clear();

		void add( pod::Collider* );
		uf::Collider::container_t& getContainer();
		const uf::Collider::container_t& getContainer() const;
		
		std::size_t getSize() const;

		uf::stl::vector<pod::Collider::Manifold> intersects( const uf::Collider&, bool = false ) const;
		uf::stl::vector<pod::Collider::Manifold> intersects( const pod::Collider&, bool = false ) const;
	};
}
#endif