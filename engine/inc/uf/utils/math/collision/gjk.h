#pragma once

#include <uf/config.h>

#include <uf/utils/math/vector.h>
#include <uf/utils/math/transform.h>

namespace pod {
	struct UF_API Simplex {
	public:
		struct UF_API SupportPoint {
			pod::Vector3 a, b, v;
			bool operator==( const pod::Simplex::SupportPoint& r ) const {
				return v == r.v;
			}
			pod::Vector3 operator-( const pod::Simplex::SupportPoint& r ) const {
				return v - r.v;
			}
			pod::Vector3 operator*( float r ) const {
				return v * r;
			}
			pod::Vector3 operator-( const pod::Vector3& r ) const {
				return v - r;
			}
			pod::Math::num_t dot( const pod::Vector3& r ) const {
				return uf::vector::dot(v, r);
			}
			pod::Vector3 cross( const pod::Simplex::SupportPoint& r ) const {
				return uf::vector::cross(v, r.v);
			}
			pod::Vector3 cross( const pod::Vector3& r ) const {
				return uf::vector::cross(v, r);
			}
		};
		int size = 0;
		pod::Simplex::SupportPoint b, c, d;
	/*	
		pod::Vector3 b, c, d;
		Simplex( const pod::Vector3& = pod::Vector3(), const pod::Vector3& = pod::Vector3(), const pod::Vector3& = pod::Vector3() );
		void add( const pod::Vector3& );
		void set( const pod::Vector3& = pod::Vector3(), const pod::Vector3& = pod::Vector3(), const pod::Vector3& = pod::Vector3() );
	*/
	};
}
namespace pod {
	class UF_API Collider {
	protected:
		pod::Transform<> m_transform;
	public:
		struct UF_API Manifold {
			const pod::Collider* a;
			const pod::Collider* b;
			pod::Vector3 normal;
			float depth;
			bool colliding;

			Manifold() {
				this->normal = pod::Vector3{0.0f, 0.0f, 0.0f};
				this->depth = -0.1f;
				this->colliding = false;
			}
			Manifold( const pod::Collider& a, const pod::Collider& b ) {
				this->a = &a;
				this->b = &b;
				this->normal = pod::Vector3{0.0f, 0.0f, 0.0f};
				this->depth = -0.1f;
				this->colliding = false;
			}
			Manifold( const pod::Collider& a, const pod::Collider& b, const pod::Vector3& normal, float depth, bool colliding ) {
				this->a = &a;
				this->b = &b;
				this->normal = normal;
				this->depth = depth;
				this->colliding = colliding;
			}
		};
		virtual ~Collider();
		virtual std::string type() const;
		virtual pod::Vector3* expand() const = 0;
		virtual pod::Vector3 support( const pod::Vector3& ) const = 0;
		pod::Collider::Manifold intersects( const pod::Collider& ) const;

		pod::Vector3f getPosition() const;
		pod::Transform<>& getTransform();
		const pod::Transform<>& getTransform() const;
		void setTransform( const pod::Transform<>& );
	};
}