#include <uf/utils/memory/unordered_set.h>

namespace {
	void transitionAttachmentsTo(
		ext::vulkan::RenderMode* self,
		ext::vulkan::Shader& shader,
		VkCommandBuffer commandBuffer,
		size_t frame,
		VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	) {
		uf::stl::unordered_set<VkImage> transitioned;

		VkImageSubresourceRange subresourceRange;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = 1;
		subresourceRange.baseArrayLayer = 0;
		subresourceRange.layerCount = self->metadata.eyes;
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		for ( auto& descriptor : shader.metadata.aliases.attachments ) {
			if ( descriptor.layout == VK_IMAGE_LAYOUT_UNDEFINED ) continue;
			VkImage image = VK_NULL_HANDLE;
			VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			size_t mips = 1;

			if ( descriptor.renderMode ) {
				if ( descriptor.renderMode->hasAttachment(descriptor.name) ) {
					auto& attachment = descriptor.renderMode->getAttachment(descriptor.name);
					image = attachment.getImage(frame);
					mips = attachment.descriptor.mips;
					initialLayout = attachment.descriptor.layout;
				}

			} else if ( self->hasAttachment(descriptor.name) ) {
				if ( self->hasAttachment(descriptor.name) ) {
					auto& attachment = self->getAttachment(descriptor.name);
					image = attachment.getImage(frame);
					mips = attachment.descriptor.mips;
					initialLayout = attachment.descriptor.layout;
				}
			}
			if ( image == VK_NULL_HANDLE ) continue;

			if ( transitioned.count(image) > 0 ) continue;
			transitioned.insert(image);

			bool isDepth = descriptor.name.starts_with("depth");
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.aspectMask = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
			VkImageLayout oldLayout = layout;
			if ( isDepth && layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ) {
				oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			}

			VkImageLayout targetLayout = descriptor.layout;
			if ( isDepth && targetLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ) {
				targetLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			}

			uf::renderer::Texture::setImageLayout( commandBuffer, image, oldLayout, targetLayout, subresourceRange );
			if ( mips > 1 ) {
				subresourceRange.baseMipLevel = 1;
				subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
				uf::renderer::Texture::setImageLayout( commandBuffer, image, initialLayout, targetLayout, subresourceRange );
			}
		}
	}
	void transitionAttachmentsFrom(
		ext::vulkan::RenderMode* self,
		ext::vulkan::Shader& shader,
		VkCommandBuffer commandBuffer,
		size_t frame,
		VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	) {
		uf::stl::unordered_set<VkImage> transitioned;
		
		VkImageSubresourceRange subresourceRange;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = 1;
		subresourceRange.baseArrayLayer = 0;
		subresourceRange.layerCount = self->metadata.eyes;
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		for ( auto& descriptor : shader.metadata.aliases.attachments ) {
			if ( descriptor.layout == VK_IMAGE_LAYOUT_UNDEFINED ) continue;
			VkImage image = VK_NULL_HANDLE;
			VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			size_t mips = 1;

			if ( descriptor.renderMode ) {
				if ( descriptor.renderMode->hasAttachment(descriptor.name) ) {
					auto& attachment = descriptor.renderMode->getAttachment(descriptor.name);
					image = attachment.getImage(frame);
					mips = attachment.descriptor.mips;
					initialLayout = attachment.descriptor.layout;
				}

			} else if ( self->hasAttachment(descriptor.name) ) {
				if ( self->hasAttachment(descriptor.name) ) {
					auto& attachment = self->getAttachment(descriptor.name);
					image = attachment.getImage(frame);
					mips = attachment.descriptor.mips;
					initialLayout = attachment.descriptor.layout;
				}
			}
			if ( image == VK_NULL_HANDLE ) continue;

			if ( transitioned.count(image) > 0 ) continue;
			transitioned.insert(image);

			bool isDepth = descriptor.name.starts_with("depth");
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.aspectMask = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
			VkImageLayout newLayout = layout;
			if ( isDepth && layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ) {
				newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			}
			uf::renderer::Texture::setImageLayout( commandBuffer, image, descriptor.layout, newLayout, subresourceRange );
			if ( mips > 1 ) {
				subresourceRange.baseMipLevel = 1;
				subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
				uf::renderer::Texture::setImageLayout( commandBuffer, image, descriptor.layout, initialLayout, subresourceRange );
			}
		}
	}
}