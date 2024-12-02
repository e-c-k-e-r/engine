#pragma once

#include <uf/ext/vulkan/enums.h>
#include <uf/ext/vulkan/device.h>
#include <uf/ext/vulkan/rendertarget.h>
#include <uf/utils/image/image.h>

namespace ext {
	namespace vulkan {
		struct UF_API Sampler {
			Device* device = NULL;

			VkSampler sampler = VK_NULL_HANDLE;
			struct Descriptor {
				struct {
					VkFilter min = VK_FILTER_LINEAR;
					VkFilter mag = VK_FILTER_LINEAR;
					VkBorderColor borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
				} filter;
				struct {
					VkSamplerAddressMode u = VK_SAMPLER_ADDRESS_MODE_REPEAT, v = VK_SAMPLER_ADDRESS_MODE_REPEAT, w = VK_SAMPLER_ADDRESS_MODE_REPEAT;
					bool unnormalizedCoordinates = VK_FALSE;
				} addressMode;
				struct {
					bool compareEnable = VK_FALSE;
					VkCompareOp compareOp = VK_COMPARE_OP_ALWAYS;
					VkSamplerMipmapMode mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
					float lodBias = 0.0f;
					float min = 0.0f;
					float max = 0.0f;
				} mip;
				struct {
					bool enabled = VK_TRUE;
					float max = 16.0f;
				} anisotropy;
				struct {
					bool enabled = false;
					VkSamplerReductionMode mode = {};
				} reduction;
				VkDescriptorImageInfo info;
			} descriptor = {};

			void initialize( Device& device );
			void destroy(bool defer = VK_DEFAULT_DEFER_BUFFER_DESTROY);

			static uf::stl::vector<ext::vulkan::Sampler> samplers;
			static ext::vulkan::Sampler retrieve( const Descriptor& info );
		};

		struct UF_API Texture {
			static VkFormat DefaultFormat;

			Device* device = nullptr;
			bool aliased = false;

			VkImage image = VK_NULL_HANDLE;
			VkImageView view = VK_NULL_HANDLE;
			VkImageType type = enums::Image::TYPE_2D;
			VkImageViewType viewType = enums::Image::VIEW_TYPE_2D;
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
			VkDescriptorImageInfo descriptor = {};
			VkFormat format = enums::Format::R8G8B8A8_UNORM;
			bool srgb = false;
			
			Sampler sampler = {};

			VmaAllocation allocation = {};
			VmaAllocationInfo allocationInfo = {};

			uint32_t width = 0;
			uint32_t height = 0;
			uint32_t depth = 0;
			uint32_t layers = 0;
			uint32_t mips = 1;
			// RAII
			void initialize( Device& device, size_t width, size_t height, size_t depth = 1, size_t layers = 1 );
			void initialize( Device& device, VkImageViewType, size_t width, size_t height, size_t depth = 1, size_t layers = 1 );
			void updateDescriptors();
			void destroy(bool defer = VK_DEFAULT_DEFER_BUFFER_DESTROY);
			bool generated() const;

			static VkImageLayout remapRenderpassLayout( VkImageLayout );
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
			void loadFromFile(
				const uf::stl::string& filename, 
				VkFormat format = DefaultFormat,
				VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
				VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
			void loadFromFile(
				const uf::stl::string& filename, 
				Device& device,
				VkFormat format = DefaultFormat,
				VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
				VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
			void loadFromImage(
				const uf::Image& image, 
				Device& device,
				VkFormat format = DefaultFormat,
				VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
				VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
			void loadFromImage(
				const uf::Image& image,
				VkFormat format = DefaultFormat,
				VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
				VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
			void fromBuffers( void* buffer, VkDeviceSize bufferSize, VkFormat format, uint32_t texWidth, uint32_t texHeight, uint32_t texDepth, uint32_t layers, Device& device, VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
			void asRenderTarget( Device& device, uint32_t texWidth, uint32_t texHeight, VkFormat format = DefaultFormat );
			
			Texture alias() const;
			void aliasTexture( const Texture& );

			void aliasAttachment( const RenderTarget::Attachment& attachment, bool = true );
			void aliasAttachment( const RenderTarget::Attachment& attachment, size_t, bool = true );
			
			void update( uf::Image& image, VkImageLayout, uint32_t layer = 1 );
			void update( void*, VkDeviceSize, VkImageLayout, uint32_t layer = 1 );

			inline void fromBuffers( void* buffer, VkDeviceSize bufferSize, VkFormat format, uint32_t texWidth, uint32_t texHeight, Device& device, VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ) { return this->fromBuffers(buffer, bufferSize, format, texWidth, texHeight, 1, 1, device, imageUsageFlags, imageLayout); }
			inline void update( uf::Image& image, uint32_t layer = 1 ) { return this->update(image, this->imageLayout, layer); }
			inline void update( void* data, VkDeviceSize size, uint32_t layer = 1 ) { return this->update(data, size, this->imageLayout, layer); }
			
			void generateMipmaps(VkCommandBuffer commandBuffer, uint32_t layer = 1);

			void fromBuffers( void* buffer, VkDeviceSize bufferSize, VkFormat format, uint32_t texWidth, uint32_t texHeight, uint32_t texDepth, uint32_t layers, VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
			inline void fromBuffers( void* buffer, VkDeviceSize bufferSize, VkFormat format, uint32_t texWidth, uint32_t texHeight, VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ) { return this->fromBuffers(buffer, bufferSize, format, texWidth, texHeight, 1, 1, imageUsageFlags, imageLayout); }
		};
		class UF_API Texture2D : public Texture {
		public:
			static Texture2D empty;
			Texture2D();

			Texture2D alias() const;
			uf::Image screenshot( uint32_t layer = 0 );
		};
		class UF_API Texture3D : public Texture {
		public:
			static Texture3D empty;
			Texture3D();
			
			Texture3D alias() const;
			uf::Image screenshot( uint32_t layer = 0 );
		};
		class UF_API TextureCube : public Texture {
		public:
			static TextureCube empty;
			TextureCube();
			
			TextureCube alias() const;
		};
	}
}