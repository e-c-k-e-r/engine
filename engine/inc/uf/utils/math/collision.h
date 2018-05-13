#pragma once

#include <uf/config.h>

#include <uf/utils/math/vector.h>
#include <functional>

// #include <uf/gl/mesh/mesh.h>
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
			float dot( const pod::Vector3& r ) const {
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
	/*	pod::Vector3 b, c, d;
		Simplex( const pod::Vector3& = pod::Vector3(), const pod::Vector3& = pod::Vector3(), const pod::Vector3& = pod::Vector3() );
		void add( const pod::Vector3& );
		void set( const pod::Vector3& = pod::Vector3(), const pod::Vector3& = pod::Vector3(), const pod::Vector3& = pod::Vector3() );
	*/
	};
}
namespace uf {
	class UF_API Collider {
	protected:
	public:
		struct UF_API Manifold {
			const uf::Collider* a;
			const uf::Collider* b;
			pod::Vector3 normal;
			float depth;
			bool colliding;

			Manifold() {
				this->normal = pod::Vector3{0.0f, 0.0f, 0.0f};
				this->depth = -0.1f;
				this->colliding = false;
			}
			Manifold( const uf::Collider& a, const uf::Collider& b ) {
				this->a = &a;
				this->b = &b;
				this->normal = pod::Vector3{0.0f, 0.0f, 0.0f};
				this->depth = -0.1f;
				this->colliding = false;
			}
			Manifold( const uf::Collider& a, const uf::Collider& b, const pod::Vector3& normal, float depth, bool colliding ) {
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
		virtual uf::Collider::Manifold intersects( const uf::Collider& ) const;
	};

	class UF_API AABBox : public uf::Collider {
	protected:
		pod::Vector3 m_origin;
		pod::Vector3 m_corner;
	public:
		AABBox( const pod::Vector3&, const pod::Vector3& );
		virtual std::string type() const;
		virtual pod::Vector3* expand() const;
		virtual pod::Vector3 support( const pod::Vector3& ) const;
		virtual uf::Collider::Manifold intersects( const uf::AABBox& ) const;
	};
	class UF_API SphereCollider : public uf::Collider {
	protected:
		float m_radius;
		pod::Vector3 m_origin;
	public:
		SphereCollider( float = 1.0f, const pod::Vector3& = pod::Vector3{} );

		float getRadius() const;
		const pod::Vector3& getOrigin() const;
		
		void setRadius( float = 1.0f );
		void setOrigin( const pod::Vector3& );

		virtual std::string type() const;
		virtual pod::Vector3* expand() const;
		virtual pod::Vector3 support( const pod::Vector3& ) const;
		virtual uf::Collider::Manifold intersects( const uf::SphereCollider& ) const;
	};
/*
	class UF_API MeshCollider : public uf::Collider {
	protected:
		uf::Mesh& mesh;
		uf::Mesh::vertices_t::vector_t raw;
	public:
		MeshCollider( uf::Mesh& );

		uf::Mesh& getMesh();
		const uf::Mesh& getMesh() const;
		void setMesh( const uf::Mesh& );

		virtual std::string type() const;
		virtual pod::Vector3* expand() const;
		virtual pod::Vector3 support( const pod::Vector3& ) const;
	};
*/
	class UF_API ModularCollider : public uf::Collider {
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

		virtual std::string type() const;
		virtual pod::Vector3* expand() const;
		virtual pod::Vector3 support( const pod::Vector3& ) const;
	};
	class UF_API CollisionBody {
	public:
		typedef std::vector<uf::Collider*> container_t;
	protected:
		uf::CollisionBody::container_t m_container;
	public:
		~CollisionBody();
		void clear();

		void add( uf::Collider* );
		uf::CollisionBody::container_t& getContainer();
		const uf::CollisionBody::container_t& getContainer() const;
		
		std::size_t getSize() const;

		std::vector<uf::Collider::Manifold> intersects( const uf::CollisionBody& ) const;
		std::vector<uf::Collider::Manifold> intersects( const uf::Collider& ) const;
	};
}