#pragma once

#include <uf/config.h>
#include <uf/gl/gl.h>

#if defined(UF_USE_OPENGL) && UF_USE_OPENGL == 1
#include <string>
#include <vector>
#include <map>

#include <uf/utils/math/vector.h>
#include <uf/utils/math/matrix.h>

namespace uf {
	namespace ogl {
		class UF_API Shader {
		public:
			typedef GLuint type_t;
			typedef GLuint index_t;

			class UF_API Component {
			public:
				typedef std::string source_t;
				typedef std::vector<Shader::Component> container_t;
			protected:
				Shader::type_t m_type;
				Shader::index_t m_index;
				Component::source_t m_source;
			public:
				// C-tors
				Component( Shader::type_t type = 0 );
				Component( const Component::source_t& source, Shader::type_t type, bool isFilename = true );
				// D-tor
				~Component();

				// Read from file
				bool read( const std::string& file );
				// Import from source
				void import( const std::string& source );
				// Compile shader unit
				bool compile();
				// Checks if valid
				bool compiled() const;
				// Releases resources
				void destroy();
				// Error checking
				std::string log() const;
				// Gets index
				Shader::index_t getIndex() const;
			};

			struct Uniform {
				typedef int 			index_t;
				typedef std::string 	name_t;
				typedef std::map<Uniform::name_t, Uniform::index_t> container_t;
			};
			struct Attribute {
				typedef int 			index_t;
				typedef std::string 	name_t;
			};
		protected:
			Shader::index_t 		m_index;
			Uniform::container_t 	m_uniforms;
			Component::container_t 	m_components;
		public:
			// C-tors
			Shader();
			Shader( const Component::container_t& components );
			// D-tor
			~Shader();

			// Attaches shader units
			void add( const Shader::Component& component );
			void add( const Component::container_t& components );
			// Compiles all shader units (Calls this->link() if link==true)
			void compile( bool link = false );
			// Checks if valid
			bool compiled() const;
			// Links shader units
			void link();
			// Destroys all shader units
			void flush();
			// Destroy this shader
			void destroy();
			// Populates uniform list
			void populate();
			// Error checking
			std::string log() const;
			// Swap shader
			void swap( Shader& shader );
			// Bind's shader
			void bind() const;
			// Gets index
			Shader::index_t getIndex() const;

			// Shader input/ouptuts
			Uniform::index_t getUniform( const Uniform::name_t& name ) const;
			void bindAttribute( Attribute::index_t index, const Attribute::name_t& name  ) const;
			void bindFragmentData( Attribute::index_t index, const Attribute::name_t& name  ) const;
			// Uniforms
			// 	Legacy glUniform calls
			void push( Attribute::index_t index, int x ) const;
			void push( Attribute::index_t index, uint x ) const;
			void push( Attribute::index_t index, float x ) const;
			void push( Attribute::index_t index, float x, float y ) const;
			void push( Attribute::index_t index, float x, float y, float z ) const;
			void push( Attribute::index_t index, float x, float y, float z, float w ) const;
			void push( Attribute::index_t index, bool transpose, float* matrix ) const;

			void push( Attribute::index_t index, const uf::Vector2& vec2 ) const;
			void push( Attribute::index_t index, const uf::Vector3& vec3 ) const;
			void push( Attribute::index_t index, const uf::Vector4& vec4 ) const;
			void push( Attribute::index_t index, bool transpose, const uf::Matrix4& mat ) const;
		// 	"optimized" glUniform calls
			void push( Attribute::name_t name, int x ) const;
			void push( Attribute::name_t name, uint x ) const;
			void push( Attribute::name_t name, float x ) const;
			void push( Attribute::name_t name, float x, float y ) const;
			void push( Attribute::name_t name, float x, float y, float z ) const;
			void push( Attribute::name_t name, float x, float y, float z, float w ) const;
			void push( Attribute::name_t name, float* mat, bool transpose = false ) const;

			void push( Attribute::name_t name, const uf::Vector2& vec2 ) const;
			void push( Attribute::name_t name, const uf::Vector3& vec3 ) const;
			void push( Attribute::name_t name, const uf::Vector4& vec4 ) const;
			void push( Attribute::name_t name, const uf::Matrix4& mat, bool transpose = false ) const;
		/*
		// cstr
			void push( const char* cstr, int x ) const;
			void push( const char* cstr, uint x ) const;
			void push( const char* cstr, float x ) const;
			void push( const char* cstr, float w, float x, float y, float z ) const;
			void push( const char* cstr, bool transpose, float* mat ) const;

			void push( const char* cstr, const uf::Vector2& vec2 ) const;
			void push( const char* cstr, const uf::Vector3& vec3 ) const;
			void push( const char* cstr, const uf::Vector4& vec4 ) const;
			void push( const char* cstr, bool transpose, const uf::Matrix4& mat ) const;
		*/
		};
	}
	typedef ogl::Shader Shader;
}

#endif