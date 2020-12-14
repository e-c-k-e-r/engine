#pragma once 	

#include <uf/ext/vulkan/device.h>
#include <uf/ext/vulkan/swapchain.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/texture.h>
#include <uf/utils/graphic/mesh.h>

namespace ext {
	namespace vulkan {
		ext::json::Value definitionToJson(/*const*/ ext::json::Value& definition );
		uf::Userdata jsonToUserdata( const ext::json::Value& payload, const ext::json::Value& definition );

		struct Graphic;
		struct GraphicDescriptor {
			std::string renderMode = "";
			uint32_t renderTarget = 0;
			uint32_t subpass = 0;

			uf::BaseGeometry geometry;
			size_t indices = 0;

			VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			VkPolygonMode fill = VK_POLYGON_MODE_FILL;
			VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
			float lineWidth = 1.0f;
			VkFrontFace frontFace = VK_FRONT_FACE_CLOCKWISE;
			struct {
				VkBool32 test = true;
				VkBool32 write = true;
				VkCompareOp operation = VK_COMPARE_OP_GREATER_OR_EQUAL;
			} depthTest;

			std::string hash() const;
			bool operator==( const GraphicDescriptor& right ) const { return this->hash() == right.hash(); }
		};
		struct UF_API Shader : public Buffers {
			bool aliased = false;

			Device* device = NULL;
			std::string filename = "";

			VkShaderModule module = VK_NULL_HANDLE;
			VkPipelineShaderStageCreateInfo descriptor;
			std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
			std::vector<VkSpecializationMapEntry> specializationMapEntries;
			VkSpecializationInfo specializationInfo;

			std::vector<uf::Userdata> pushConstants;
			std::vector<uint8_t> specializationConstants;
			std::vector<uf::Userdata> uniforms;
			uf::Serializer metadata;
		//	~Shader();
			void initialize( Device& device, const std::string&, VkShaderStageFlagBits );
			void destroy();
			bool validate();

			bool hasUniform( const std::string& name );

			Buffer* getUniformBuffer( const std::string& name );
			uf::Userdata& getUniform( const std::string& name );
			uf::Serializer getUniformJson( const std::string& name, bool cache = true );
			uf::Userdata getUniformUserdata( const std::string& name, const ext::json::Value& payload );
			bool updateUniform( const std::string& name );
			bool updateUniform( const std::string& name, const uf::Userdata& );
			bool updateUniform( const std::string& name, const ext::json::Value& payload );
			
			bool hasStorage( const std::string& name );
			Buffer* getStorageBuffer( const std::string& name );
			uf::Serializer getStorageJson( const std::string& name, bool cache = true );
			uf::Userdata getStorageUserdata( const std::string& name, const ext::json::Value& payload );
		};
		struct UF_API Pipeline {
			bool aliased = false;

			Device* device = NULL;

			VkPipeline pipeline;
			VkPipelineLayout pipelineLayout;
			VkDescriptorSetLayout descriptorSetLayout;
			VkDescriptorPool descriptorPool;
			VkDescriptorSet descriptorSet;

			void initialize( Graphic& graphic );
			void initialize( Graphic& graphic, GraphicDescriptor& descriptor );
			void update( Graphic& graphic );
			void update( Graphic& graphic, GraphicDescriptor& descriptor );
			void record( Graphic& graphic, VkCommandBuffer );
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

			void attachShader( const std::string&, VkShaderStageFlagBits );
			void initializeShaders( const std::vector<std::pair<std::string, VkShaderStageFlagBits>>& );

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
			void initializeGeometry( uf::BaseMesh<T, U>& mesh, bool stage = VK_DEFAULT_STAGE_BUFFERS );

			bool hasPipeline( GraphicDescriptor& descriptor );
			void initializePipeline();
			Pipeline& initializePipeline( GraphicDescriptor& descriptor, bool update = true );
			Pipeline& getPipeline();
			Pipeline& getPipeline( GraphicDescriptor& descriptor );
			void updatePipelines();
			
			void record( VkCommandBuffer commandBuffer );
			void record( VkCommandBuffer commandBuffer, GraphicDescriptor& descriptor );

			bool hasStorage( const std::string& name );
			Buffer* getStorageBuffer( const std::string& name );
			uf::Serializer getStorageJson( const std::string& name, bool cache = true );
			uf::Userdata getStorageUserdata( const std::string& name, const ext::json::Value& payload );
		};
	}
}


#include "graphic.inl"