#pragma once

#include <uf/ext/opengl/device.h>
#include <uf/ext/opengl/swapchain.h>
#include <uf/ext/opengl/initializers.h>
#include <uf/ext/opengl/texture.h>
#include <uf/ext/opengl/commands.h>
#include <uf/utils/graphic/mesh.h>
#include <uf/utils/graphic/descriptor.h>

#define UF_GRAPHIC_POINTERED_USERDATA 1

struct GLdescriptorSetLayoutBinding {
	uint32_t binding;
	GLhandle(VkDescriptorType) descriptorType;
	uint32_t descriptorCount;
	GLhandle(VkShaderStageFlags) stageFlags;
	const GLhandle(VkSampler)* pImmutableSamplers;
};

namespace ext {
	namespace opengl {
		#if UF_GRAPHIC_POINTERED_USERDATA
			typedef uf::PointeredUserdata userdata_t;
		#else
			typedef uf::Userdata userdata_t;
		#endif

		ext::json::Value definitionToJson(/*const*/ ext::json::Value& definition );
		ext::opengl::userdata_t jsonToUserdata( const ext::json::Value& payload, const ext::json::Value& definition );

		struct Graphic;
		struct UF_API Shader : public Buffers {
			bool aliased = false;

			Device* device = NULL;
			std::string filename = "";

			GLhandle(VkShaderModule) module = GL_NULL_HANDLE;
			GLhandle(VkPipelineShaderStageCreateInfo) descriptor;
			std::vector<GLdescriptorSetLayoutBinding> descriptorSetLayoutBindings;
			std::vector<GLhandle(VkSpecializationMapEntry)> specializationMapEntries;
			GLhandle(VkSpecializationInfo) specializationInfo;

			uf::Serializer metadata;

			ext::opengl::userdata_t specializationConstants;
			std::vector<ext::opengl::userdata_t> pushConstants;
			std::vector<ext::opengl::userdata_t> uniforms;
		//	~Shader();
			void initialize( Device& device, const std::string&, GLhandle(VkShaderStageFlagBits) );
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
		};
		struct UF_API Pipeline {
			bool aliased = false;

			Device* device = NULL;

			GLhandle(VkPipeline) pipeline;
			GLhandle(VkPipelineLayout) pipelineLayout;
			GLhandle(VkDescriptorSetLayout) descriptorSetLayout;
			GLhandle(VkDescriptorPool) descriptorPool;
			GLhandle(VkDescriptorSet) descriptorSet;
			GraphicDescriptor descriptor;

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
			std::vector<Texture2D> textures;
			std::vector<Shader> shaders;
			uf::Serializer metadata;

			void initialize( Device& device );
			void destroy();

			void attachShader( const std::string&, GLhandle(VkShaderStageFlagBits) );
			void initializeShaders( const std::vector<std::pair<std::string, GLhandle(VkShaderStageFlagBits)>>& );

			bool hasShader( const std::string& type );
			Shader& getShader( const std::string& type );

			bool validate();
		};
		struct UF_API Graphic : public Buffers {
			GraphicDescriptor descriptor;

			bool initialized = false;
			bool process = true;
			Material material;
			std::unordered_map<std::string, Pipeline> pipelines;

			~Graphic();
			void initialize( const std::string& = "" );
			void destroy();
			
			template<typename T, typename U>
			void initializeGeometry( uf::BaseMesh<T, U>& mesh, size_t = SIZE_MAX );

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
		};
	}
}


#include "graphic.inl"