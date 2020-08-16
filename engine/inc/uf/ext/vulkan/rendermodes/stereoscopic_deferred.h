#pragma once

#include <uf/ext/vulkan/rendermode.h>
#include <uf/ext/vulkan/graphics/deferredrendering.h>

namespace ext {
	namespace vulkan {
		struct UF_API StereoscopicDeferredRenderMode : public ext::vulkan::RenderMode {
			struct {
				ext::vulkan::RenderTarget& left;
				ext::vulkan::RenderTarget right;
			} renderTargets;
			struct {
				ext::vulkan::Graphic left;
				ext::vulkan::Graphic right;
			} blitters;
			ext::vulkan::Graphic blitter;

			StereoscopicDeferredRenderMode();
			// RAII
			virtual std::string getType() const;
			virtual ext::vulkan::RenderTarget& getRenderTarget(size_t = 0);
			virtual const ext::vulkan::RenderTarget& getRenderTarget(size_t = 0) const;
			
			virtual void createCommandBuffers( const std::vector<ext::vulkan::Graphic*>& graphics );
			virtual void initialize( Device& device );
			virtual void tick();
			virtual void destroy();
		};
	}
}