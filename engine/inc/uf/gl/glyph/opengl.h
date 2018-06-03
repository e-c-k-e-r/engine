#pragma once

#include <uf/config.h>
#include <uf/gl/gl.h>

#if defined(UF_USE_OPENGL) && UF_USE_OPENGL == 1
#include <vector>

#include <uf/utils/math/vector.h>
#include <uf/ext/freetype/freetype.h>
#include <uf/gl/texture/texture.h>
#include <uf/gl/mesh/mesh.h>

namespace spec {
	namespace ogl {	
		class UF_API GlyphTexture : public spec::ogl::Texture {
		protected:
			pod::Vector2ui m_size = { 0, 0 };
			pod::Vector2ui m_bearing = { 0, 0 };
			pod::Vector2i m_advance = { 0, 0 };
			
			pod::Vector2ui m_padding = { 0, 0 };
			
			bool m_sdf = false;
			int m_spread = 0;
		public:
		// 	OpenGL ops
		//	void bind( uint ) const;
			void generate( const std::string&, unsigned long, uint = 48 );
			void generate( ext::freetype::Glyph&, unsigned long, uint = 48 );
			void generate( const std::string&, const uf::String&, uint = 48 );
			void generate( ext::freetype::Glyph&, const uf::String&, uint = 48 );

			void generateSdf( uint8_t* );
		// 	Get
			const pod::Vector2ui& getSize() const;
			const pod::Vector2ui& getBearing() const;
			const pod::Vector2i& getAdvance() const;
			const pod::Vector2ui& getPadding() const;
			
			bool isSdf() const;
			int getSpread() const;
		//	Set
			void setSize( const pod::Vector2ui& );
			void setBearing( const pod::Vector2ui& );
			void setAdvance( const pod::Vector2i& );
			void setPadding( const pod::Vector2ui& );

			void useSdf( bool = true );
			void setSpread( int );
		};
		class UF_API GlyphMesh {
		protected:
			spec::ogl::Vao 	m_vao;
			uf::Vertices4f 	m_vbo;
		public:
			// OpenGL ops
			void generate();
			void bindAttributes();
			void render();
			// OpenGL Getters
			bool loaded() const;
			bool generated() const;
			// Move Setters
			void setPoints(uf::Vertices4f&& points );
			// Copy Setters
			void setPoints( const uf::Vertices4f& points );
			// Non-const Getters
			uf::Vertices4f& getPoints();
			// Const Getters
			const uf::Vertices4f& getPoints() const;
		};
	}
}

namespace uf {
	typedef spec::ogl::GlyphTexture GlyphTexture;
	typedef spec::ogl::GlyphMesh GlyphMesh;
}
#endif