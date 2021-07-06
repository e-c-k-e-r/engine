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
		struct UF_API Pipeline {
			bool aliased = false;

			Device* device = NULL;
		#if !UF_USE_OPENGL_FIXED_FUNCTION
			GLuint pipeline;
			GLuint vertexArray;
			uf::stl::vector<Buffer::Descriptor> descriptorSet;
		#endif

			struct Descriptor {
				GLuint pipeline;
				GLuint vertexArray;
			} descriptor;

			struct {
				uf::Serializer json;
				
				uf::stl::string type = "";
				bool process = true;
			} metadata;

			void initialize( const Graphic& graphic );
			void initialize( const Graphic& graphic, const GraphicDescriptor& descriptor );
			void update( const Graphic& graphic );
			void update( const Graphic& graphic, const GraphicDescriptor& descriptor );
			void record( const Graphic& graphic, CommandBuffer&, size_t = 0, size_t = 0 ) const;
			void destroy();
		};
		struct UF_API Material {
			bool aliased = false;
			Device* device;

			uf::stl::vector<Sampler> samplers;
			uf::stl::vector<Texture> textures;
			uf::stl::vector<Shader> shaders;
			struct Metadata {
				uf::Serializer json;
				uf::stl::unordered_map<uf::stl::string, size_t> shaders;
			} metadata;

			void initialize( Device& device );
			void destroy();

			void attachShader( const uf::stl::string&, enums::Shader::type_t, const uf::stl::string& pipeline = "" );
			void initializeShaders( const uf::stl::vector<std::pair<uf::stl::string, enums::Shader::type_t>>&, const uf::stl::string& pipeline = "" );

			bool hasShader( const uf::stl::string& type, const uf::stl::string& pipeline = "" ) const;
			Shader& getShader( const uf::stl::string& type, const uf::stl::string& pipeline = "" );
			const Shader& getShader( const uf::stl::string& type, const uf::stl::string& pipeline = "" ) const;

			bool validate();
		};
		struct UF_API Graphic : public Buffers {
			GraphicDescriptor descriptor;

			bool initialized = false;
			bool process = true;
			Material material;
			uf::stl::unordered_map<GraphicDescriptor::hash_t, Pipeline> pipelines;

			struct {
				uf::stl::unordered_map<uf::stl::string, size_t> buffers;
			} metadata;

			~Graphic();
			void initialize( const uf::stl::string& = "" );
			void destroy();
			
			template<typename T, typename U>
			void initializeMesh( uf::Mesh<T, U>& mesh, size_t = SIZE_MAX );
			
			void initializeAttributes( const pod::Mesh::Attributes& mesh );

			bool hasPipeline( const GraphicDescriptor& descriptor ) const;
			void initializePipeline();
			Pipeline& initializePipeline( const GraphicDescriptor& descriptor, bool update = true );
			Pipeline& getPipeline();
			const Pipeline& getPipeline() const;
			Pipeline& getPipeline( const GraphicDescriptor& descriptor );
			const Pipeline& getPipeline( const GraphicDescriptor& descriptor ) const;
			void updatePipelines();
			
			void record( CommandBuffer& commandBuffer, size_t pass = 0, size_t draw = 0 ) const;
			void record( CommandBuffer& commandBuffer, const GraphicDescriptor& descriptor, size_t pass = 0, size_t draw = 0 ) const;

			bool hasStorage( const uf::stl::string& name ) const;
			Buffer* getStorageBuffer( const uf::stl::string& name );
			const Buffer* getStorageBuffer( const uf::stl::string& name ) const;

			Buffer::Descriptor getUniform( size_t = 0 ) const;
			void updateUniform( const void*, size_t, size_t = 0 ) const;

			Buffer::Descriptor getStorage( size_t = 0 ) const;
			void updateStorage( const void*, size_t, size_t = 0 ) const;
			
			uf::Serializer getStorageJson( const uf::stl::string& name, bool cache = true );
			ext::opengl::userdata_t getStorageUserdata( const uf::stl::string& name, const ext::json::Value& payload );
		};
	}
}


#include "graphic.inl"