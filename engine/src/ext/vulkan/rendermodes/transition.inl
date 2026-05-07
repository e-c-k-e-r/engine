namespace {
	void transitionAttachmentsTo(
		ext::vulkan::RenderMode* self,
		ext::vulkan::Shader& shader,
		VkCommandBuffer commandBuffer,
		VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	) {
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
					image = attachment.image;
					mips = attachment.descriptor.mips;
					initialLayout = attachment.descriptor.layout;
				}

			} else if ( self->hasAttachment(descriptor.name) ) {
				if ( self->hasAttachment(descriptor.name) ) {
					auto& attachment = self->getAttachment(descriptor.name);
					image = attachment.image;
					mips = attachment.descriptor.mips;
					initialLayout = attachment.descriptor.layout;
				}
			}
			if ( image == VK_NULL_HANDLE ) continue;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.aspectMask = descriptor.name == "depth" ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
			uf::renderer::Texture::setImageLayout( commandBuffer, image, layout, descriptor.layout, subresourceRange );
			if ( mips > 1 ) {
				subresourceRange.baseMipLevel = 1;
				subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
				uf::renderer::Texture::setImageLayout( commandBuffer, image, initialLayout, descriptor.layout, subresourceRange );
			}
		}
	}
	void transitionAttachmentsFrom(
		ext::vulkan::RenderMode* self,
		ext::vulkan::Shader& shader,
		VkCommandBuffer commandBuffer,
		VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	) {
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
					image = attachment.image;
					mips = attachment.descriptor.mips;
					initialLayout = attachment.descriptor.layout;
				}

			} else if ( self->hasAttachment(descriptor.name) ) {
				if ( self->hasAttachment(descriptor.name) ) {
					auto& attachment = self->getAttachment(descriptor.name);
					image = attachment.image;
					mips = attachment.descriptor.mips;
					initialLayout = attachment.descriptor.layout;
				}
			}
			if ( image == VK_NULL_HANDLE ) continue;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.aspectMask = descriptor.name == "depth" ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
			uf::renderer::Texture::setImageLayout( commandBuffer, image, descriptor.layout, layout, subresourceRange );
			if ( mips > 1 ) {
				subresourceRange.baseMipLevel = 1;
				subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
				uf::renderer::Texture::setImageLayout( commandBuffer, image, descriptor.layout, initialLayout, subresourceRange );
			}
		}
	}
}