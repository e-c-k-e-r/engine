#pragma once

#include <uf/ext/opengl/device.h>
#include <uf/ext/opengl/swapchain.h>
#include <uf/ext/opengl/initializers.h>
#include <uf/ext/opengl/texture.h>
#include <uf/ext/opengl/shader.h>
#include <uf/utils/graphic/mesh.h>
#include <uf/utils/graphic/descriptor.h>

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
			std::string filename = "";

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
				
				std::string pipeline = "";
				std::string type = "";

				struct Definition {
					struct Texture {
						std::string name = "";
						uint32_t index = 0;
						uint32_t binding = 0;
						uint32_t size = 0;
						ext::opengl::enums::Image::viewType_t type{};
					};
					struct Uniform {
						std::string name = "";
						uint32_t index = 0;
						uint32_t binding = 0;
						uint32_t size = 0;
					};
					struct Storage {
						std::string name = "";
						uint32_t index = 0;
						uint32_t binding = 0;
						uint32_t size = 0;
					};
					struct PushConstant {
						std::string name = "";
						uint32_t index = 0;
						uint32_t binding = 0;
						uint32_t size = 0;
					};
					std::unordered_map<size_t, Texture> textures;
					std::unordered_map<std::string, Uniform> uniforms;
					std::unordered_map<std::string, Storage> storage;
					std::unordered_map<std::string, PushConstant> pushConstants;
				} definitions;
			} metadata;

			ext::opengl::userdata_t specializationConstants;
			std::vector<ext::opengl::userdata_t> pushConstants;
			std::vector<ext::opengl::userdata_t> uniforms;
		//	~Shader();
			void initialize( Device& device, const std::string&, enums::Shader::type_t stage );
			void destroy();
			bool validate();

			bool hasUniform( const std::string& name );

			Buffer* getUniformBuffer( const std::string& name );
			ext::opengl::userdata_t& getUniform( const std::string& name );
			uf::Serializer getUniformJson( const std::string& name, bool cache = true );
			ext::opengl::userdata_t getUniformUserdata( const std::string& name, const ext::json::Value& payload );
			bool updateUniform( const std::string& name );
			bool updateUniform( const std::string& name, const ext::opengl::userdata_t& );
			bool updateUniform( const std::string& name, const ext::json::Value& payload );
			
			bool hasStorage( const std::string& name );
			Buffer* getStorageBuffer( const std::string& name );
			uf::Serializer getStorageJson( const std::string& name, bool cache = true );
			ext::opengl::userdata_t getStorageUserdata( const std::string& name, const ext::json::Value& payload );

		#if UF_USE_OPENGL_FIXED_FUNCTION
			static void bind( const std::string& name, const module_t& module );
			void execute( const Graphic& graphic, void* = NULL );
		#endif
		};
	}
}