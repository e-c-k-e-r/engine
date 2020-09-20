#pragma once

#include <uf/ext/vulkan/rendermode.h>
#include <uf/ext/vulkan/graphic.h>

namespace ext {
	namespace vulkan {
		struct UF_API ComputeRenderMode : public ext::vulkan::RenderMode {
			ext::vulkan::Graphic blitter, compute;
			pod::Vector2ui dispatchSize = { 32, 32 };

			// RAII
			virtual std::string getType() const;
			
			virtual void createCommandBuffers();
			virtual void initialize( Device& device );
			virtual void tick();
			virtual void render();
			virtual void destroy();
			virtual void pipelineBarrier( VkCommandBuffer, uint8_t = -1 );
		};
	}
}