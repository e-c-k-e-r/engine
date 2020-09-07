#pragma once

#include <uf/ext/vulkan/device.h>
#include <uf/utils/image/image.h>

#include <uf/ext/vulkan/rendertarget.h>

namespace ext {
	namespace vulkan {
		struct UF_API Sampler {
			Device* device = NULL;

			VkSampler sampler;
			struct {
				struct {
					VkFilter min = VK_FILTER_LINEAR;
					VkFilter mag = VK_FILTER_LINEAR;
				} filter;
				struct {
					VkSamplerAddressMode u = VK_SAMPLER_ADDRESS_MODE_REPEAT, v = VK_SAMPLER_ADDRESS_MODE_REPEAT, w = VK_SAMPLER_ADDRESS_MODE_REPEAT;
				} addressMode;
				struct {
					VkSamplerMipmapMode mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
					float lodBias = 0.0f;
				} mip;
				VkCompareOp compareOp = VK_COMPARE_OP_NEVER;
				struct {
					float min = 0.0f;
					float max = 0.0f;
				} lod;
				float maxAnisotropy;

				VkDescriptorImageInfo info;
			} descriptor;

			void initialize( Device& device );
			void destroy();
		};
		struct UF_API Texture {
			Device* device = nullptr;

			VkImage image;
			VkImageView view;
			VkImageLayout imageLayout;
			VkDeviceMemory deviceMemory;
			VkDescriptorImageInfo descriptor;
			
			Sampler sampler;

			VmaAllocation allocation;
			VmaAllocationInfo allocationInfo;

			uint32_t width, height;
			uint32_t mips;
			uint32_t layers;
			// RAII
			void initialize( Device& device, size_t width, size_t height );
			void updateDescriptors();
			void destroy();
			bool generated() const;
			static void setImageLayout(
				VkCommandBuffer cmdbuffer,
				VkImage image,
				VkImageLayout oldImageLayout,
				VkImageLayout newImageLayout,
				VkImageSubresourceRange subresourceRange,
				VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
			);
			static void setImageLayout(
				VkCommandBuffer cmdbuffer,
				VkImage image,
				VkImageAspectFlags aspectMask,
				VkImageLayout oldImageLayout,
				VkImageLayout newImageLayout,
				VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
			);
		};
		struct UF_API Texture2D : public Texture {
			static Texture2D empty;
			void loadFromFile(
				std::string filename, 
				VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
				VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
				VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
				bool forceLinear = false
			);
			void loadFromFile(
				std::string filename, 
				Device& device,
				VkQueue copyQueue,
				VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
				VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
				VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
				bool forceLinear = false
			);
			void loadFromImage(
				uf::Image& image, 
				Device& device,
				VkQueue copyQueue,
				VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
				VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
				VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
				bool forceLinear = false
			);
			void loadFromImage(
				uf::Image& image,
				VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
				VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
				VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
				bool forceLinear = false
			);
			void fromBuffers(
				void* buffer,
				VkDeviceSize bufferSize,
				VkFormat format,
				uint32_t texWidth,
				uint32_t texHeight,
				Device& device,
				VkQueue copyQueue,
				VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
				VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
			void asRenderTarget(
				Device& device,
				uint32_t texWidth,
				uint32_t texHeight,
				VkQueue copyQueue,
				VkFormat format = VK_FORMAT_R8G8B8A8_UNORM
			);
			void aliasAttachment( const RenderTarget::Attachment& attachment, bool = true );
		};
	}
}