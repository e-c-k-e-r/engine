#pragma once

#include "gjk.h"

namespace uf {
	class UF_API SphereCollider : public pod::Collider {
	protected:
		float m_radius;
		pod::Vector3 m_origin;
	public:
		SphereCollider( float = 1.0f, const pod::Vector3& = pod::Vector3{} );

		float getRadius() const;
		const pod::Vector3& getOrigin() const;
		
		void setRadius( float = 1.0f );
		void setOrigin( const pod::Vector3& );

		virtual uf::stl::string type() const;
		virtual pod::Vector3* expand() const;
		virtual pod::Vector3 support( const pod::Vector3& ) const;
		pod::Collider::Manifold intersects( const uf::SphereCollider& ) const;
	};
}