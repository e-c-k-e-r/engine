#pragma once

#include <uf/ext/opengl/device.h>
#include <uf/ext/opengl/swapchain.h>
#include <uf/ext/opengl/initializers.h>
#include <uf/ext/opengl/texture.h>
#include <uf/ext/opengl/shader.h>
#include <uf/utils/mesh/mesh.h>

#define UF_GRAPHIC_POINTERED_USERDATA 1

namespace ext {
	namespace opengl {
		#if UF_GRAPHIC_POINTERED_USERDATA
			typedef uf::PointeredUserdata userdata_t;
		#else
			typedef uf::Userdata userdata_t;
		#endif

		struct Graphic;
		struct CommandBuffer;

		ext::json::Value definitionToJson(/*const*/ ext::json::Value& definition );
		ext::opengl::userdata_t jsonToUserdata( const ext::json::Value& payload, const ext::json::Value& definition );

		struct UF_API Shader : public Buffers {
			bool aliased = false;

			Device* device = NULL;
			uf::stl::string filename = "";

		#if !UF_USE_OPENGL_FIXED_FUNCTION
			GLuint module = GL_NULL_HANDLE;
		#else
			typedef std::function<void(const Shader&, const Graphic&, void*)> module_t;
			module_t module;
		#endif
			struct Descriptor {
				GLuint module;
				enums::Shader::type_t stage;
			} descriptor;

			struct Metadata {
				uf::Serializer json;
				
				uf::stl::string pipeline = "";
				uf::stl::string type = "";

				struct Definition {
					struct Texture {
						uf::stl::string name = "";
						uint32_t index = 0;
						uint32_t binding = 0;
						uint32_t size = 0;
						ext::opengl::enums::Image::viewType_t type{};
					};
					struct Uniform {
						uf::stl::string name = "";
						uint32_t index = 0;
						uint32_t binding = 0;
						uint32_t size = 0;
					};
					struct Storage {
						uf::stl::string name = "";
						uint32_t index = 0;
						uint32_t binding = 0;
						uint32_t size = 0;
					};
					struct PushConstant {
						uf::stl::string name = "";
						uint32_t index = 0;
						uint32_t binding = 0;
						uint32_t size = 0;
					};
					uf::stl::unordered_map<size_t, Texture> textures;
					uf::stl::unordered_map<uf::stl::string, Uniform> uniforms;
					uf::stl::unordered_map<uf::stl::string, Storage> storage;
					uf::stl::unordered_map<uf::stl::string, PushConstant> pushConstants;
				} definitions;
			} metadata;

			ext::opengl::userdata_t specializationConstants;
			uf::stl::vector<ext::opengl::userdata_t> pushConstants;
			uf::stl::vector<ext::opengl::userdata_t> uniforms;
		//	~Shader();
			void initialize( Device& device, const uf::stl::string&, enums::Shader::type_t stage );
			void destroy();
			bool validate();

			bool hasUniform( const uf::stl::string& name ) const;

			Buffer& getUniformBuffer( const uf::stl::string& name );
			const Buffer& getUniformBuffer( const uf::stl::string& name ) const;
			
			ext::opengl::userdata_t& getUniform( const uf::stl::string& name );
			const ext::opengl::userdata_t& getUniform( const uf::stl::string& name ) const;

			bool updateUniform( const uf::stl::string& name, const void*, size_t ) const;
			inline bool updateUniform( const uf::stl::string& name ) const {
				if ( !hasUniform(name) ) return false;
				return updateUniform(name, getUniform(name));
			}
			inline bool updateUniform( const uf::stl::string& name, const ext::opengl::userdata_t& userdata ) const {
				return updateUniform(name, (const void*) userdata, userdata.size() );
			}

			bool hasStorage( const uf::stl::string& name ) const;
			Buffer& getStorageBuffer( const uf::stl::string& name );
			const Buffer& getStorageBuffer( const uf::stl::string& name ) const;
			
			uf::Serializer getUniformJson( const uf::stl::string& name, bool cache = true );
			ext::opengl::userdata_t getUniformUserdata( const uf::stl::string& name, const ext::json::Value& payload );
			inline bool updateUniform( const uf::stl::string& name, const ext::json::Value& payload ) {
				if ( !hasUniform(name) ) return false;
				return updateUniform( name, getUniformUserdata( name, payload ) );
			}

			uf::Serializer getStorageJson( const uf::stl::string& name, bool cache = true );
			ext::opengl::userdata_t getStorageUserdata( const uf::stl::string& name, const ext::json::Value& payload );

		#if UF_USE_OPENGL_FIXED_FUNCTION
			static void bind( const uf::stl::string& name, const module_t& module );
			void execute( const Graphic& graphic, void* = NULL ) const;
		#endif
		};
	}
}