#pragma once

#include <uf/ext/vulkan/rendermode.h>
#include <uf/ext/vulkan/graphics/framebuffer.h>

namespace ext {
	namespace vulkan {
		struct UF_API DeferredRenderMode : public ext::vulkan::RenderMode {
			ext::vulkan::FramebufferGraphic blitter;

			// RAII
			virtual std::string getType() const;
			virtual size_t subpasses() const;
			
			virtual void createCommandBuffers( const std::vector<ext::vulkan::Graphic*>& graphics, const std::vector<std::string>& passes );
			virtual void initialize( Device& device );
			virtual void destroy();
		};
	}
}