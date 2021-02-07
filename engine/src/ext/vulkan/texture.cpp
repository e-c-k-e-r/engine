#include <uf/ext/vulkan/texture.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/image/image.h>
#include <uf/ext/vulkan/vulkan.h>

ext::vulkan::Texture2D ext::vulkan::Texture2D::empty;

void ext::vulkan::Sampler::initialize( Device& device ) {
	this->device = &device;
	if ( device.enabledFeatures.samplerAnisotropy == VK_FALSE ) {
		descriptor.anisotropy.enabled = VK_FALSE;
	}
	{
		VkSamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.minFilter = descriptor.filter.min;
		samplerCreateInfo.magFilter = descriptor.filter.mag;
		samplerCreateInfo.borderColor = descriptor.filter.borderColor;
		samplerCreateInfo.addressModeU = descriptor.addressMode.u;
		samplerCreateInfo.addressModeV = descriptor.addressMode.v;
		samplerCreateInfo.addressModeW = descriptor.addressMode.w;
		samplerCreateInfo.unnormalizedCoordinates = descriptor.addressMode.unnormalizedCoordinates;

		samplerCreateInfo.mipmapMode = descriptor.mip.mode;
		samplerCreateInfo.mipLodBias = descriptor.mip.lodBias;
		samplerCreateInfo.compareEnable = descriptor.mip.compareEnable;
		samplerCreateInfo.compareOp = descriptor.mip.compareOp;
		samplerCreateInfo.minLod = descriptor.mip.min;
		samplerCreateInfo.maxLod = descriptor.mip.max;

		samplerCreateInfo.anisotropyEnable = descriptor.anisotropy.enabled;
		samplerCreateInfo.maxAnisotropy = descriptor.anisotropy.max;
		VK_CHECK_RESULT(vkCreateSampler(device.logicalDevice, &samplerCreateInfo, nullptr, &sampler));
	}
	{
		descriptor.info.sampler = sampler;
	}
}
void ext::vulkan::Sampler::destroy() {
	if ( sampler != VK_NULL_HANDLE ) {
		vkDestroySampler(device->logicalDevice, sampler, nullptr);
		sampler = VK_NULL_HANDLE;
	}
}

void ext::vulkan::Texture::initialize( Device& device, size_t width, size_t height, size_t depth, size_t layers ) {
	this->device = &device;
	this->width = width;
	this->height = height;
	this->depth = depth;
	this->layers = layers;
	// implicitly set type
	if ( width > 1 && height > 1 && depth > 1 ) {
		this->type = VK_IMAGE_TYPE_3D;
	} else if ( (width == 1 && height > 1 && depth > 1) || (width > 1 && height == 1 && depth > 1) || (width > 1 && height > 1 && depth == 1) ) {
		this->type = VK_IMAGE_TYPE_2D;
	} else if ( (width > 1 && height == 1 && depth == 1) || (width == 1 && height > 1 && depth == 1) || (width == 1 && height == 1 && depth > 1) ) {
		this->type = VK_IMAGE_TYPE_1D;
	}
	if ( depth > 1 ) {
		if ( viewType == VK_IMAGE_VIEW_TYPE_1D ) viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
		if ( viewType == VK_IMAGE_VIEW_TYPE_2D ) viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		if ( viewType == VK_IMAGE_VIEW_TYPE_CUBE ) viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
	}
}
void ext::vulkan::Texture::initialize( Device& device, VkImageViewType viewType, size_t width, size_t height, size_t depth, size_t layers ) {
	this->initialize( device, width, height, depth, layers );
	this->viewType = viewType;
	switch ( viewType ) {
		case VK_IMAGE_VIEW_TYPE_1D:
		case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
			type = VK_IMAGE_TYPE_1D;
		break;
		case VK_IMAGE_VIEW_TYPE_2D:
		case VK_IMAGE_VIEW_TYPE_CUBE:
		case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
		case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY: 
			type = VK_IMAGE_TYPE_2D;
		break;
		case VK_IMAGE_VIEW_TYPE_3D:
			type = VK_IMAGE_TYPE_3D;
		break;
	}
}
void ext::vulkan::Texture::updateDescriptors() {
	descriptor.sampler = sampler.sampler;
	descriptor.imageView = view;
	descriptor.imageLayout = imageLayout;
}
bool ext::vulkan::Texture::generated() const {
	return view != VK_NULL_HANDLE;
}
void ext::vulkan::Texture::destroy() {
	if ( !device ) return;

	if ( view != VK_NULL_HANDLE ) {
		vkDestroyImageView(device->logicalDevice, view, nullptr);
		view = VK_NULL_HANDLE;
	}
	if ( image != VK_NULL_HANDLE ) {
//		vkDestroyImage(device->logicalDevice, image, nullptr);
		vmaDestroyImage( allocator, image, allocation );
		image = VK_NULL_HANDLE;
	}
	if ( deviceMemory != VK_NULL_HANDLE ) {
//		vkFreeMemory(device->logicalDevice, deviceMemory, nullptr);
		deviceMemory = VK_NULL_HANDLE;
	}
	sampler.destroy();
}
void ext::vulkan::Texture::setImageLayout(
	VkCommandBuffer cmdbuffer,
	VkImage image,
	VkImageLayout oldImageLayout,
	VkImageLayout newImageLayout,
	VkImageSubresourceRange subresourceRange
) {
	// Create an image barrier object
	VkImageMemoryBarrier imageMemoryBarrier = ext::vulkan::initializers::imageMemoryBarrier();
	imageMemoryBarrier.oldLayout = oldImageLayout;
	imageMemoryBarrier.newLayout = newImageLayout;
	imageMemoryBarrier.image = image;
	imageMemoryBarrier.subresourceRange = subresourceRange;

	VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

	// Source layouts (old)
	// Source access mask controls actions that have to be finished on the old layout
	// before it will be transitioned to the new layout
	switch ( oldImageLayout ) {
		case VK_IMAGE_LAYOUT_UNDEFINED:
			// Image layout is undefined (or does not matter)
			// Only valid as initial layout
			// No flags required, listed only for completeness
			imageMemoryBarrier.srcAccessMask = 0;
		break;
		case VK_IMAGE_LAYOUT_PREINITIALIZED:
			// Image is preinitialized
			// Only valid as initial layout for linear images, preserves memory contents
			// Make sure host writes have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		break;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			// Image is a color attachment
			// Make sure any writes to the color buffer have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			// Image is a depth/stencil attachment
			// Make sure any writes to the depth/stencil buffer have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			// Image is a transfer source 
			// Make sure any reads from the image have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			// Image is a transfer destination
			// Make sure any writes to the image have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// Image is read by a shader
			// Make sure any shader reads from the image have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
		default:
			// Other source layouts aren't handled (yet)
		break;
	}

	// Target layouts (new)
	// Destination access mask controls the dependency for the new image layout
	switch ( newImageLayout ) {
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			// Image will be used as a transfer destination
			// Make sure any writes to the image have been finished
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			// Image will be used as a transfer source
			// Make sure any reads from the image have been finished
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			// Image will be used as a color attachment
			// Make sure any writes to the color buffer have been finished
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			// Image layout will be used as a depth/stencil attachment
			// Make sure any writes to depth/stencil buffer have been finished
			imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// Image will be read in a shader (sampler, input attachment)
			// Make sure any writes to the image have been finished
			if (imageMemoryBarrier.srcAccessMask == 0) {
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
			}
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
		default:
			// Other source layouts aren't handled (yet)
		break;
	}

	 if ( oldImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ) {
		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if ( oldImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	// Put barrier inside setup command buffer
	vkCmdPipelineBarrier(
		cmdbuffer,
		srcStageMask, dstStageMask,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier
	);
}
void ext::vulkan::Texture::setImageLayout(
	VkCommandBuffer cmdbuffer,
	VkImage image,
	VkImageAspectFlags aspectMask,
	VkImageLayout oldImageLayout,
	VkImageLayout newImageLayout,
	uint32_t mipLevels
) {
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = aspectMask;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = mipLevels;
	subresourceRange.layerCount = 1;
	setImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange);
}
void ext::vulkan::Texture::loadFromFile(
	std::string filename, 
	VkFormat format,
	VkImageUsageFlags imageUsageFlags,
	VkImageLayout imageLayout
) {
	return loadFromFile( filename, ext::vulkan::device, format, imageUsageFlags, imageLayout );
}
void ext::vulkan::Texture::loadFromFile(
	std::string filename, 
	Device& device,
	VkFormat format,
	VkImageUsageFlags imageUsageFlags,
	VkImageLayout imageLayout 
) {
	uf::Image image;
	image.open( filename );
	switch ( image.getChannels() ) {
		// R
		case 1:
			switch ( image.getBpp() ) {
				case 8:
					format = VK_FORMAT_R8_UNORM;
				break;
				default:
					throw std::runtime_error("unsupported BPP of " + std::to_string(image.getBpp()));
				break;
			}
		break;
		// RGB
		case 3:
			switch ( image.getBpp() ) {
				case 24:
					format = VK_FORMAT_R8G8B8_UNORM;
				break;
				default:
					throw std::runtime_error("unsupported BPP of " + std::to_string(image.getBpp()));
				break;
			}
		break;
		// RGBA
		case 4:
			switch ( image.getBpp() ) {
				case 32:
					format = VK_FORMAT_R8G8B8A8_UNORM;
				break;
				default:
					throw std::runtime_error("unsupported BPP of " + std::to_string(image.getBpp()));
				break;
			}
		break;
		default:
			throw std::runtime_error("unsupported channels of " + std::to_string(image.getChannels()));
		break;
	}
/*
	if (device.features.textureCompressionBC) {
		format = VK_FORMAT_BC3_UNORM_BLOCK;
	}
	else if (device.features.textureCompressionASTC_LDR) {
		format = VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
	}
	else if (device.features.textureCompressionETC2) {
		format = VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
	}
	else {
		throw std::runtime_error("VK_ERROR_FEATURE_NOT_PRESENT");
	}
*/

	// convert to power of two
	//image.padToPowerOfTwo();

	this->fromBuffers( 
		(void*) image.getPixelsPtr(),
		image.getPixels().size(),
		format,
		image.getDimensions()[0],
		image.getDimensions()[1],
		device,
		imageUsageFlags,
		imageLayout
	);
}
void ext::vulkan::Texture::loadFromImage(
	uf::Image& image,
	VkFormat format,
	VkImageUsageFlags imageUsageFlags,
	VkImageLayout imageLayout
) {
	return loadFromImage( image, ext::vulkan::device, format, imageUsageFlags, imageLayout );
}
void ext::vulkan::Texture::loadFromImage(
	uf::Image& image, 
	Device& device,
	VkFormat format,
	VkImageUsageFlags imageUsageFlags,
	VkImageLayout imageLayout
) {
	switch ( image.getChannels() ) {
		// R
		case 1:
			switch ( image.getBpp() ) {
				case 8:
					format = VK_FORMAT_R8_UNORM;
				break;
				default:
					throw std::runtime_error("unsupported BPP of " + std::to_string(image.getBpp()));
				break;
			}
		break;
		// RGB
		case 3:
			switch ( image.getBpp() ) {
				case 24:
					format = VK_FORMAT_R8G8B8_UNORM;
				break;
				default:
					throw std::runtime_error("unsupported BPP of " + std::to_string(image.getBpp()));
				break;
			}
		break;
		// RGBA
		case 4:
			switch ( image.getBpp() ) {
				case 32:
					format = VK_FORMAT_R8G8B8A8_UNORM;
				break;
				default:
					throw std::runtime_error("unsupported BPP of " + std::to_string(image.getBpp()));
				break;
			}
		break;
		default:
			throw std::runtime_error("unsupported channels of " + std::to_string(image.getChannels()));
		break;
	}

	// convert to power of two
	//image.padToPowerOfTwo();

	this->fromBuffers( 
		(void*) image.getPixelsPtr(),
		image.getPixels().size(),
		format,
		image.getDimensions()[0],
		image.getDimensions()[1],
		1,
		1,
		device,
		imageUsageFlags,
		imageLayout
	);
}

void ext::vulkan::Texture::fromBuffers(
	void* data,
	VkDeviceSize bufferSize,
	VkFormat format,
	uint32_t texWidth,
	uint32_t texHeight,
	uint32_t texDepth,
	uint32_t layers,
	Device& device,
	VkImageUsageFlags imageUsageFlags,
	VkImageLayout imageLayout
) {
	this->initialize(device, texWidth, texHeight, texDepth, layers);

	if ( this->mips == 0 ) {
		this->mips = 1;
	} else if ( this->depth == 1 ) {
		this->mips = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(device.physicalDevice, format, &formatProperties);
		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
			this->mips = 1;
			VK_VALIDATION_MESSAGE("Texture image format does not support linear blitting!");
		}
	}

	// Create optimal tiled target image
	VkImageCreateInfo imageCreateInfo = ext::vulkan::initializers::imageCreateInfo();
	imageCreateInfo.imageType = this->type;
	imageCreateInfo.format = this->format = format;
	imageCreateInfo.mipLevels = this->mips;
	imageCreateInfo.arrayLayers = this->layers;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.extent = { width, height, depth };
	imageCreateInfo.usage = imageUsageFlags;
	// Ensure that the TRANSFER_SRC bit is set for mip creation
	if ( this->mips > 1 && !(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)) {
		imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}
	// Ensure that the TRANSFER_DST bit is set for staging
	if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
		imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}
	// Ensure cube maps get the compat flag bit
	if ( this->viewType == VK_IMAGE_VIEW_TYPE_CUBE || this->viewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY ) {
		imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	VK_CHECK_RESULT(vmaCreateImage(allocator, &imageCreateInfo, &allocInfo, &image, &allocation, &allocationInfo));
	deviceMemory = allocationInfo.deviceMemory;

	// Create sampler
	sampler.descriptor.mip.min = 0;
	sampler.descriptor.mip.max = static_cast<float>(this->mips);
	sampler.initialize( device );

	// Create image view	
	VkImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.pNext = NULL;
	viewCreateInfo.viewType = viewType;
	viewCreateInfo.format = format;
	viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewCreateInfo.subresourceRange.baseMipLevel = 0;
	viewCreateInfo.subresourceRange.layerCount = this->layers;
	viewCreateInfo.subresourceRange.levelCount = this->mips;
	viewCreateInfo.image = image;
	VK_CHECK_RESULT(vkCreateImageView(device.logicalDevice, &viewCreateInfo, nullptr, &view));

	if ( data ) {
		uint8_t* layerPointer = (uint8_t*) data;
		VkDeviceSize layerSize = bufferSize / this->layers;
		for ( size_t layer = 1; layer <= this->layers; ++layer ) {
			this->update( (void*) layerPointer, layerSize, imageLayout, layer );
			layerPointer += layerSize;
		}
	}
	
	this->updateDescriptors();
}

void ext::vulkan::Texture::asRenderTarget( Device& device, uint32_t width, uint32_t height, VkFormat format ) {
	// Prepare blit target texture
	this->initialize( device, width, height );
	
	// Get device properties for the requested texture format
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(device.physicalDevice, format, &formatProperties);
	// Check if requested image format supports image storage operations
	assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT);

	VkImageCreateInfo imageCreateInfo = ext::vulkan::initializers::imageCreateInfo();
	imageCreateInfo.imageType = type;
	imageCreateInfo.format = format;
	imageCreateInfo.extent = { width, height, depth };
	imageCreateInfo.mipLevels = this->mips = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	// Image will be sampled in the fragment shader and used as storage target in the compute shader
	imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	imageCreateInfo.flags = 0;

	VkMemoryAllocateInfo memAllocInfo = ext::vulkan::initializers::memoryAllocateInfo();
	VkMemoryRequirements memReqs;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	VK_CHECK_RESULT(vmaCreateImage(allocator, &imageCreateInfo, &allocInfo, &image, &allocation, &allocationInfo));
	deviceMemory = allocationInfo.deviceMemory;

	VkCommandBuffer layoutCmd = device.createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	setImageLayout(
		layoutCmd, 
		image,
		VK_IMAGE_ASPECT_COLOR_BIT, 
		VK_IMAGE_LAYOUT_UNDEFINED,
		imageLayout,
		this->mips
	);

	device.flushCommandBuffer(layoutCmd, true);

	// Create sampler
	sampler.initialize( device );

	// Create image view
	VkImageViewCreateInfo viewCreateInfo = ext::vulkan::initializers::imageViewCreateInfo();
	viewCreateInfo.viewType = viewType;
	viewCreateInfo.format = format;
	viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	viewCreateInfo.image = image;
	VK_CHECK_RESULT(vkCreateImageView(device, &viewCreateInfo, nullptr, &view));

	// Initialize a descriptor for later use
	this->updateDescriptors();
}
void ext::vulkan::Texture::aliasTexture( const Texture& texture ) {
	image = texture.image;
	view = texture.view;
	imageLayout = texture.imageLayout;
	deviceMemory = texture.deviceMemory;
	sampler = texture.sampler;
	
	this->updateDescriptors();
}
void ext::vulkan::Texture::aliasAttachment( const RenderTarget::Attachment& attachment, bool createSampler ) {
	image = attachment.image;
	view = attachment.view;
	imageLayout = attachment.descriptor.layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : attachment.descriptor.layout;
	deviceMemory = attachment.mem;

	// Create sampler
	if ( createSampler ) sampler.initialize( ext::vulkan::device );
	
	this->updateDescriptors();
}

void ext::vulkan::Texture::update( uf::Image& image, VkImageLayout targetImageLayout, uint32_t layer ) {
	if ( width != image.getDimensions()[0] || height != image.getDimensions()[1] ) return;
	return this->update( (void*) image.getPixelsPtr(), image.getPixels().size(), layer );
}
void ext::vulkan::Texture::update( void* data, VkDeviceSize bufferSize, VkImageLayout targetImageLayout, uint32_t layer ) {
		// Update descriptor image info member that can be used for setting up descriptor sets

	auto& device = *this->device;

	if ( targetImageLayout == VK_IMAGE_LAYOUT_UNDEFINED ) {
		targetImageLayout = this->imageLayout;
	}

	// Create image view
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = this->mips;
	subresourceRange.layerCount = 1;

	VkBufferImageCopy bufferCopyRegion = {};
	bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	bufferCopyRegion.imageSubresource.mipLevel = 0;
	bufferCopyRegion.imageSubresource.baseArrayLayer = layer - 1;
	bufferCopyRegion.imageSubresource.layerCount = 1;
	bufferCopyRegion.imageExtent.width = width;
	bufferCopyRegion.imageExtent.height = height;
	bufferCopyRegion.imageExtent.depth = depth;
	bufferCopyRegion.bufferOffset = 0;
	//
	Buffer staging;
	VK_CHECK_RESULT(device.createBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		staging,
		bufferSize,
		data
	));

	// Use a separate command buffer for texture loading
	VkCommandBuffer commandBuffer = device.createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	// Image barrier for optimal image (target)
	// Optimal image will be used as destination for the copy
	setImageLayout(
		commandBuffer,
		image,
		imageLayout,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		subresourceRange
	);

	// Copy mip levels from staging buffer
	vkCmdCopyBufferToImage(
		commandBuffer,
		staging.buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&bufferCopyRegion
	);

	if ( this->mips > 1 ) this->generateMipmaps(commandBuffer, layer);
	
	setImageLayout(
		commandBuffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		targetImageLayout,
		subresourceRange
	);
	device.flushCommandBuffer(commandBuffer);
	// Clean up staging resources
	staging.destroy();
	
	this->imageLayout = targetImageLayout;

	this->updateDescriptors();
}

void ext::vulkan::Texture::generateMipmaps( VkCommandBuffer commandBuffer, uint32_t layer ) {
	auto& device = *this->device;

	if ( this->mips <= 1 ) return;

	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(device.physicalDevice, format, &formatProperties);
	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		throw std::runtime_error("texture image format does not support linear blitting!");
	}

	// base layer barrier
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = layer - 1;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = width;
	int32_t mipHeight = height;
	int32_t mipDepth = depth;
	for ( size_t i = 1; i < this->mips; ++i ) {
		// transition previous layer to read from it
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		// blit to current mip layer
		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, mipDepth };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = layer - 1;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, mipDepth > 1 ? mipDepth / 2 : 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = layer - 1;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(
			commandBuffer,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR
		);

		// transition previous layer back
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		if (mipWidth > 1) mipWidth /= 2;
    	if (mipHeight > 1) mipHeight /= 2;
    	if (mipDepth > 1) mipDepth /= 2;
	}
}

ext::vulkan::Texture2D::Texture2D() {
	type = VK_IMAGE_TYPE_2D;
	viewType = VK_IMAGE_VIEW_TYPE_2D;
}
ext::vulkan::Texture3D::Texture3D() {
	type = VK_IMAGE_TYPE_3D;
	viewType = VK_IMAGE_VIEW_TYPE_3D;
}
ext::vulkan::TextureCube::TextureCube() {
	type = VK_IMAGE_TYPE_2D;
	viewType = VK_IMAGE_VIEW_TYPE_CUBE;
}