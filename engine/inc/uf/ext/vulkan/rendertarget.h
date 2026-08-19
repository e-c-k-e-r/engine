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
					uint8_t mips = 0;
					bool screenshottable = false;
					bool aliased = false;

					uint32_t width = 0;
					uint32_t height = 0;
					uint8_t layers = 0;
				} descriptor;

				uf::stl::vector<VkImage> images;
				uf::stl::vector<VkDeviceMemory> mems;
				uf::stl::vector<VkImageView> frameViews;
				uf::stl::vector<VkImageView> framebufferViews;
				uf::stl::vector<VmaAllocation> allocations;
				uf::stl::vector<VmaAllocationInfo> allocationInfos;
				uf::stl::vector<uf::stl::vector<VkImageView>> subViews;

				VkImage image = VK_NULL_HANDLE;
				VkDeviceMemory mem = VK_NULL_HANDLE;
				VkImageView view = VK_NULL_HANDLE;
				VkImageView framebufferView = VK_NULL_HANDLE;
				uf::stl::vector<VkImageView> views;
				VmaAllocation allocation = VK_NULL_HANDLE;
				VmaAllocationInfo allocationInfo = {};
				VkPipelineColorBlendAttachmentState blendState = {};

				VkImage getImage(size_t frame = 0) const {
					return frame < images.size() ? images[frame] : image;
				}
				VkImageView getView(size_t frame = 0) const {
					return frame < frameViews.size() ? frameViews[frame] : view;
				}
				VkImageView getFramebufferView(size_t frame = 0) const {
					if (frame < framebufferViews.size() && framebufferViews[frame] != VK_NULL_HANDLE) {
						return framebufferViews[frame];
					}
					return getView(frame);
				}
			};
			uf::stl::vector<Attachment> attachments;

			struct Subpass {
				VkPipelineStageFlags stage = {};
				VkAccessFlags access = {};
				uint8_t layer = 0;
				bool autoBuildPipeline;

				uf::stl::vector<VkAttachmentReference> colors = {};
				uf::stl::vector<VkAttachmentReference> inputs = {};
				uf::stl::vector<VkAttachmentReference> resolves = {};
				VkAttachmentReference depth = {};
			};
			uf::stl::vector<Subpass> passes;

			bool initialized = false;
			Device* device = VK_NULL_HANDLE;
			VkRenderPass renderPass = VK_NULL_HANDLE;
			uf::stl::vector<VkFramebuffer> framebuffers;
			uint32_t width = 0;
			uint32_t height = 0;
			uint8_t views = 1;
			float scale = 0;

			// RAII
			void initialize( Device& device );
			void destroy();
			void addPass( VkPipelineStageFlags, VkAccessFlags, const uf::stl::vector<size_t>&, const uf::stl::vector<size_t>&, const uf::stl::vector<size_t>&, size_t, size_t = 0, bool = true );
			size_t attach( const Attachment::Descriptor& descriptor, Attachment* attachment = NULL );
			size_t aliasAttachment( const Attachment& attachment );
		};
	}
}