#pragma once

#include <uf/ext/vulkan/device.h>
#include <uf/utils/image/image.h>

#include <uf/ext/vulkan/rendertarget.h>

namespace ext {
	namespace vulkan {
		struct UF_API Sampler {
			Device* device = NULL;

			VkSampler sampler;
			struct {
				struct {
					VkFilter min = VK_FILTER_LINEAR;
					VkFilter mag = VK_FILTER_LINEAR;
					VkBorderColor borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
				} filter;
				struct {
					VkSamplerAddressMode u = VK_SAMPLER_ADDRESS_MODE_REPEAT, v = VK_SAMPLER_ADDRESS_MODE_REPEAT, w = VK_SAMPLER_ADDRESS_MODE_REPEAT;
					VkBool32 unnormalizedCoordinates = VK_FALSE;
				} addressMode;
				struct {
					VkBool32 compareEnable = VK_FALSE;
					VkCompareOp compareOp = VK_COMPARE_OP_ALWAYS;
					VkSamplerMipmapMode mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
					float lodBias = 0.0f;
					float min = 0.0f;
					float max = 0.0f;
				} mip;
				struct {
					VkBool32 enabled = VK_TRUE;
					float max = 16.0f;
				} anisotropy;
				VkDescriptorImageInfo info;
			} descriptor;

			void initialize( Device& device );
			void destroy();
		};
		struct UF_API Texture {
			Device* device = nullptr;

			VkImage image;
			VkImageView view;
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			VkDeviceMemory deviceMemory;
			VkDescriptorImageInfo descriptor;
			VkFormat format;
			
			Sampler sampler;

			VmaAllocation allocation;
			VmaAllocationInfo allocationInfo;

			uint32_t width, height;
			uint32_t mips;
			uint32_t layers;
			// RAII
			void initialize( Device& device, size_t width, size_t height );
			void updateDescriptors();
			void destroy();
			bool generated() const;
			static void setImageLayout(
				VkCommandBuffer cmdbuffer,
				VkImage image,
				VkImageLayout oldImageLayout,
				VkImageLayout newImageLayout,
				VkImageSubresourceRange subresourceRange
			);
			static void setImageLayout(
				VkCommandBuffer cmdbuffer,
				VkImage image,
				VkImageAspectFlags aspectMask,
				VkImageLayout oldImageLayout,
				VkImageLayout newImageLayout,
				uint32_t mipLevels
			);
		};
		struct UF_API Texture2D : public Texture {
			static Texture2D empty;
			void loadFromFile(
				std::string filename, 
				VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
				VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
				VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
			void loadFromFile(
				std::string filename, 
				Device& device,
				VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
				VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
				VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
			void loadFromImage(
				uf::Image& image, 
				Device& device,
				VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
				VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
				VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
			void loadFromImage(
				uf::Image& image,
				VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
				VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
				VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
			void fromBuffers(
				void* buffer,
				VkDeviceSize bufferSize,
				VkFormat format,
				uint32_t texWidth,
				uint32_t texHeight,
				Device& device,
				VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
				VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
			void asRenderTarget( Device& device, uint32_t texWidth, uint32_t texHeight, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM );
			void aliasAttachment( const RenderTarget::Attachment& attachment, bool = true );
			void update( uf::Image& image, VkImageLayout );
			void update( void*, VkDeviceSize, VkImageLayout );
			inline void update( uf::Image& image ) { return this->update(image, this->imageLayout) ; }
			inline void update( void* data, VkDeviceSize size ) { return this->update(data, size, this->imageLayout) ; }
			
			void generateMipmaps(VkCommandBuffer commandBuffer);
		};
	}
}