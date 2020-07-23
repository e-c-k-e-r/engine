#pragma once

#include <uf/ext/vulkan/device.h>

namespace ext {
	namespace vulkan {
		struct UF_API RenderTarget {
			typedef struct {
				VkFormat format;
				VkImageLayout layout;
				VkImageUsageFlags usage;
				VkImage image;
				VkDeviceMemory mem;
				VkImageView view;
			} Attachment;
			std::vector<Attachment> attachments;

			typedef struct {
				VkPipelineStageFlags stage;
				VkAccessFlags access;

				std::vector<VkAttachmentReference> colors;
				std::vector<VkAttachmentReference> inputs;
				VkAttachmentReference depth;
			} Subpass;
			std::vector<Subpass> passes;

			bool initialized = false;
			Device* device = VK_NULL_HANDLE;
			VkRenderPass renderPass = VK_NULL_HANDLE;
			std::vector<VkFramebuffer> framebuffers;
			// RAII
			void initialize( Device& device );
			void destroy();
			void addPass( VkPipelineStageFlags, VkAccessFlags, const std::vector<size_t>&, const std::vector<size_t>&, size_t );
			size_t attach( VkFormat format, VkImageUsageFlags usage, Attachment* attachment = NULL );
		};
	}
}