#pragma once

#pragma once

#include <uf/ext/opengl/enums.h>
#include <uf/ext/opengl/rendertarget.h>
#include <uf/utils/image/image.h>

namespace ext {
	namespace opengl {
		struct Device;

		struct UF_API Sampler {
			Device* device = NULL;

			GLhandle(VkSampler) sampler;
			struct {
				struct {
					enums::Filter::type_t min = enums::Filter::LINEAR;
					enums::Filter::type_t mag = enums::Filter::LINEAR;
					GLhandle(VkBorderColor) borderColor = GLenumerator(VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK);
				} filter;
				struct {
					enums::AddressMode::type_t  u = enums::AddressMode::REPEAT,
					 							v = enums::AddressMode::REPEAT,
					 							w = enums::AddressMode::REPEAT;
					bool unnormalizedCoordinates = false;
				} addressMode;
				struct {
					bool compareEnable = false;
					enums::Compare::type_t compareOp = enums::Compare::ALWAYS;
					GLhandle(VkSamplerMipmapMode) mode = GLenumerator(VK_SAMPLER_MIPMAP_MODE_LINEAR);
					float lodBias = 0.0f;
					float min = 0.0f;
					float max = 0.0f;
				} mip;
				struct {
					bool enabled = true;
					float max = 16.0f;
				} anisotropy;
				struct {
					uint8_t mode{};
					float alphaCutoff = 0.001f;
				} blend;

				GLhandle(VkDescriptorImageInfo) info;
			} descriptor;

			void initialize( Device& device );
			void destroy();
		};
		struct UF_API Texture {
			enums::Format::type_t DefaultFormat = enums::Format::R8G8B8A8_UNORM;

			Device* device = nullptr;

			GLuint image = GL_NULL_HANDLE;
			enums::Image::type_t type = enums::Image::TYPE_2D;
			enums::Image::viewType_t viewType = enums::Image::VIEW_TYPE_2D;
			enums::Format::type_t format = enums::Format::R8G8B8A8_UNORM;
			size_t internalFormat = 0;
			bool srgb = false;

			struct Descriptor {
				GLuint image = GL_NULL_HANDLE;
				enums::Image::viewType_t viewType = 0;
				enums::Format::type_t format = 0;
				uint32_t width = 0, height = 0, depth = 0, layers = 0;
			} descriptor;

			Sampler sampler;

			uint32_t width, height, depth, layers;
			uint32_t mips = 1;
			// RAII
		#if UF_ENV_DREAMCAST
			void initialize( Device& device, size_t width, size_t height, size_t depth, size_t layers );
			void initialize( Device& device, enums::Image::viewType_t, size_t width, size_t height, size_t depth, size_t layers );
		#else
			void initialize( Device& device, size_t width, size_t height, size_t depth = 1, size_t layers = 1 );
			void initialize( Device& device, enums::Image::viewType_t, size_t width, size_t height, size_t depth = 1, size_t layers = 1 );
		#endif
			void updateDescriptors();
			void destroy();
			bool generated() const;
			void loadFromFile(
				const uf::stl::string& filename, 
				enums::Format::type_t format = enums::Format::R8G8B8A8_UNORM
			);
			void loadFromFile(
				const uf::stl::string& filename, 
				Device& device,
				enums::Format::type_t format = enums::Format::R8G8B8A8_UNORM
			);
			void loadFromImage(
				const uf::Image& image,
				enums::Format::type_t format = enums::Format::R8G8B8A8_UNORM
			);
			void loadFromImage(
				const uf::Image& image, 
				Device& device,
				enums::Format::type_t format = enums::Format::R8G8B8A8_UNORM
			);
			void fromBuffers( void* buffer, size_t bufferSize, enums::Format::type_t format, uint32_t texWidth, uint32_t texHeight, uint32_t texDepth, uint32_t layers, Device& device );
			void asRenderTarget( Device& device, uint32_t texWidth, uint32_t texHeight, enums::Format::type_t format = enums::Format::R8G8B8A8_UNORM );
			
			Texture alias() const;
			void aliasTexture( const Texture& );

			void aliasAttachment( const RenderTarget::Attachment& attachment, bool = true );
			
			void update( uf::Image& image, uint32_t layer = 1 );
			void update( void*, size_t, uint32_t layer = 1 );

			inline void fromBuffers( void* buffer, size_t bufferSize, enums::Format::type_t format, uint32_t texWidth, uint32_t texHeight, Device& device ) { return this->fromBuffers(buffer, bufferSize, format, texWidth, texHeight, 1, 1, device); }
			
			void generateMipmaps( uint32_t layer = 1);

			void fromBuffers( void* buffer, size_t bufferSize, enums::Format::type_t format, uint32_t texWidth, uint32_t texHeight, uint32_t texDepth, uint32_t layers );
			inline void fromBuffers( void* buffer, size_t bufferSize, enums::Format::type_t format, uint32_t texWidth, uint32_t texHeight ) { return this->fromBuffers(buffer, bufferSize, format, texWidth, texHeight, 1, 1); }
		};
		class UF_API Texture2D : public Texture {
		public:
			Texture2D();
			static Texture2D empty;
			
			Texture2D alias() const;
		};
		class UF_API Texture3D : public Texture {
		public:
			Texture3D();
			
			Texture3D alias() const;
		};
		class UF_API TextureCube : public Texture {
		public:
			TextureCube();
			
			TextureCube alias() const;
		};
	}
}