#pragma once

#include <uf/config.h>
#include <uf/gl/gl.h>

#if defined(UF_USE_OPENGL) && UF_USE_OPENGL == 1
#include <vector>
#include <uf/utils/math/vector.h>

namespace spec {
	namespace ogl {	
		class UF_API Vbo {
		public:
			typedef float default_t;
		protected:
			GLuint m_index;
			GLuint m_type;
			GLuint m_attribute;
			std::size_t m_dimensions;
		public:
		// 	C-tors
			Vbo( GLuint type = GL_ARRAY_BUFFER, std::size_t dimensions = 0 );
			Vbo( Vbo&& move );
			Vbo( const Vbo& copy );
		// 	D-tors
			virtual ~Vbo();
			virtual void clear();
			virtual void destroy();
		// 	OpenGL ops
			virtual void bind() const;
			virtual void setType( GLuint type );
			virtual void bindAttribute( bool enable = true );
			virtual void bindAttribute( GLuint attribute, GLuint dimensions = 0, GLboolean normalize = GL_FALSE, GLuint stride = 0, void* offset = NULL );
			virtual void generate() = 0;
			virtual void render( GLuint mode = GL_TRIANGLES, std::size_t start = 0, std::size_t end = 0 ) const;
		// 	OpenGL Getters
			virtual GLuint& getIndex();
			virtual GLuint getIndex() const;
			virtual GLuint getType() const;
			virtual bool loaded() const = 0;
			virtual bool generated() const;
		// 	C++ Overloaded ops
			spec::ogl::Vbo& operator=( Vbo&& move );
			spec::ogl::Vbo& operator=( const Vbo& copy );
		protected:
		// 	typeinfo -> GLenum
			virtual GLuint deduceType() const = 0;
		};
		template<typename T = Vbo::default_t, std::size_t N = 3>
		class UF_API Vertices : public Vbo {
		public:
			typedef Vbo 											base_t;
			typedef T 												vertex_t;
			typedef std::vector<T> 									container_t;

			typedef uf::Vector<T, N> 								vector_t;
			typedef std::vector<typename Vertices<T,N>::vector_t> 	vectors_t;
		protected:
			typename Vertices<T,N>::container_t m_vertices;
		public:
		// 	C-tors
			Vertices( GLuint type = GL_ARRAY_BUFFER );
			Vertices( const typename Vertices<T,N>::vector_t::container_t array, std::size_t len, GLuint type = GL_ARRAY_BUFFER );
			Vertices( typename Vertices<T,N>::vectors_t&& vectors, GLuint type = GL_ARRAY_BUFFER );
			Vertices( const typename Vertices<T,N>::vectors_t& vectors, GLuint type = GL_ARRAY_BUFFER );
			Vertices( Vertices<T,N>&& move );
			Vertices( const Vertices<T,N>& copy );
		// 	D-tors
		//	~Vertices();
		// 	Setters
			void add( const typename Vertices<T,N>::vector_t::container_t array, std::size_t len );
			void add( const typename Vertices<T,N>::vectors_t& vectors );
			void add( const typename Vertices<T,N>::vector_t& vector );
			void set( typename Vertices<T,N>::vectors_t&& vectors );
			virtual void clear();
		// 	Getters
			typename Vertices<T,N>::container_t& getVertices();
			const typename Vertices<T,N>::container_t& getVertices() const;
			std::size_t verticesCount() const;
		//	typename Vertices<T,N>::vectors_t asVectors() const;
		
		// 	Abstract implementations
			virtual bool loaded() const;
			virtual void generate();
			virtual void render( GLuint mode = GL_TRIANGLES, std::size_t start = 0, std::size_t end = 0 ) const;
		// 	C++ Overloaded ops
			spec::ogl::Vertices<T,N>& operator=( Vertices<T,N>&& move );
			spec::ogl::Vertices<T,N>& operator=( const Vertices<T,N>& copy );
		protected:
			virtual GLuint deduceType() const;
		};
	}
}

#include "opengl.inl"

namespace uf {
	template<typename T = float, std::size_t N = 3>
	using Vertices = spec::ogl::Vertices<T,N>;
}
#endif