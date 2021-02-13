#pragma once

#pragma once

#include <uf/ext/opengl/device.h>
#include <uf/ext/opengl/rendertarget.h>
#include <uf/utils/image/image.h>

namespace ext {
	namespace opengl {
		struct UF_API Sampler {
			Device* device = NULL;

			GLhandle(VkSampler) sampler;
			struct {
				struct {
					GLhandle(VkFilter) min = GLenumerator(VK_FILTER_LINEAR);
					GLhandle(VkFilter) mag = GLenumerator(VK_FILTER_LINEAR);
					GLhandle(VkBorderColor) borderColor = GLenumerator(VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK);
				} filter;
				struct {
					GLhandle(VkSamplerAddressMode) u = GLenumerator(VK_SAMPLER_ADDRESS_MODE_REPEAT), v = GLenumerator(VK_SAMPLER_ADDRESS_MODE_REPEAT), w = GLenumerator(VK_SAMPLER_ADDRESS_MODE_REPEAT);
					GLhandle(VkBool32) unnormalizedCoordinates = GLenumerator(VK_FALSE);
				} addressMode;
				struct {
					GLhandle(VkBool32) compareEnable = GLenumerator(VK_FALSE);
					GLhandle(VkCompareOp) compareOp = GLenumerator(VK_COMPARE_OP_ALWAYS);
					GLhandle(VkSamplerMipmapMode) mode = GLenumerator(VK_SAMPLER_MIPMAP_MODE_LINEAR);
					float lodBias = 0.0f;
					float min = 0.0f;
					float max = 0.0f;
				} mip;
				struct {
					GLhandle(VkBool32) enabled = GLenumerator(VK_TRUE);
					float max = 16.0f;
				} anisotropy;
				GLhandle(VkDescriptorImageInfo) info;
			} descriptor;

			void initialize( Device& device );
			void destroy();
		};
		struct UF_API Texture {
			Device* device = nullptr;

			GLhandle(VkImage) image;
			GLhandle(VkImageView) view;
			GLhandle(VkImageType) type;
			GLhandle(VkImageViewType) viewType;
			GLhandle(VkImageLayout) imageLayout = GLenumerator(VK_IMAGE_LAYOUT_UNDEFINED);
			GLhandle(VkDeviceMemory) deviceMemory;
			GLhandle(VkDescriptorImageInfo) descriptor;
			GLhandle(VkFormat) format;
			
			Sampler sampler;

			GLhandle(VmaAllocation) allocation;
			GLhandle(VmaAllocationInfo) allocationInfo;

			uint32_t width, height, depth, layers;
			uint32_t mips = 1;
			// RAII
			void initialize( Device& device, size_t width, size_t height, size_t depth = 1, size_t layers = 1 );
			void initialize( Device& device, GLhandle(VkImageViewType), size_t width, size_t height, size_t depth = 1, size_t layers = 1 );
			void updateDescriptors();
			void destroy();
			bool generated() const;
			static void setImageLayout(
				GLhandle(VkCommandBuffer) cmdbuffer,
				GLhandle(VkImage) image,
				GLhandle(VkImageLayout) oldImageLayout,
				GLhandle(VkImageLayout) newImageLayout,
				GLhandle(VkImageSubresourceRange) subresourceRange
			);
			static void setImageLayout(
				GLhandle(VkCommandBuffer) cmdbuffer,
				GLhandle(VkImage) image,
				GLhandle(VkImageAspectFlags) aspectMask,
				GLhandle(VkImageLayout) oldImageLayout,
				GLhandle(VkImageLayout) newImageLayout,
				uint32_t mipLevels
			);
			void loadFromFile(
				std::string filename, 
				GLhandle(VkFormat) format = GLenumerator(VK_FORMAT_R8G8B8A8_UNORM),
				GLhandle(VkImageUsageFlags) imageUsageFlags = GLenumerator(VK_IMAGE_USAGE_SAMPLED_BIT),
				GLhandle(VkImageLayout) imageLayout = GLenumerator(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			);
			void loadFromFile(
				std::string filename, 
				Device& device,
				GLhandle(VkFormat) format = GLenumerator(VK_FORMAT_R8G8B8A8_UNORM),
				GLhandle(VkImageUsageFlags) imageUsageFlags = GLenumerator(VK_IMAGE_USAGE_SAMPLED_BIT),
				GLhandle(VkImageLayout) imageLayout = GLenumerator(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			);
			void loadFromImage(
				uf::Image& image, 
				Device& device,
				GLhandle(VkFormat) format = GLenumerator(VK_FORMAT_R8G8B8A8_UNORM),
				GLhandle(VkImageUsageFlags) imageUsageFlags = GLenumerator(VK_IMAGE_USAGE_SAMPLED_BIT),
				GLhandle(VkImageLayout) imageLayout = GLenumerator(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			);
			void loadFromImage(
				uf::Image& image,
				GLhandle(VkFormat) format = GLenumerator(VK_FORMAT_R8G8B8A8_UNORM),
				GLhandle(VkImageUsageFlags) imageUsageFlags = GLenumerator(VK_IMAGE_USAGE_SAMPLED_BIT),
				GLhandle(VkImageLayout) imageLayout = GLenumerator(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			);
			void fromBuffers( void* buffer, GLhandle(VkDeviceSize) bufferSize, GLhandle(VkFormat) format, uint32_t texWidth, uint32_t texHeight, uint32_t texDepth, uint32_t layers, Device& device, GLhandle(VkImageUsageFlags) imageUsageFlags = GLenumerator(VK_IMAGE_USAGE_SAMPLED_BIT), GLhandle(VkImageLayout) imageLayout = GLenumerator(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) );
			void asRenderTarget( Device& device, uint32_t texWidth, uint32_t texHeight, GLhandle(VkFormat) format = GLenumerator(VK_FORMAT_R8G8B8A8_UNORM) );
			void aliasTexture( const Texture& );
			void aliasAttachment( const RenderTarget::Attachment& attachment, bool = true );
			
			void update( uf::Image& image, GLhandle(VkImageLayout), uint32_t layer = 1 );
			void update( void*, GLhandle(VkDeviceSize), GLhandle(VkImageLayout), uint32_t layer = 1 );

			inline void fromBuffers( void* buffer, GLhandle(VkDeviceSize) bufferSize, GLhandle(VkFormat) format, uint32_t texWidth, uint32_t texHeight, Device& device, GLhandle(VkImageUsageFlags) imageUsageFlags = GLenumerator(VK_IMAGE_USAGE_SAMPLED_BIT), GLhandle(VkImageLayout) imageLayout = GLenumerator(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) ) { return this->fromBuffers(buffer, bufferSize, format, texWidth, texHeight, 1, 1, device, imageUsageFlags, imageLayout); }
			inline void update( uf::Image& image, uint32_t layer = 1 ) { return this->update(image, this->imageLayout, layer); }
			inline void update( void* data, GLhandle(VkDeviceSize) size, uint32_t layer = 1 ) { return this->update(data, size, this->imageLayout, layer); }
			
			void generateMipmaps(GLhandle(VkCommandBuffer) commandBuffer, uint32_t layer = 1);

			void fromBuffers( void* buffer, GLhandle(VkDeviceSize) bufferSize, GLhandle(VkFormat) format, uint32_t texWidth, uint32_t texHeight, uint32_t texDepth, uint32_t layers, GLhandle(VkImageUsageFlags) imageUsageFlags = GLenumerator(VK_IMAGE_USAGE_SAMPLED_BIT), GLhandle(VkImageLayout) imageLayout = GLenumerator(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) );
			inline void fromBuffers( void* buffer, GLhandle(VkDeviceSize) bufferSize, GLhandle(VkFormat) format, uint32_t texWidth, uint32_t texHeight, GLhandle(VkImageUsageFlags) imageUsageFlags = GLenumerator(VK_IMAGE_USAGE_SAMPLED_BIT), GLhandle(VkImageLayout) imageLayout = GLenumerator(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) ) { return this->fromBuffers(buffer, bufferSize, format, texWidth, texHeight, 1, 1, imageUsageFlags, imageLayout); }
		};
		class UF_API Texture2D : public Texture {
		public:
			Texture2D();
			static Texture2D empty;
		};
		class UF_API Texture3D : public Texture {
		public:
			Texture3D();
		};
		class UF_API TextureCube : public Texture {
		public:
			TextureCube();
		};
	}
}