#pragma once

#include <uf/ext/vulkan/device.h>

namespace ext {
	namespace vulkan {
		struct UF_API RenderTarget {
			typedef struct {
				VkFormat format;
				VkImageLayout layout;
				VkImageUsageFlags usage;
				VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
				bool aliased = false;

				VkImage image;
				VkDeviceMemory mem;
				VkImageView view;
				VmaAllocation allocation;
				VmaAllocationInfo allocationInfo;
				VkPipelineColorBlendAttachmentState blendState;
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
			uint32_t width = 0;
			uint32_t height = 0;
			uint8_t multiviews = 1;
			uint8_t samples = 1;
			// RAII
			void initialize( Device& device );
			void destroy();
			void addPass( VkPipelineStageFlags, VkAccessFlags, const std::vector<size_t>&, const std::vector<size_t>&, size_t );
			size_t attach( VkFormat format, VkImageUsageFlags usage, VkImageLayout layout, bool blend = false, Attachment* attachment = NULL );
		};
	}
}