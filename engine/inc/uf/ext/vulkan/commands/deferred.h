#pragma once

#include <uf/ext/vulkan/device.h>
#include <uf/ext/vulkan/rendertarget.h>
#include <uf/ext/vulkan/graphics/framebuffer.h>

namespace ext {
	namespace vulkan {
		struct UF_API DeferredCommand : public ext::vulkan::Command {
			ext::vulkan::RenderTarget framebuffer;
			ext::vulkan::FramebufferGraphic blitter;

			// RAII
			virtual const std::string& getName() const;
			virtual size_t subpasses() const;
			
			virtual void createCommandBuffers( const std::vector<void*>& graphics, const std::vector<std::string>& passes );
			virtual void render();
			virtual void initialize( Device& device );
			virtual void destroy();
			virtual VkRenderPass& getRenderPass();
		};
	}
}