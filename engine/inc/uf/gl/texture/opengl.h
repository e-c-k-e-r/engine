#pragma once

#include <uf/config.h>
#include <uf/gl/gl.h>

#if defined(UF_USE_OPENGL) && UF_USE_OPENGL == 1
#include <vector>

#include <uf/utils/math/vector.h>
#include <uf/utils/image/image.h>

namespace spec {
	namespace ogl {	
		class UF_API Texture {
		public:
			typedef uf::Image image_t;
		protected:
			GLuint m_index;
			Texture::image_t m_image;
		public:
		// 	C-tors
			Texture();
			Texture( Texture&& move ); 					// Move texture
			Texture( const Texture& copy ); 			// Copy texture
			Texture( Texture::image_t&& move ); 		// Move image
			Texture( const Texture::image_t& copy ); 	// Copy image

			void open( const std::string& filename ); 	// Open image from filename
		// 	D-tor
			~Texture();
			void clear();
			void destroy();
		// 	OpenGL ops
			void bind( uint slot = 0 ) const;
			void generate();
		// 	OpenGL Getters
			GLuint& getIndex();
			GLuint getIndex() const;
			GLuint getType() const;
			bool loaded() const;
			bool generated() const;
			Texture::image_t& getImage();
			const Texture::image_t& getImage() const;
		};
	}
}

namespace uf {
	typedef spec::ogl::Texture Texture;
}
#endif