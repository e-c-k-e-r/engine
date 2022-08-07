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
		subresourceRange.layerCount = 1;
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.layerCount = self->metadata.eyes;

		for ( auto& descriptor : shader.metadata.aliases.attachments ) {
			if ( descriptor.layout == VK_IMAGE_LAYOUT_UNDEFINED ) continue;
			VkImage image = VK_NULL_HANDLE;
			if ( descriptor.renderMode ) {
				if ( descriptor.renderMode->hasAttachment(descriptor.name) )
					image = descriptor.renderMode->getAttachment(descriptor.name).image;

			} else if ( self->hasAttachment(descriptor.name) ) {
				if ( self->hasAttachment(descriptor.name) )
					image = self->getAttachment(descriptor.name).image;
			}
			if ( image == VK_NULL_HANDLE ) continue;
			subresourceRange.aspectMask = descriptor.name == "depth" ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
			uf::renderer::Texture::setImageLayout( commandBuffer, image, layout, descriptor.layout, subresourceRange );
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
		subresourceRange.layerCount = 1;
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.layerCount = self->metadata.eyes;

		for ( auto& descriptor : shader.metadata.aliases.attachments ) {
			if ( descriptor.layout == VK_IMAGE_LAYOUT_UNDEFINED ) continue;
			VkImage image = VK_NULL_HANDLE;
			if ( descriptor.renderMode ) {
				if ( descriptor.renderMode->hasAttachment(descriptor.name) )
					image = descriptor.renderMode->getAttachment(descriptor.name).image;

			} else if ( self->hasAttachment(descriptor.name) ) {
				if ( self->hasAttachment(descriptor.name) )
					image = self->getAttachment(descriptor.name).image;
			}
			if ( image == VK_NULL_HANDLE ) continue;
			subresourceRange.aspectMask = descriptor.name == "depth" ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
			uf::renderer::Texture::setImageLayout( commandBuffer, image, descriptor.layout, layout, subresourceRange );
		}
	}
}