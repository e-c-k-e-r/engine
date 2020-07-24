#pragma once

#include <uf/ext/vulkan/device.h>
#include <uf/ext/vulkan/rendertarget.h>

namespace ext {
	namespace vulkan {
		struct UF_API Swapchain {
			Device* device = nullptr;
			VkSurfaceKHR surface;
			VkSwapchainKHR swapChain = VK_NULL_HANDLE;

			bool initialized = false;
			bool vsync = true;
			uint32_t buffers;

			VkSemaphore presentCompleteSemaphore;
			VkSemaphore renderCompleteSemaphore;

			// helpers
			VkResult acquireNextImage( uint32_t* imageIndex, VkSemaphore );
			VkResult queuePresent( VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore = VK_NULL_HANDLE );
			
			// RAII
			void initialize( Device& device );
			void destroy();
		};
	}
}