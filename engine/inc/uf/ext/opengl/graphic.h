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
		struct UF_API Pipeline {
			bool aliased = false;

			Device* device = NULL;
		#if !UF_USE_OPENGL_FIXED_FUNCTION
			GLuint pipeline;
			GLuint vertexArray;
			std::vector<Buffer::Descriptor> descriptorSet;
		#endif

			struct Descriptor {
				GLuint pipeline;
				GLuint vertexArray;
			} descriptor;

			struct {
				uf::Serializer json;
				
				std::string type = "";
				bool process = true;
			} metadata;

			void initialize( Graphic& graphic );
			void initialize( Graphic& graphic, GraphicDescriptor& descriptor );
			void update( Graphic& graphic );
			void update( Graphic& graphic, GraphicDescriptor& descriptor );
			void record( Graphic& graphic, CommandBuffer&, size_t = 0, size_t = 0 );
			void destroy();
		};
		struct UF_API Material {
			bool aliased = false;
			Device* device;

			std::vector<Sampler> samplers;
			std::vector<Texture> textures;
			std::vector<Shader> shaders;
			struct Metadata {
				uf::Serializer json;
				std::unordered_map<std::string, size_t> shaders;
			} metadata;

			void initialize( Device& device );
			void destroy();

			void attachShader( const std::string&, enums::Shader::type_t, const std::string& pipeline = "" );
			void initializeShaders( const std::vector<std::pair<std::string, enums::Shader::type_t>>&, const std::string& pipeline = "" );

			bool hasShader( const std::string& type, const std::string& pipeline = "" );
			Shader& getShader( const std::string& type, const std::string& pipeline = "" );

			bool validate();
		};
		struct UF_API Graphic : public Buffers {
			GraphicDescriptor descriptor;

			bool initialized = false;
			bool process = true;
			Material material;
			std::unordered_map<GraphicDescriptor::hash_t, Pipeline> pipelines;

			struct {
				std::unordered_map<std::string, size_t> buffers;
			} metadata;

			~Graphic();
			void initialize( const std::string& = "" );
			void destroy();
			
			template<typename T, typename U>
			void initializeMesh( uf::BaseMesh<T, U>& mesh, size_t = SIZE_MAX );
			void initializeGeometry( const uf::BaseGeometry& mesh );

			bool hasPipeline( GraphicDescriptor& descriptor );
			void initializePipeline();
			Pipeline& initializePipeline( GraphicDescriptor& descriptor, bool update = true );
			Pipeline& getPipeline();
			Pipeline& getPipeline( GraphicDescriptor& descriptor );
			void updatePipelines();
			
			void record( CommandBuffer& commandBuffer, size_t pass = 0, size_t draw = 0 );
			void record( CommandBuffer& commandBuffer, GraphicDescriptor& descriptor, size_t pass = 0, size_t draw = 0 );

			bool hasStorage( const std::string& name );
			Buffer* getStorageBuffer( const std::string& name );
			uf::Serializer getStorageJson( const std::string& name, bool cache = true );
			ext::opengl::userdata_t getStorageUserdata( const std::string& name, const ext::json::Value& payload );

			Buffer::Descriptor getUniform( size_t = 0 ) const;
			void updateUniform( void*, size_t, size_t = 0 );

			Buffer::Descriptor getStorage( size_t = 0 ) const;
			void updateStorage( void*, size_t, size_t = 0 );
		};
	}
}


#include "graphic.inl"