#pragma once 	

#include <uf/ext/vulkan/device.h>
#include <uf/ext/vulkan/swapchain.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/texture.h>
#include <uf/ext/vulkan/shader.h>
#include <uf/utils/graphic/descriptor.h>
#include <uf/utils/graphic/mesh.h>

namespace ext {
	namespace vulkan {
		struct Graphic;
		struct UF_API Pipeline {
			bool aliased = false;

			Device* device = NULL;

			VkPipeline pipeline;
			VkPipelineLayout pipelineLayout;
			VkDescriptorSetLayout descriptorSetLayout;
			VkDescriptorPool descriptorPool;
			VkDescriptorSet descriptorSet;
			GraphicDescriptor descriptor;

			uf::Serializer metadata;

			void initialize( Graphic& graphic );
			void initialize( Graphic& graphic, GraphicDescriptor& descriptor );
			void update( Graphic& graphic );
			void update( Graphic& graphic, GraphicDescriptor& descriptor );
			void record( Graphic& graphic, VkCommandBuffer, size_t = 0, size_t = 0 );
			void destroy();

			std::vector<Shader*> getShaders( std::vector<Shader>& );
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

			void attachShader( const std::string&, VkShaderStageFlagBits, const std::string& pipeline = "" );
			void initializeShaders( const std::vector<std::pair<std::string, VkShaderStageFlagBits>>&, const std::string& pipeline = "" );

			bool hasShader( const std::string& type, const std::string& pipeline = "" );
			Shader& getShader( const std::string& type, const std::string& pipeline = "" );

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
			void initializeMesh( uf::BaseMesh<T, U>& mesh, size_t = SIZE_MAX );
			void initializeGeometry( const uf::BaseGeometry& mesh );

			bool hasPipeline( GraphicDescriptor& descriptor );
			void initializePipeline();
			Pipeline& initializePipeline( GraphicDescriptor& descriptor, bool update = true );
			Pipeline& getPipeline();
			Pipeline& getPipeline( GraphicDescriptor& descriptor );
			void updatePipelines();
			
			void record( VkCommandBuffer commandBuffer, size_t pass = 0, size_t draw = 0 );
			void record( VkCommandBuffer commandBuffer, GraphicDescriptor& descriptor, size_t pass = 0, size_t draw = 0 );

			bool hasStorage( const std::string& name );
			Buffer* getStorageBuffer( const std::string& name );
			uf::Serializer getStorageJson( const std::string& name, bool cache = true );
			ext::vulkan::userdata_t getStorageUserdata( const std::string& name, const ext::json::Value& payload );
		};
	}
}


#include "graphic.inl"