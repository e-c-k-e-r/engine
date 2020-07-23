#pragma once

#include <uf/ext/vulkan/device.h>
#include <uf/ext/vulkan/rendertarget.h>

namespace ext {
	namespace vulkan {
		struct UF_API Swapchain {
			Device* device = nullptr;
			VkSurfaceKHR surface;
			VkSwapchainKHR swapChain = VK_NULL_HANDLE;
			

			VkSemaphore presentCompleteSemaphore;
			VkSemaphore renderCompleteSemaphore;
			std::vector<VkFence> waitFences;

			bool commandBufferSet = false;
			std::vector<VkCommandBuffer> drawCommandBuffers;

			bool vsync = true;
			uint32_t buffers;

			// helpers
			VkResult acquireNextImage( uint32_t *imageIndex );
			VkResult queuePresent( VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore = VK_NULL_HANDLE );
			
			// RAII
			void initialize( Device& device, size_t width = 0, size_t height = 0, bool vsync = true );
			void destroy();
		};
	}
}