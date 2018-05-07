#pragma once

#include <uf/config.h>
#include <uf/gl/gl.h>

#include <uf/gl/vertex/vertex.h>

#if defined(UF_USE_OPENGL)
	#include "opengl.h"
#elif defined(UF_USE_DIRECTX)
	#include "directx.h"
#endif
#include <uf/utils/math/matrix.h>

#include <vector>
#include <memory>
namespace uf {
	class UF_API Mesh {
	public:
		typedef spec::ogl::Vao vao_t;
		typedef spec::ogl::Ibo<GLushort> ibo_t;
	protected:
		Mesh::vao_t 	m_vao;
		Mesh::ibo_t 	m_ibo;
		bool 			m_indexed;

		uf::Vertices3f 	m_position;
		uf::Vertices3f 	m_normal;
		uf::Vertices4f 	m_color;
		uf::Vertices2f 	m_uv;
	public:
		// C-tor
		Mesh();
		// D-tor
		~Mesh();
		void clear();
		void destroy();

		// OpenGL ops
		void generate();
		void bindAttributes();
		void render();
		// OpenGL Getters
		bool loaded() const;
		bool generated() const;

		// Indexed ops
		void index();

		// Move Setters
		void setPositions( uf::Vertices3f&& position );
		void setNormals( uf::Vertices3f&& normal );
		void setColors(uf::Vertices4f&& color );
		void setUvs( uf::Vertices2f&& uf );
		// Copy Setters
		void setPositions( const uf::Vertices3f& position );
		void setNormals( const uf::Vertices3f& normal );
		void setColors( const uf::Vertices4f& color );
		void setUvs( const uf::Vertices2f& uv );

		// Non-const Getters
		uf::Vertices3f& getPositions();
		uf::Vertices3f& getNormals();
		uf::Vertices4f& getColors();
		uf::Vertices2f& getUvs();
		// Const Getters
		const uf::Vertices3f& getPositions() const;
		const uf::Vertices3f& getNormals() const;
		const uf::Vertices4f& getColors() const;
		const uf::Vertices2f& getUvs() const;
	};
}