#pragma once

#include <uf/ext/vulkan/rendermode.h>
#include <uf/ext/vulkan/graphics/rendertarget.h>

namespace ext {
	namespace vulkan {
		struct UF_API RenderTargetRenderMode : public ext::vulkan::RenderMode {
			ext::vulkan::RenderTargetGraphic blitter;
			
			VkFence fence;
			VkCommandBuffer commandBuffer;

			// RAII
			virtual std::string getType() const;
			virtual size_t subpasses() const;
			
			virtual void createCommandBuffers( const std::vector<ext::vulkan::Graphic*>& graphics, const std::vector<std::string>& passes );
			virtual void initialize( Device& device );
			virtual void destroy();
			virtual void render();
		};
	}
}