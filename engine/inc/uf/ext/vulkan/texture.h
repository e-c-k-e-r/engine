#pragma once

#include <uf/ext/vulkan/device.h>
#include <uf/utils/image/image.h>

namespace ext {
	namespace vulkan {
		struct UF_API Texture {
			Device* device = nullptr;
			VkImage image;
			VkImageLayout imageLayout;
			VkDeviceMemory deviceMemory;

			VmaAllocation allocation;
			VmaAllocationInfo allocationInfo;

			VkImageView view;
			uint32_t width, height;
			uint32_t mips;
			uint32_t layers;
			VkDescriptorImageInfo descriptor;
			VkSampler sampler; 					// optional
			// RAII
			void initialize( Device& device, size_t width, size_t height );
			void updateDescriptors();
			void destroy();
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
			void fromBuffers(
				void* buffer,
				VkDeviceSize bufferSize,
				VkFormat format,
				uint32_t texWidth,
				uint32_t texHeight,
				Device& device,
				VkQueue copyQueue,
				VkFilter filter = VK_FILTER_LINEAR,
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
		};
	}
}