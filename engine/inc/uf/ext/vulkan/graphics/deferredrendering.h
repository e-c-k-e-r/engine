#pragma once

#include <uf/ext/vulkan/device.h>
#include <uf/ext/vulkan/swapchain.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/graphic.h>
#include <uf/ext/vulkan/texture.h>
#include <uf/ext/vulkan/rendertarget.h>

#include <uf/utils/math/matrix.h>

namespace ext {
	namespace vulkan {
		struct UF_API DeferredRenderingGraphic : public Graphic {
			struct Vertex {
				alignas(16) pod::Vector2f position;
				alignas(16) pod::Vector2f uv;
			};

			static size_t maxLights;
			uf::Userdata uniforms;

			struct {
				uint32_t pass = 0;
			} pushConstants;

			uint32_t indices = 0;
			ext::vulkan::RenderTarget* renderTarget;
			
			virtual void createCommandBuffer( VkCommandBuffer );
			virtual bool autoAssignable() const;
			virtual std::string name() const;
			// RAII
			virtual void initialize( const std::string& = "" );
			virtual void initialize( Device& device, RenderMode& renderMode );
			virtual void destroy();
		};
	}
}