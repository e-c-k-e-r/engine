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
			ext::vulkan::RenderTarget renderTarget;

			VkSemaphore renderCompleteSemaphore;
			std::vector<VkFence> fences;
			std::vector<VkCommandBuffer> commands;

			// virtual ~RenderMode();
			// RAII
			virtual std::string getType() const;
			const std::string& getName() const;
			virtual size_t subpasses() const;
			
			virtual void initialize( Device& device );
			virtual void createCommandBuffers();
			virtual void createCommandBuffers( const std::vector<ext::vulkan::Graphic*>& graphics, const std::vector<std::string>& passes );
			virtual void tick();
			virtual void render();
			virtual void destroy();
			virtual void synchronize( uint64_t = UINT64_MAX );
		};
	}
}