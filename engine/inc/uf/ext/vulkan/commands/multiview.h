#pragma once

#include <uf/ext/vulkan/device.h>
#include <uf/ext/vulkan/rendertarget.h>
#include <uf/ext/vulkan/graphics/framebuffer.h>

namespace ext {
	namespace vulkan {
		struct UF_API MultiviewCommand : public ext::vulkan::Command {
			struct {
				ext::vulkan::RenderTarget left;
				ext::vulkan::RenderTarget right;
			} framebuffers;
			ext::vulkan::FramebufferGraphic blitter;

			// RAII
			virtual const std::string& getName() const;
			virtual void createCommandBuffers( const std::vector<void*>& graphics, const std::vector<std::string>& passes );
			virtual void render();
			virtual void initialize( Device& device );
			virtual void destroy();
		};
	}
}