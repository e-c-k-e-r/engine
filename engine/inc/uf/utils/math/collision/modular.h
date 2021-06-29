#pragma once

#include "gjk.h"
#include <functional>

namespace uf {
	class UF_API ModularCollider : public pod::Collider {
	public:
		typedef std::function<pod::Vector3*()> function_expand_t;
		typedef std::function<pod::Vector3(const pod::Vector3&)> function_support_t;
	protected:
		uint m_len;
		pod::Vector3* m_container;

		bool m_should_free;

		uf::ModularCollider::function_expand_t m_function_expand;
		uf::ModularCollider::function_support_t m_function_support;
	public:
		ModularCollider( uint len = 0, pod::Vector3* = NULL, bool = false, const uf::ModularCollider::function_expand_t& = NULL, const uf::ModularCollider::function_support_t& = NULL );
		~ModularCollider();
		
		void setExpand( const uf::ModularCollider::function_expand_t& = NULL );
		void setSupport( const uf::ModularCollider::function_support_t& = NULL );
		
		pod::Vector3* getContainer();
		uint getSize() const;
		void setContainer( pod::Vector3*, uint );

		virtual uf::stl::string type() const;
		virtual pod::Vector3* expand() const;
		virtual pod::Vector3 support( const pod::Vector3& ) const;
	};
}