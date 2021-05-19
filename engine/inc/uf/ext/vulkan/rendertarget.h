#pragma once

#include <uf/ext/vulkan/device.h>

namespace ext {
	namespace vulkan {
		struct UF_API RenderTarget {
			struct Attachment {
				struct Descriptor {
					VkFormat format;
					VkImageLayout layout;
					VkImageUsageFlags usage;
					bool blend = false;
					uint8_t samples = 1;
					bool screenshottable = true;
					bool aliased = false;
				} descriptor;

				VkImage image;
				VkDeviceMemory mem;
				VkImageView view;
				std::vector<VkImageView> views;
				VmaAllocation allocation;
				VmaAllocationInfo allocationInfo;
				VkPipelineColorBlendAttachmentState blendState;
			};
			std::vector<Attachment> attachments;

			struct Subpass {
				VkPipelineStageFlags stage;
				VkAccessFlags access;
				uint8_t layer;
				bool autoBuildPipeline;

				std::vector<VkAttachmentReference> colors;
				std::vector<VkAttachmentReference> inputs;
				std::vector<VkAttachmentReference> resolves;
				VkAttachmentReference depth;
			};
			std::vector<Subpass> passes;

			bool initialized = false;
			Device* device = VK_NULL_HANDLE;
			VkRenderPass renderPass = VK_NULL_HANDLE;
			std::vector<VkFramebuffer> framebuffers;
			uint32_t width = 0;
			uint32_t height = 0;
			uint8_t views = 1;
			// RAII
			void initialize( Device& device );
			void destroy();
			void addPass( VkPipelineStageFlags, VkAccessFlags, const std::vector<size_t>&, const std::vector<size_t>&, const std::vector<size_t>&, size_t, size_t = 0, bool = true  );
			size_t attach( const Attachment::Descriptor& descriptor, Attachment* attachment = NULL );
		};
	}
}