#pragma once

#include <uf/config.h>
#include <uf/gl/gl.h>

#if defined(UF_USE_OPENGL)
#include <uf/utils/math/vector.h>
#include <uf/gl/vertex/vertex.h>

#include <vector>
namespace spec {
	namespace ogl {
		class UF_API Vao {
		public:
			typedef std::vector<GLuint> indexes_t;
		protected:
			GLuint 			m_index;
			Vao::indexes_t 	m_indexes;
		public:
		// 	C-tors
			Vao();
		// 	D-tors
			~Vao();
			void clear();
			void destroy();
		// 	OpenGL ops
			void generate();
			void bind();
			void bindAttribute(GLuint i);
			void bindAttribute(GLuint i, bool enable);
			void bindAttributes(bool enable);
			void render( uf::Vertices<>::base_t* vbo );
		// 	OpenGL Getters
			GLuint& getIndex();
			GLuint getIndex() const;
			bool generated() const;
		// 	Alternative to passing a base-ptr
			template<typename T>
			void render( T& vbo );
		};
		template<typename T = unsigned short>
		class UF_API Ibo {
		public:
			typedef std::vector<T> indices_t;
		protected:
			GLuint 				m_index;
			Ibo<T>::indices_t 	m_indices;
		public:
		// 	C-tors
			Ibo();
			Ibo( Ibo<T>::indices_t&& indices );
			Ibo( const Ibo<T>::indices_t& indices );
		// 	D-tors
			~Ibo();
			void clear();
			void destroy();
		// 	OpenGL ops
			void bind() const;
			void generate();
			void render( GLuint mode = GL_TRIANGLES, void* offset = NULL ) const;
		// 	OpenGL Getters
			GLuint& getIndex();
			GLuint getIndex() const;
			bool loaded() const;
			bool generated() const;
		// 	Generates indices
			void index( uf::Vertices3f& position, uf::Vertices3f& normal, uf::Vertices4f& color, uf::Vertices2f& uv );
			void index( uf::Vertices3f& position, uf::Vertices3f& normal, uf::Vertices4f& color, uf::Vertices2f& uv, uf::Vertices4i& bones_id, uf::Vertices4f& bones_weight );
		protected:
		// 	typeinfo -> GLenum
			GLuint deduceType() const;
		};
	}
}
#include "opengl.inl"
#endif