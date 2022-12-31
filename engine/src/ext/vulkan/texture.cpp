#if UF_USE_VULKAN
#include <bitset>
#include <uf/ext/vulkan/texture.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/image/image.h>
#include <uf/ext/vulkan/vulkan.h>

ext::vulkan::Texture2D ext::vulkan::Texture2D::empty;
ext::vulkan::Texture3D ext::vulkan::Texture3D::empty;
ext::vulkan::TextureCube ext::vulkan::TextureCube::empty;

VkFormat ext::vulkan::Texture::DefaultFormat = VK_FORMAT_R8G8B8A8_UNORM;

uf::stl::vector<ext::vulkan::Sampler> ext::vulkan::Sampler::samplers;

namespace {
	void enforceFilterFromFormat( ext::vulkan::Sampler::Descriptor& descriptor, VkFormat format ) {
		switch ( format ) {
			case VK_FORMAT_R8_UINT:
			case VK_FORMAT_R8_SINT:
			case VK_FORMAT_R8G8_UINT:
			case VK_FORMAT_R8G8_SINT:
			case VK_FORMAT_R8G8B8_UINT:
			case VK_FORMAT_R8G8B8_SINT:
			case VK_FORMAT_B8G8R8_UINT:
			case VK_FORMAT_B8G8R8_SINT:
			case VK_FORMAT_R8G8B8A8_UINT:
			case VK_FORMAT_R8G8B8A8_SINT:
			case VK_FORMAT_B8G8R8A8_UINT:
			case VK_FORMAT_B8G8R8A8_SINT:
			case VK_FORMAT_R16_UINT:
			case VK_FORMAT_R16_SINT:
			case VK_FORMAT_R16G16_UINT:
			case VK_FORMAT_R16G16_SINT:
			case VK_FORMAT_R16G16B16_UINT:
			case VK_FORMAT_R16G16B16_SINT:
			case VK_FORMAT_R16G16B16A16_UINT:
			case VK_FORMAT_R16G16B16A16_SINT:
			case VK_FORMAT_R32_UINT:
			case VK_FORMAT_R32_SINT:
			case VK_FORMAT_R32G32_UINT:
			case VK_FORMAT_R32G32_SINT:
			case VK_FORMAT_R32G32B32_UINT:
			case VK_FORMAT_R32G32B32_SINT:
			case VK_FORMAT_R32G32B32A32_UINT:
			case VK_FORMAT_R32G32B32A32_SINT:
			case VK_FORMAT_R64_UINT:
			case VK_FORMAT_R64_SINT:
			case VK_FORMAT_R64G64_UINT:
			case VK_FORMAT_R64G64_SINT:
			case VK_FORMAT_R64G64B64_UINT:
			case VK_FORMAT_R64G64B64_SINT:
			case VK_FORMAT_R64G64B64A64_UINT:
			case VK_FORMAT_R64G64B64A64_SINT:
			case VK_FORMAT_S8_UINT:
			case VK_FORMAT_D16_UNORM_S8_UINT:
			case VK_FORMAT_D24_UNORM_S8_UINT:
			case VK_FORMAT_D32_SFLOAT_S8_UINT:
				descriptor.filter.min = VK_FILTER_NEAREST;
				descriptor.filter.mag = VK_FILTER_NEAREST;
			break;
		}
	}
}

ext::vulkan::Sampler ext::vulkan::Sampler::retrieve( const ext::vulkan::Sampler::Descriptor& info ) {
	ext::vulkan::Sampler sampler;
	for ( auto& s : samplers ) {
		if ( memcmp( &info, &s.descriptor, sizeof(info) - sizeof(VkDescriptorImageInfo) ) != 0 ) continue;
		sampler = s;
		goto RETRIEVE_RETURN;
	/*
		auto& d = s.descriptor;
		if ( d.filter.min != info.filter.min || d.filter.mag != info.filter.mag || d.filter.borderColor != info.filter.borderColor ) continue;
		if ( d.addressMode.unnormalizedCoordinates != info.addressMode.unnormalizedCoordinates || d.addressMode.u != info.addressMode.u || d.addressMode.v != info.addressMode.v || d.addressMode.w != info.addressMode.w ) continue;
		if ( d.mip.compareEnable != info.mip.compareEnable || d.mip.compareOp != info.mip.compareOp || d.mip.mode != info.mip.mode || d.mip.lodBias != info.mip.lodBias || d.mip.min != info.mip.min || d.mip.max != info.mip.max ) continue;
	*/
	}
	{
		auto& s = samplers.emplace_back();
		s.descriptor = info;
		s.initialize( ext::vulkan::device );
		sampler = s;
	}
RETRIEVE_RETURN:
	sampler.device = NULL;
	return sampler;
}

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

		VkSamplerReductionModeCreateInfoEXT createInfoReduction{};
		if ( descriptor.reduction.enabled ) {
			createInfoReduction.sType = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO_EXT;
			createInfoReduction.reductionMode = descriptor.reduction.mode;
			samplerCreateInfo.pNext = &createInfoReduction;
		}

		VK_CHECK_RESULT(vkCreateSampler(device.logicalDevice, &samplerCreateInfo, nullptr, &sampler));
	}
	{
		descriptor.info.sampler = sampler;
	}
}
void ext::vulkan::Sampler::destroy() {
	if ( !device ) return;

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
	if ( width > 1 && height == 1 && depth == 1 ) {
		this->type = ext::vulkan::enums::Image::TYPE_1D;
		this->viewType = layers == 1 ? ext::vulkan::enums::Image::VIEW_TYPE_1D : ext::vulkan::enums::Image::VIEW_TYPE_1D_ARRAY;
	} else if ( height > 1 && depth == 1 ) {
		this->type = ext::vulkan::enums::Image::TYPE_2D;
		this->viewType = layers == 1 ? ext::vulkan::enums::Image::VIEW_TYPE_2D : ext::vulkan::enums::Image::VIEW_TYPE_2D_ARRAY;
		if ( layers == 6 ) this->viewType = ext::vulkan::enums::Image::VIEW_TYPE_CUBE;
	} else {
		this->type = ext::vulkan::enums::Image::TYPE_3D;
		this->viewType = layers == 6 ? ext::vulkan::enums::Image::VIEW_TYPE_CUBE_ARRAY : ext::vulkan::enums::Image::VIEW_TYPE_3D;
	}
/*
	if ( width > 1 && height > 1 && depth > 1 ) {
		this->type = ext::vulkan::enums::Image::TYPE_3D;
	} else if ( (width == 1 && height > 1 && depth > 1) || (width > 1 && height == 1 && depth > 1) || (width > 1 && height > 1 && depth == 1) ) {
		this->type = ext::vulkan::enums::Image::TYPE_2D;
	} else if ( (width > 1 && height == 1 && depth == 1) || (width == 1 && height > 1 && depth == 1) || (width == 1 && height == 1 && depth > 1) ) {
		this->type = ext::vulkan::enums::Image::TYPE_1D;
	}
*/
/*
	if ( layers > 1 ) {
		if ( viewType == ext::vulkan::enums::Image::VIEW_TYPE_1D ) viewType = ext::vulkan::enums::Image::VIEW_TYPE_1D_ARRAY;
		if ( viewType == ext::vulkan::enums::Image::VIEW_TYPE_2D ) viewType = ext::vulkan::enums::Image::VIEW_TYPE_2D_ARRAY;
		if ( viewType == ext::vulkan::enums::Image::VIEW_TYPE_CUBE ) viewType = ext::vulkan::enums::Image::VIEW_TYPE_CUBE_ARRAY;
	}
*/
}
void ext::vulkan::Texture::initialize( Device& device, VkImageViewType viewType, size_t width, size_t height, size_t depth, size_t layers ) {
	this->initialize( device, width, height, depth, layers );
	this->viewType = viewType;
	switch ( viewType ) {
		case ext::vulkan::enums::Image::VIEW_TYPE_1D:
		case ext::vulkan::enums::Image::VIEW_TYPE_1D_ARRAY:
			type = ext::vulkan::enums::Image::TYPE_1D;
		break;
		case ext::vulkan::enums::Image::VIEW_TYPE_2D:
		case ext::vulkan::enums::Image::VIEW_TYPE_CUBE:
		case ext::vulkan::enums::Image::VIEW_TYPE_2D_ARRAY:
		case ext::vulkan::enums::Image::VIEW_TYPE_CUBE_ARRAY: 
			type = ext::vulkan::enums::Image::TYPE_2D;
		break;
		case ext::vulkan::enums::Image::VIEW_TYPE_3D:
			type = ext::vulkan::enums::Image::TYPE_3D;
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
	if ( !device || !device->logicalDevice || aliased ) return; // device->logicalDevice should never be null, but it happens, somehow

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
	if ( sampler.device ) sampler.destroy();
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
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
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
	const uf::stl::string& filename, 
	VkFormat format,
	VkImageUsageFlags imageUsageFlags,
	VkImageLayout imageLayout
) {
	return loadFromFile( filename, ext::vulkan::device, format, imageUsageFlags, imageLayout );
}
void ext::vulkan::Texture::loadFromFile(
	const uf::stl::string& filename, 
	Device& device,
	VkFormat format,
	VkImageUsageFlags imageUsageFlags,
	VkImageLayout imageLayout 
) {
	uf::Image image;
	image.open( filename );
	return loadFromImage( image, device, format, imageUsageFlags, imageLayout );
}
void ext::vulkan::Texture::loadFromImage(
	const uf::Image& image,
	VkFormat format,
	VkImageUsageFlags imageUsageFlags,
	VkImageLayout imageLayout
) {
	return loadFromImage( image, ext::vulkan::device, format, imageUsageFlags, imageLayout );
}
void ext::vulkan::Texture::loadFromImage(
	const uf::Image& image, 
	Device& device,
	VkFormat format,
	VkImageUsageFlags imageUsageFlags,
	VkImageLayout imageLayout
) {
/*
	switch ( format ) {
		case enums::Format::R8_SRGB:
		case enums::Format::R8G8_SRGB:
		case enums::Format::R8G8B8_SRGB:
		case enums::Format::R8G8B8A8_SRGB:
			srgb = true;
		break;
	}
*/
	switch ( image.getChannels() ) {
		// R
		case 1:
			switch ( image.getBpp() ) {
				case 8:
					format = srgb ? VK_FORMAT_R8_SRGB : VK_FORMAT_R8_UNORM;
				break;
				default:
					UF_EXCEPTION("Vulkan error: unsupported BPP of {}", image.getBpp() );
				break;
			}
		break;
		// RG
		case 2:
			switch ( image.getBpp() ) {
				case 16:
					format = srgb ? VK_FORMAT_R8G8_SRGB : VK_FORMAT_R8G8_UNORM;
				break;
				default:
					UF_EXCEPTION("Vulkan error: unsupported BPP of {}", image.getBpp() );
				break;
			}
		break;
		// RGB
		case 3:
			switch ( image.getBpp() ) {
				case 24:
					format = srgb ? VK_FORMAT_R8G8B8_SRGB : VK_FORMAT_R8G8B8_UNORM;
				break;
				default:
					UF_EXCEPTION("Vulkan error: unsupported BPP of {}", image.getBpp() );
				break;
			}
		break;
		// RGBA
		case 4:
			switch ( image.getBpp() ) {
				case 32:
					format = srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
				break;
				default:
					UF_EXCEPTION("Vulkan error: unsupported BPP of {}", image.getBpp() );
				break;
			}
		break;
		default:
			UF_EXCEPTION("Vulkan error: unsupported channels of {}", image.getChannels() );
		break;
	}

	// convert to power of two
	//image.padToPowerOfTwo();

	this->fromBuffers( 
		(void*) image.getPixelsPtr(),
		image.getPixels().size(),
		format,
		image.getDimensions().x,
		image.getDimensions().y,
		1,
		1,
		device,
		imageUsageFlags,
		imageLayout
	);
}

void ext::vulkan::Texture::fromBuffers(
	void* buffer,
	VkDeviceSize bufferSize,
	VkFormat format,
	uint32_t texWidth,
	uint32_t texHeight,
	uint32_t texDepth,
	uint32_t layers,
	VkImageUsageFlags imageUsageFlags,
	VkImageLayout imageLayout
) {
	return this->fromBuffers( buffer, bufferSize, format, texWidth, texHeight, texDepth, layers, ext::vulkan::device, imageUsageFlags, imageLayout );
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
	this->imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	if ( this->mips == 0 ) {
		this->mips = 1;
//	} else if ( this->depth == 1 ) {
	} else {
	//	this->mips = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
		this->mips = uf::vector::mips( pod::Vector3ui{ texWidth, texHeight, texDepth } );
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(device.physicalDevice, format, &formatProperties);
		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
			this->mips = 1;
			VK_VALIDATION_MESSAGE("Texture image format {} does not support linear blitting", format);
		}
	}

	uf::stl::vector<uint32_t> queueFamilyIndices = {};
	if ( std::find( queueFamilyIndices.begin(), queueFamilyIndices.end(), device.queueFamilyIndices.graphics ) == queueFamilyIndices.end() ) queueFamilyIndices.emplace_back(device.queueFamilyIndices.graphics);
	if ( std::find( queueFamilyIndices.begin(), queueFamilyIndices.end(), device.queueFamilyIndices.present ) == queueFamilyIndices.end() ) queueFamilyIndices.emplace_back(device.queueFamilyIndices.present);
	if ( std::find( queueFamilyIndices.begin(), queueFamilyIndices.end(), device.queueFamilyIndices.compute ) == queueFamilyIndices.end() ) queueFamilyIndices.emplace_back(device.queueFamilyIndices.compute);
	if ( std::find( queueFamilyIndices.begin(), queueFamilyIndices.end(), device.queueFamilyIndices.transfer ) == queueFamilyIndices.end() ) queueFamilyIndices.emplace_back(device.queueFamilyIndices.transfer);


	bool exclusive = true; // device.queueFamilyIndices.graphics == 0 && device.queueFamilyIndices.present == 0 && device.queueFamilyIndices.compute == 0 && device.queueFamilyIndices.transfer == 0;
	// Create optimal tiled target image
	VkImageCreateInfo imageCreateInfo = ext::vulkan::initializers::imageCreateInfo();
	imageCreateInfo.imageType = this->type;
	imageCreateInfo.format = this->format = format;
	imageCreateInfo.mipLevels = this->mips;
	imageCreateInfo.arrayLayers = this->layers;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.queueFamilyIndexCount = queueFamilyIndices.size();
	imageCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
//	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.sharingMode = exclusive ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
	imageCreateInfo.initialLayout = this->imageLayout;
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
	if ( this->viewType == ext::vulkan::enums::Image::VIEW_TYPE_CUBE || this->viewType == ext::vulkan::enums::Image::VIEW_TYPE_CUBE_ARRAY ) {
		imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	VK_CHECK_RESULT(vmaCreateImage(allocator, &imageCreateInfo, &allocInfo, &image, &allocation, &allocationInfo));
	deviceMemory = allocationInfo.deviceMemory;

	// Create sampler
	sampler.descriptor.mip.min = 0;
	sampler.descriptor.mip.max = static_cast<float>(this->mips);
	// sampler.initialize( device );
	::enforceFilterFromFormat( sampler.descriptor, format );
	sampler = ext::vulkan::Sampler::retrieve( sampler.descriptor );

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

	{
		auto commandBuffer = device.fetchCommandBuffer(uf::renderer::QueueEnum::GRAPHICS, true);
		device.UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "setImageLayout" );
		uf::renderer::Texture::setImageLayout( commandBuffer, image, VK_IMAGE_LAYOUT_UNDEFINED, (this->imageLayout = imageLayout), viewCreateInfo.subresourceRange );
		device.flushCommandBuffer(commandBuffer);
	}

	if ( data && bufferSize ) {
		uint8_t* layerPointer = (uint8_t*) data;
		VkDeviceSize layerSize = bufferSize / this->layers;
		for ( size_t layer = 0; layer < this->layers; ++layer ) {
			this->update( (void*) layerPointer, layerSize, imageLayout, layer );
			layerPointer += layerSize;
		}
	}
/*
	else {
		// Create image view
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.baseArrayLayer = 0;
		subresourceRange.levelCount = this->mips;
		subresourceRange.layerCount = this->layers;

		auto commandBuffer = device.fetchCommandBuffer(QueueEnum::GRAPHICS);
		setImageLayout(
			commandBuffer,
			image,
			this->imageLayout,
			imageLayout,
			subresourceRange
		);
		device.flushCommandBuffer(commandBuffer, QueueEnum::GRAPHICS, true);

		this->imageLayout = imageLayout;
	}
*/

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

	auto commandBuffer = device.fetchCommandBuffer(QueueEnum::GRAPHICS);

	imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	setImageLayout(
		commandBuffer, 
		image,
		VK_IMAGE_ASPECT_COLOR_BIT, 
		VK_IMAGE_LAYOUT_UNDEFINED,
		imageLayout,
		this->mips
	);

	device.flushCommandBuffer(commandBuffer);

	// Create sampler
	// sampler.initialize( device );
	::enforceFilterFromFormat( sampler.descriptor, format );
	sampler = ext::vulkan::Sampler::retrieve( sampler.descriptor );

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
ext::vulkan::Texture ext::vulkan::Texture::alias() const {
	ext::vulkan::Texture texture;
	texture.aliasTexture(*this);
	return texture;
}
void ext::vulkan::Texture::aliasTexture( const Texture& texture ) {
	*this = {
		.device = texture.device,
		.aliased = true,

		.image = texture.image,
		.view = texture.view,
		.type = texture.type,
	 	.viewType = texture.viewType,
		.imageLayout = texture.imageLayout,
		.deviceMemory = texture.deviceMemory, 
		.descriptor = texture.descriptor, 
		.format = texture.format,
		
		.sampler = texture.sampler,

		.allocation = texture.allocation,
		.allocationInfo = texture.allocationInfo,

		.width = texture.width,
		.height = texture.height,
		.depth = texture.depth,
		.layers = texture.layers,
		.mips = texture.mips,
	};

	sampler.device = nullptr;

	this->updateDescriptors();
}
VkImageLayout ext::vulkan::Texture::remapRenderpassLayout( VkImageLayout layout ) {
	switch ( layout ) {
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	}
	return layout;
}
void ext::vulkan::Texture::aliasAttachment( const RenderTarget::Attachment& attachment, bool createSampler ) {
	device = &ext::vulkan::device;
	aliased = true;
	image = attachment.image;
	type = ext::vulkan::enums::Image::TYPE_2D;
	viewType = attachment.views.size() == 6 ? ext::vulkan::enums::Image::VIEW_TYPE_CUBE : ext::vulkan::enums::Image::VIEW_TYPE_2D;
	view = attachment.view;
	imageLayout = ext::vulkan::Texture::remapRenderpassLayout( attachment.descriptor.layout );
	deviceMemory = attachment.mem;
	format = attachment.descriptor.format;
	mips = attachment.descriptor.mips;

	// Create sampler
	if ( createSampler ) {
		// sampler.initialize( ext::vulkan::device );
		sampler.descriptor.mip.min = 0;
		sampler.descriptor.mip.max = static_cast<float>(this->mips);
		::enforceFilterFromFormat( sampler.descriptor, format );
		sampler = ext::vulkan::Sampler::retrieve( sampler.descriptor );
	}
	
	this->updateDescriptors();
}
void ext::vulkan::Texture::aliasAttachment( const RenderTarget::Attachment& attachment, size_t layer, bool createSampler ) {
	device = &ext::vulkan::device;
	aliased = true;
	image = attachment.image;
	type = ext::vulkan::enums::Image::TYPE_2D;
	viewType = ext::vulkan::enums::Image::VIEW_TYPE_2D;
	view = attachment.views[layer];
	imageLayout = ext::vulkan::Texture::remapRenderpassLayout( attachment.descriptor.layout );
	deviceMemory = attachment.mem;
	format = attachment.descriptor.format;
	mips = attachment.descriptor.mips;

	// Create sampler
	if ( createSampler ) {
		// sampler.initialize( ext::vulkan::device );
		sampler.descriptor.mip.min = 0;
		sampler.descriptor.mip.max = static_cast<float>(this->mips);
		::enforceFilterFromFormat( sampler.descriptor, format );
		sampler = ext::vulkan::Sampler::retrieve( sampler.descriptor );
	}
	
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
	subresourceRange.baseArrayLayer = layer;
	subresourceRange.levelCount = this->mips;
	subresourceRange.layerCount = 1;

	VkBufferImageCopy bufferCopyRegion = {};
	bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	bufferCopyRegion.imageSubresource.mipLevel = 0;
	bufferCopyRegion.imageSubresource.baseArrayLayer = layer;
	bufferCopyRegion.imageSubresource.layerCount = 1;
	bufferCopyRegion.imageExtent.width = width;
	bufferCopyRegion.imageExtent.height = height;
	bufferCopyRegion.imageExtent.depth = depth;
	bufferCopyRegion.bufferOffset = 0;
	//
	Buffer staging = device.fetchTransientBuffer(
		data,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	// Use a separate command buffer for texture loading
	auto commandBuffer = device.fetchCommandBuffer(QueueEnum::GRAPHICS);

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
	device.UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "copyBuffer" );
	vkCmdCopyBufferToImage(
		commandBuffer,
		staging.buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&bufferCopyRegion
	);
	if ( this->mips > 1 ) this->generateMipmaps(commandBuffer.handle, layer);
	setImageLayout(
		commandBuffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		targetImageLayout,
		subresourceRange
	);
	
	device.flushCommandBuffer(commandBuffer);
	
	this->imageLayout = targetImageLayout;

	this->updateDescriptors();
}
void ext::vulkan::Texture::generateMipmaps( VkCommandBuffer commandBuffer, uint32_t layer ) {
	auto& device = *this->device;

	if ( this->mips <= 1 ) return;

	bool blitting = true;
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(device.physicalDevice, format, &formatProperties);
	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		UF_MSG_ERROR("Texture image format does not support linear blitting: {}", format);
		blitting = false;
	}

	bool isDepth = formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
	switch ( format ) {
		case ext::vulkan::enums::Format::D16_UNORM:
		case ext::vulkan::enums::Format::D32_SFLOAT:
		case ext::vulkan::enums::Format::D16_UNORM_S8_UINT:
		case ext::vulkan::enums::Format::D24_UNORM_S8_UINT:
		case ext::vulkan::enums::Format::D32_SFLOAT_S8_UINT:
			isDepth = true;
		break;
	}
	auto aspectMask = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	auto filter = isDepth ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;

	// base layer barrier
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = aspectMask;
	barrier.subresourceRange.baseArrayLayer = layer;
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

		if ( blitting ) {
			// blit to current mip layer
			VkImageBlit blit{};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, mipDepth };
			blit.srcSubresource.aspectMask = aspectMask;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = layer;
			blit.srcSubresource.layerCount = 1;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, mipDepth > 1 ? mipDepth / 2 : 1 };
			blit.dstSubresource.aspectMask = aspectMask;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = layer;
			blit.dstSubresource.layerCount = 1;

			device.UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "blitImage" );
			vkCmdBlitImage(
				commandBuffer,
				image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				filter
			);
		} else {
			UF_MSG_ERROR("mipmapping not supported without blitting");
		}

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

uf::Image ext::vulkan::Texture2D::screenshot( uint32_t layerID ) {
	uf::Image image;
	if ( !device ) return image;
	
	bool blitting = true;
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(device->physicalDevice, this->format, &formatProperties);
	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) blitting = false;
	vkGetPhysicalDeviceFormatProperties(device->physicalDevice, VK_FORMAT_R8G8B8A8_UNORM, &formatProperties);
	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) blitting = false;

	VkImage temporary;
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageCreateInfo.extent = { this->width, this->height, 1 };
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VmaAllocation allocation;
	VmaAllocationInfo allocationInfo;
	VmaAllocationCreateInfo allocationCreateInfo = {};
	allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
	VK_CHECK_RESULT(vmaCreateImage(allocator, &imageCreateInfo, &allocationCreateInfo, &temporary, &allocation, &allocationInfo));
	VkDeviceMemory temporaryMemory = allocationInfo.deviceMemory;

	auto commandBuffer = device->fetchCommandBuffer(QueueEnum::GRAPHICS, true); // waits on flush
	
	VkImageMemoryBarrier imageMemoryBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device.queueFamilyIndices.graphics; //VK_QUEUE_FAMILY_IGNORED
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device.queueFamilyIndices.graphics; //VK_QUEUE_FAMILY_IGNORED
	imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
	imageMemoryBarrier.subresourceRange.levelCount = 1;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imageMemoryBarrier.subresourceRange.layerCount = 1;
	imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageMemoryBarrier.srcAccessMask = 0;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	
	imageMemoryBarrier.image = temporary;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );

	imageMemoryBarrier.image = this->image;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = layerID;
	imageMemoryBarrier.oldLayout = descriptor.imageLayout;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );

	if ( blitting ) {
		VkOffset3D blitSize;
		blitSize.x = this->width;
		blitSize.y = this->height;
		blitSize.z = 1;

		VkImageBlit imageBlit{};
		imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlit.srcSubresource.baseArrayLayer = layerID;
		imageBlit.srcSubresource.layerCount = 1;
		imageBlit.srcOffsets[1] = blitSize;
		imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlit.dstSubresource.baseArrayLayer = 0;
		imageBlit.dstSubresource.layerCount = 1;
		imageBlit.dstOffsets[1] = blitSize;

		device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "blitImage" );
		vkCmdBlitImage( commandBuffer, this->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, temporary, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_NEAREST);
	} else {
		VkImageCopy imageCopy{};
		imageCopy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopy.srcSubresource.baseArrayLayer = layerID;
		imageCopy.srcSubresource.layerCount = 1;
		imageCopy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopy.dstSubresource.baseArrayLayer = 0;
		imageCopy.dstSubresource.layerCount = 1;
		imageCopy.extent = { this->width, this->height, 1 };

		device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "copyImage" );
		vkCmdCopyImage( commandBuffer, this->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, temporary, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy );
	}
	imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

	imageMemoryBarrier.image = temporary;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );

	imageMemoryBarrier.image = this->image;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = layerID;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	imageMemoryBarrier.newLayout = descriptor.imageLayout;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
	device->flushCommandBuffer(commandBuffer);

	const uint8_t* data;
	vmaMapMemory( allocator, allocation, (void**)&data );
	image.loadFromBuffer( data, {this->width, this->height}, 8, 4, false );
	vmaUnmapMemory( allocator, allocation );
	vmaDestroyImage(allocator, temporary, allocation);
	return image;
}

uf::Image ext::vulkan::Texture3D::screenshot( uint32_t layerID ) {
	uf::Image image;
	if ( !device ) return image;
	
	bool blitting = true;
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(device->physicalDevice, this->format, &formatProperties);
	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) blitting = false;
	vkGetPhysicalDeviceFormatProperties(device->physicalDevice, VK_FORMAT_R8G8B8A8_UNORM, &formatProperties);
	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) blitting = false;

	VkImage temporary;
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageCreateInfo.extent = { this->width, this->height, 1 };
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VmaAllocation allocation;
	VmaAllocationInfo allocationInfo;
	VmaAllocationCreateInfo allocationCreateInfo = {};
	allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
	VK_CHECK_RESULT(vmaCreateImage(allocator, &imageCreateInfo, &allocationCreateInfo, &temporary, &allocation, &allocationInfo));
	VkDeviceMemory temporaryMemory = allocationInfo.deviceMemory;

	auto commandBuffer = device->fetchCommandBuffer(QueueEnum::GRAPHICS);
	
	VkImageMemoryBarrier imageMemoryBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device.queueFamilyIndices.graphics; //VK_QUEUE_FAMILY_IGNORED
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device.queueFamilyIndices.graphics; //VK_QUEUE_FAMILY_IGNORED
	imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
	imageMemoryBarrier.subresourceRange.levelCount = 1;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imageMemoryBarrier.subresourceRange.layerCount = 1;
	imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageMemoryBarrier.srcAccessMask = 0;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	
	imageMemoryBarrier.image = temporary;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );

	imageMemoryBarrier.image = this->image;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imageMemoryBarrier.oldLayout = descriptor.imageLayout;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );

	if ( blitting ) {
		VkOffset3D blitSize;
		blitSize.x = this->width;
		blitSize.y = this->height;
		blitSize.z = 1;

		VkImageBlit imageBlit{};
		imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlit.srcSubresource.baseArrayLayer = 0;
		imageBlit.srcSubresource.layerCount = 1;
		imageBlit.srcOffsets[0] = { 0, 0, layerID };
		imageBlit.srcOffsets[1] = { this->width, this->height, layerID + 1 };
		imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlit.dstSubresource.baseArrayLayer = 0;
		imageBlit.dstSubresource.layerCount = 1;
		imageBlit.dstOffsets[0] = { 0, 0, 0 };
		imageBlit.dstOffsets[1] = { this->width, this->height, 1 };

		device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "blitImage" );
		vkCmdBlitImage( commandBuffer, this->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, temporary, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_NEAREST);
	} else {
		VkImageCopy imageCopy{};
		imageCopy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopy.srcSubresource.baseArrayLayer = 0;
		imageCopy.srcSubresource.layerCount = 1;
		imageCopy.srcOffset = { 0, 0, layerID };

		imageCopy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopy.dstSubresource.baseArrayLayer = 0;
		imageCopy.dstSubresource.layerCount = 1;
		imageCopy.dstOffset = { 0, 0, 0 };
		imageCopy.extent = { this->width, this->height, 1 };

		device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "copyImage" );
		vkCmdCopyImage( commandBuffer, this->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, temporary, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy );
	}
	imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

	imageMemoryBarrier.image = temporary;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );

	imageMemoryBarrier.image = this->image;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	imageMemoryBarrier.newLayout = descriptor.imageLayout;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
	device->flushCommandBuffer(commandBuffer);

	const uint8_t* data;
	vmaMapMemory( allocator, allocation, (void**)&data );
	image.loadFromBuffer( data, {this->width, this->height}, 8, 4, false );
	vmaUnmapMemory( allocator, allocation );
	vmaDestroyImage(allocator, temporary, allocation);
	return image;
}

ext::vulkan::Texture2D::Texture2D() {
	type = ext::vulkan::enums::Image::TYPE_2D;
	viewType = ext::vulkan::enums::Image::VIEW_TYPE_2D;
}
ext::vulkan::Texture3D::Texture3D() {
	type = ext::vulkan::enums::Image::TYPE_3D;
	viewType = ext::vulkan::enums::Image::VIEW_TYPE_3D;
}
ext::vulkan::TextureCube::TextureCube() {
	type = ext::vulkan::enums::Image::TYPE_2D;
	viewType = ext::vulkan::enums::Image::VIEW_TYPE_CUBE;
}

ext::vulkan::Texture2D ext::vulkan::Texture2D::alias() const {
	ext::vulkan::Texture2D texture;
	texture.aliasTexture(*this);
	return texture;
}
ext::vulkan::Texture3D ext::vulkan::Texture3D::alias() const {
	ext::vulkan::Texture3D texture;
	texture.aliasTexture(*this);
	return texture;
}
ext::vulkan::TextureCube ext::vulkan::TextureCube::alias() const {
	ext::vulkan::TextureCube texture;
	texture.aliasTexture(*this);
	return texture;
}

#endif