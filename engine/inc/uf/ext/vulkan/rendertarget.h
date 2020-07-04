#pragma once

#include <uf/ext/vulkan/device.h>

namespace ext {
	namespace vulkan {
		struct UF_API RenderTarget {
			typedef struct {
				VkImage image;
				VkImageLayout layout;
				VkDeviceMemory mem;
				VkImageView view;
			} Attachment;
			std::vector<Attachment> attachments;

			Device* device = nullptr;
			VkSampler sampler;
			VkDescriptorImageInfo descriptorImageInfo;
			VkRenderPass renderPass;
			VkFramebuffer framebuffer;
			bool commandBufferSet;
			// RAII
			void initialize( Device& device );
			void destroy();
		};
	}
}