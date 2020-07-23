#include <uf/ext/vulkan/texture.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/image/image.h>
#include <uf/ext/vulkan/vulkan.h>


void ext::vulkan::Texture::initialize( Device& device, size_t width, size_t height ) {
	this->device = &device;
	this->width = width;
	this->height = height;
}
void ext::vulkan::Texture::updateDescriptors() {
	descriptor.sampler = sampler;
	descriptor.imageView = view;
	descriptor.imageLayout = imageLayout;
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
	if ( sampler != VK_NULL_HANDLE ) {
		vkDestroySampler(device->logicalDevice, sampler, nullptr);
		sampler = VK_NULL_HANDLE;
	}
	if ( deviceMemory != VK_NULL_HANDLE ) {
//		vkFreeMemory(device->logicalDevice, deviceMemory, nullptr);
		deviceMemory = VK_NULL_HANDLE;
	}
}
void ext::vulkan::Texture::setImageLayout(
	VkCommandBuffer cmdbuffer,
	VkImage image,
	VkImageLayout oldImageLayout,
	VkImageLayout newImageLayout,
	VkImageSubresourceRange subresourceRange,
	VkPipelineStageFlags srcStageMask,
	VkPipelineStageFlags dstStageMask
) {
	// Create an image barrier object
	VkImageMemoryBarrier imageMemoryBarrier = ext::vulkan::initializers::imageMemoryBarrier();
	imageMemoryBarrier.oldLayout = oldImageLayout;
	imageMemoryBarrier.newLayout = newImageLayout;
	imageMemoryBarrier.image = image;
	imageMemoryBarrier.subresourceRange = subresourceRange;

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
	VkPipelineStageFlags srcStageMask,
	VkPipelineStageFlags dstStageMask
) {
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = aspectMask;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.layerCount = 1;
	setImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
}
void ext::vulkan::Texture2D::loadFromFile(
	std::string filename, 
	VkFormat format,
	VkImageUsageFlags imageUsageFlags,
	VkImageLayout imageLayout, 
	bool forceLinear
) {
	return loadFromFile( filename, ext::vulkan::device, ext::vulkan::device.graphicsQueue, format, imageUsageFlags, imageLayout, forceLinear );
}
void ext::vulkan::Texture2D::loadFromFile(
	std::string filename, 
	Device& device,
	VkQueue copyQueue,
	VkFormat format,
	VkImageUsageFlags imageUsageFlags,
	VkImageLayout imageLayout, 
	bool forceLinear
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
		copyQueue,
		filter,
		imageUsageFlags,
		imageLayout
	);
}
void ext::vulkan::Texture2D::loadFromImage(
	uf::Image& image,
	VkFormat format,
	VkImageUsageFlags imageUsageFlags,
	VkImageLayout imageLayout, 
	bool forceLinear
) {
	return loadFromImage( image, ext::vulkan::device, ext::vulkan::device.graphicsQueue, format, imageUsageFlags, imageLayout, forceLinear );
}
void ext::vulkan::Texture2D::loadFromImage(
	uf::Image& image, 
	Device& device,
	VkQueue copyQueue,
	VkFormat format,
	VkImageUsageFlags imageUsageFlags,
	VkImageLayout imageLayout, 
	bool forceLinear
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
		device,
		copyQueue,
		VK_FILTER_LINEAR,
		imageUsageFlags,
		imageLayout
	);
}
void ext::vulkan::Texture2D::fromBuffers(
	void* data,
	VkDeviceSize bufferSize,
	VkFormat format,
	uint32_t texWidth,
	uint32_t texHeight,
	Device& device,
	VkQueue copyQueue,
	VkFilter filter,
	VkImageUsageFlags imageUsageFlags,
	VkImageLayout imageLayout
) {
	this->initialize(device, texWidth, texHeight);
	this->mips = 1;


	// Use a separate command buffer for texture loading
	VkCommandBuffer copyCmd = device.createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	//
	Buffer staging;

	VK_CHECK_RESULT(device.createBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		staging,
		bufferSize,
		data
	));


	VkBufferImageCopy bufferCopyRegion = {};
	bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	bufferCopyRegion.imageSubresource.mipLevel = 0;
	bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
	bufferCopyRegion.imageSubresource.layerCount = 1;
	bufferCopyRegion.imageExtent.width = width;
	bufferCopyRegion.imageExtent.height = height;
	bufferCopyRegion.imageExtent.depth = 1;
	bufferCopyRegion.bufferOffset = 0;

	// Create optimal tiled target image
	VkImageCreateInfo imageCreateInfo = ext::vulkan::initializers::imageCreateInfo();
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = format;
	imageCreateInfo.mipLevels = this->mips;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.extent = { width, height, 1 };
	imageCreateInfo.usage = imageUsageFlags;
	// Ensure that the TRANSFER_DST bit is set for staging
	if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
		imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	vmaCreateImage(allocator, &imageCreateInfo, &allocInfo, &image, &allocation, &allocationInfo);
	deviceMemory = allocationInfo.deviceMemory;
/*
	VK_CHECK_RESULT(vkCreateImage(device.logicalDevice, &imageCreateInfo, nullptr, &image));
	VkMemoryAllocateInfo memAlloc = staging.memAlloc;
	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(device.logicalDevice, image, &memReqs);
	memAlloc.allocationSize = staging.memReqs.size;
	memAlloc.memoryTypeIndex = device.getMemoryType(staging.memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK_RESULT(vkAllocateMemory(device.logicalDevice, &memAlloc, nullptr, &deviceMemory));
	VK_CHECK_RESULT(vkBindImageMemory(device.logicalDevice, image, deviceMemory, 0));
*/

	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = this->mips;
	subresourceRange.layerCount = 1;

	// Image barrier for optimal image (target)
	// Optimal image will be used as destination for the copy
	setImageLayout(
		copyCmd,
		image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		subresourceRange
	);
	// Copy mip levels from staging buffer
	vkCmdCopyBufferToImage(
		copyCmd,
		staging.buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&bufferCopyRegion
	);
	// Change texture image layout to shader read after all mip levels have been copied
	this->imageLayout = imageLayout;
	setImageLayout(
		copyCmd,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		imageLayout,
		subresourceRange
	);
	device.flushCommandBuffer(copyCmd, copyQueue);
	// Clean up staging resources
/*
	vkFreeMemory(device.logicalDevice, staging.memory, nullptr);
	vkDestroyBuffer(device.logicalDevice, staging.buffer, nullptr);
*/
	staging.destroy();
	
	// Create sampler
	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = filter;
	samplerCreateInfo.minFilter = filter;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.mipLodBias = 0.0f;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
	samplerCreateInfo.minLod = 0.0f;
	samplerCreateInfo.maxLod = 0.0f;
	samplerCreateInfo.maxAnisotropy = 1.0f;
	VK_CHECK_RESULT(vkCreateSampler(device.logicalDevice, &samplerCreateInfo, nullptr, &sampler));

	// Create image view
	VkImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.pNext = NULL;
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewCreateInfo.format = format;
	viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	viewCreateInfo.subresourceRange.levelCount = 1;
	viewCreateInfo.image = image;
	VK_CHECK_RESULT(vkCreateImageView(device.logicalDevice, &viewCreateInfo, nullptr, &view));
	// Update descriptor image info member that can be used for setting up descriptor sets
	this->updateDescriptors();
}

void ext::vulkan::Texture2D::asRenderTarget( Device& device, uint32_t width, uint32_t height, VkQueue copyQueue, VkFormat format ) {
	// Prepare blit target texture
	this->initialize( device, width, height );

	// Get device properties for the requested texture format
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(device.physicalDevice, format, &formatProperties);
	// Check if requested image format supports image storage operations
	assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT);

	VkImageCreateInfo imageCreateInfo = ext::vulkan::initializers::imageCreateInfo();
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = format;
	imageCreateInfo.extent = { width, height, 1 };
	imageCreateInfo.mipLevels = 1;
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
	vmaCreateImage(allocator, &imageCreateInfo, &allocInfo, &image, &allocation, &allocationInfo);
	deviceMemory = allocationInfo.deviceMemory;
/*
	VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, &image));
	vkGetImageMemoryRequirements(device, image, &memReqs);
	memAllocInfo.allocationSize = memReqs.size;
	memAllocInfo.memoryTypeIndex = device.getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &deviceMemory));
	VK_CHECK_RESULT(vkBindImageMemory(device, image, deviceMemory, 0));
*/
	VkCommandBuffer layoutCmd = device.createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	setImageLayout(
		layoutCmd, 
		image,
		VK_IMAGE_ASPECT_COLOR_BIT, 
		VK_IMAGE_LAYOUT_UNDEFINED,
		imageLayout
	);

	device.flushCommandBuffer(layoutCmd, copyQueue, true);

	// Create sampler
	VkSamplerCreateInfo samplerCreateInfo = ext::vulkan::initializers::samplerCreateInfo();
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
	samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;
	samplerCreateInfo.mipLodBias = 0.0f;
	samplerCreateInfo.maxAnisotropy = 1.0f;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
	samplerCreateInfo.minLod = 0.0f;
	samplerCreateInfo.maxLod = 0.0f;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	VK_CHECK_RESULT(vkCreateSampler(device, &samplerCreateInfo, nullptr, &sampler));

	// Create image view
	VkImageViewCreateInfo viewCreateInfo = ext::vulkan::initializers::imageViewCreateInfo();
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewCreateInfo.format = format;
	viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	viewCreateInfo.image = image;
	VK_CHECK_RESULT(vkCreateImageView(device, &viewCreateInfo, nullptr, &view));

	// Initialize a descriptor for later use
	this->updateDescriptors();
}