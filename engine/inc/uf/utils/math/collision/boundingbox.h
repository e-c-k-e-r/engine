#pragma once

#include "gjk.h"

namespace uf {
	class UF_API BoundingBox : public pod::Collider {
	protected:
		pod::Vector3 m_origin;
		pod::Vector3 m_corner;
	public:
		BoundingBox( const pod::Vector3& = {}, const pod::Vector3& = {} );
		
		const pod::Vector3& getOrigin() const;
		const pod::Vector3& getCorner() const;
		void setOrigin( const pod::Vector3& );
		void setCorner( const pod::Vector3& );
		
		pod::Vector3 min() const;
		pod::Vector3 max() const;
		pod::Vector3 closest( const pod::Vector3f& ) const;

		virtual std::string type() const;
		virtual pod::Vector3* expand() const;
		virtual pod::Vector3 support( const pod::Vector3& ) const;
		virtual pod::Collider::Manifold intersects( const uf::BoundingBox& ) const;
	};
}