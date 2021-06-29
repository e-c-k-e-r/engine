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

			struct {
				uf::Serializer json;
				
				uf::stl::string type = "";
				bool process = true;
			} metadata;

			void initialize( Graphic& graphic );
			void initialize( Graphic& graphic, GraphicDescriptor& descriptor );
			void update( Graphic& graphic );
			void update( Graphic& graphic, GraphicDescriptor& descriptor );
			void record( Graphic& graphic, VkCommandBuffer, size_t = 0, size_t = 0 );
			void destroy();

			uf::stl::vector<Shader*> getShaders( uf::stl::vector<Shader>& );
		};
		struct UF_API Material {
			bool aliased = false;
			Device* device;

			uf::stl::vector<Sampler> samplers;
			uf::stl::vector<Texture2D> textures;
			uf::stl::vector<Shader> shaders;

			struct Metadata {
				uf::Serializer json;
				uf::stl::unordered_map<uf::stl::string, size_t> shaders;
			} metadata;

			void initialize( Device& device );
			void destroy();

			void attachShader( const uf::stl::string&, VkShaderStageFlagBits, const uf::stl::string& pipeline = "" );
			void initializeShaders( const uf::stl::vector<std::pair<uf::stl::string, VkShaderStageFlagBits>>&, const uf::stl::string& pipeline = "" );

			bool hasShader( const uf::stl::string& type, const uf::stl::string& pipeline = "" );
			Shader& getShader( const uf::stl::string& type, const uf::stl::string& pipeline = "" );

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

			bool hasStorage( const uf::stl::string& name );
			Buffer* getStorageBuffer( const uf::stl::string& name );
			uf::Serializer getStorageJson( const uf::stl::string& name, bool cache = true );
			ext::vulkan::userdata_t getStorageUserdata( const uf::stl::string& name, const ext::json::Value& payload );
		};
	}
}


#include "graphic.inl"