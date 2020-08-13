#pragma once

#include <uf/ext/vulkan/rendermode.h>
#include <uf/ext/vulkan/graphics/deferredrendering.h>

namespace ext {
	namespace vulkan {
		struct UF_API DeferredRenderMode : public ext::vulkan::RenderMode {
			ext::vulkan::DeferredRenderingGraphic blitter;

			// RAII
			virtual std::string getType() const;
			
			virtual void createCommandBuffers( const std::vector<ext::vulkan::Graphic*>& graphics );
			virtual void initialize( Device& device );
			virtual void tick();
			virtual void destroy();
		};
	}
}