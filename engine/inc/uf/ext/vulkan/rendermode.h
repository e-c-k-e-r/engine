#pragma once

#include <uf/ext/vulkan/device.h>

namespace ext {
	namespace vulkan {
		struct Graphic;
		
		struct UF_API RenderMode {
			uint32_t width = 0, height = 0;
			std::string name = "";
			pod::Vector4f clearColor = {0, 0, 0, 1};
			
			Device* device = VK_NULL_HANDLE;
			RenderTarget renderTarget;

			VkSemaphore renderCompleteSemaphore;
			std::vector<VkFence> fences;
			std::vector<VkCommandBuffer> commands;

			// virtual ~RenderMode();
			// RAII
			virtual std::string getType() const;
			const std::string& getName() const;
			virtual RenderTarget& getRenderTarget(size_t = 0);
			virtual const RenderTarget& getRenderTarget(size_t = 0) const;
			
			virtual void initialize( Device& device );
			virtual void createCommandBuffers();
			virtual void createCommandBuffers( const std::vector<ext::vulkan::Graphic*>& graphics );
			virtual void tick();
			virtual void render();
			virtual void destroy();
			virtual void synchronize( uint64_t = UINT64_MAX );
		};
	}
}