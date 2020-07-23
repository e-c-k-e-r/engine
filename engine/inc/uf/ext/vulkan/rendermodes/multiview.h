#pragma once

#include <uf/ext/vulkan/device.h>
#include <uf/ext/vulkan/rendertarget.h>
#include <uf/ext/vulkan/graphics/framebuffer.h>

namespace ext {
	namespace vulkan {
		struct UF_API MultiviewRenderMode : public ext::vulkan::RenderMode {
			struct {
				ext::vulkan::RenderTarget left;
				ext::vulkan::RenderTarget right;
			} renderTargets;
			ext::vulkan::FramebufferGraphic blitter;

			// RAII
			virtual std::string getType() const;
			virtual size_t subpasses() const;
			
			virtual void createCommandBuffers( const std::vector<ext::vulkan::Graphic*>& graphics, const std::vector<std::string>& passes );
			virtual void initialize( Device& device );
			virtual void destroy();

			virtual VkRenderPass& getRenderPass();
			virtual ext::vulkan::RenderTarget& getRenderTarget();
		};
	}
}