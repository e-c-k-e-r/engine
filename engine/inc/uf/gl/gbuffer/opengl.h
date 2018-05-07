#pragma once

#include <uf/config.h>
#include <uf/gl/gl.h>

#if defined(UF_USE_OPENGL) && UF_USE_OPENGL == 1
#include <vector>

#include <uf/utils/math/vector.h>
#include <uf/utils/image/image.h>
#include <uf/gl/mesh/mesh.h>
#include <uf/utils/component/component.h>

namespace spec {
	namespace ogl {	
		class UF_API GeometryTexture : public spec::ogl::Texture {
		public:
			struct type_t {
				uint attachment;
				uint type;
				uint format;
				uint internal;
			};
		protected:
			std::string m_name;
			pod::Vector2ui m_size;
			spec::ogl::GeometryTexture::type_t m_type;
		public:
		// 	C-tors
			GeometryTexture();
			GeometryTexture( GeometryTexture&& move ); 					// Move texture
			GeometryTexture( const GeometryTexture& copy ); 			// Copy texture
		// 	OpenGL ops
			void bind( uint ) const;
			void generate();
		// 	Get
			const pod::Vector2ui& getSize() const;
			spec::ogl::GeometryTexture::type_t getType() const;
			const std::string& getName() const;
		//	Set
			void setSize( const pod::Vector2ui& );
			void setType( spec::ogl::GeometryTexture::type_t );
			void setName( const std::string& );
		};
		class UF_API GeometryBuffer : public uf::Component {
		public:
			typedef uint type_t;
			typedef std::vector<spec::ogl::GeometryTexture> container_t;
		protected:
			spec::ogl::GeometryBuffer::type_t m_index;
			pod::Vector2ui m_size;

			spec::ogl::GeometryBuffer::container_t m_buffers;
		public:
			GeometryBuffer();
			GeometryBuffer( GeometryBuffer&& move );
			GeometryBuffer( const GeometryBuffer& copy );

			~GeometryBuffer();

			void generate( bool = false );
			void bind() const;
			void unbind() const;
			void destroy();

			void copy() const;
			void render();

			spec::ogl::GeometryBuffer::type_t& getIndex();
			spec::ogl::GeometryBuffer::type_t getIndex() const;
			
			pod::Vector2ui& getSize();
			const pod::Vector2ui& getSize() const;

			void setSize( const pod::Vector2ui& );

			spec::ogl::GeometryBuffer::container_t& getBuffers();
		};
	}
}

namespace uf {
	typedef spec::ogl::GeometryTexture GeometryTexture;
	typedef spec::ogl::GeometryBuffer GeometryBuffer;
}
#endif