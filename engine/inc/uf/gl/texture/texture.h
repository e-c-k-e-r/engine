#pragma once

#include <uf/config.h>
#include <uf/gl/gl.h>

#if defined(UF_USE_OPENGL)
	#include "opengl.h"
#elif defined(UF_USE_DIRECTX)
	#include "directx.h"
#endif

namespace uf {
	class UF_API Material {
	protected:
		uf::Texture  m_diffuse;
		uf::Texture  m_specular;
		uf::Texture  m_normal;
	public:
		// C-tor
		Material();
		// D-tor
		~Material();
		void clear();
		void destroy();

		// OpenGL ops
		void generate();
		void bind( uint, uint, uint );
		void bind( int&, int&, int& );
		void render();
		// OpenGL Getters
		bool loaded() const;
		bool generated() const;
/*
		// Move Setters
		void setDiffuse( uf::Texture&& );
		void setSpecular( uf::Texture&& specular );
		void setNormal(uf::Texture&& normal );
		// Copy Setters
		void setDiffuse( const uf::Texture& );
		void setSpecular( const uf::Texture& specular );
		void setNormal( const uf::Texture& normal );
*/
		// Non-const Getters
		uf::Texture& getDiffuse();
		uf::Texture& getSpecular();
		uf::Texture& getNormal();
		// Const Getters
		const uf::Texture& getDiffuse() const;
		const uf::Texture& getSpecular() const;
		const uf::Texture& getNormal() const;
	};
}