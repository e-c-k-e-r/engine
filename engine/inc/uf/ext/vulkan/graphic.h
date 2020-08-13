#pragma once 	

#include <uf/ext/vulkan/device.h>
#include <uf/ext/vulkan/swapchain.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/texture.h>
#include <uf/ext/vulkan/rendermodes/base.h>

namespace ext {
	namespace vulkan {
		struct RenderMode;
		struct Graphic;

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
		//	std::vector<uf::Userdata> specializationConstants;
			uf::Userdata specializationConstants;
			std::vector<uf::Userdata> uniforms;

		//	~Shader();
			void initialize( Device& device, const std::string&, VkShaderStageFlagBits );
			void destroy();
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
			void update( Graphic& graphic );
			void record( Graphic& graphic, VkCommandBuffer );
			void destroy();
		};
		struct UF_API Material {
			bool aliased = false;
			Device* device;

			std::vector<Sampler> samplers;
			std::vector<Texture2D> textures;
			std::vector<Shader> shaders;

			void initialize( Device& device );
			void destroy();

			void attachShader( const std::string&, VkShaderStageFlagBits );
			void initializeShaders( const std::vector<std::pair<std::string, VkShaderStageFlagBits>>& );
		};
		struct UF_API Graphic : public Buffers {
			struct UF_API VertexDescriptor {
				VkFormat format; // VK_FORMAT_R32G32B32_SFLOAT
				std::size_t offset; // offsetof(Vertex, position)
			};
			struct Descriptor {
				std::string renderMode = "";
				uint32_t subpass = 0;

				std::size_t size; // sizeof(Vertex)
				std::vector<VertexDescriptor> attributes;
				size_t indices = 0;

				VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
				VkPolygonMode fill = VK_POLYGON_MODE_FILL;
				float lineWidth = 1.0f;
				VkFrontFace frontFace = VK_FRONT_FACE_CLOCKWISE;
				struct {
					VkBool32 test = true;
					VkBool32 write = true;
					VkCompareOp operation = VK_COMPARE_OP_GREATER_OR_EQUAL;
				} depthTest;

				std::string hash() const;
				bool operator==( const Descriptor& right ) const { return this->hash() == right.hash(); }
			} descriptor;

			bool initialized = false;
			bool process = true;
			Material material;
			std::unordered_map<std::string, Pipeline> pipelines;

			void initialize( const std::string& = "" );
			void initializePipeline();
			void destroy();
			
			Pipeline& initializePipeline( Descriptor& descriptor, bool update = true );
			bool hasPipeline( Descriptor& descriptor );
			Pipeline& getPipeline( Descriptor& descriptor );
			
			void record( VkCommandBuffer commandBuffer );
		};
	}
}
