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
			uint32_t buffers = {};

			uf::stl::vector<VkSemaphore> presentCompleteSemaphores;
			uf::stl::vector<VkImage> images;
			// surfaceless (no VkSurfaceKHR): offscreen images stand in for swapchain images
			uf::stl::vector<VmaAllocation> allocations;

			// helpers
			VkResult acquireNextImage( uint32_t* imageIndex, VkSemaphore, VkFence = VK_NULL_HANDLE );
			VkResult queuePresent( VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore = VK_NULL_HANDLE );
			
			// RAII
			void initialize( Device& device );
			void destroy();
		};
	}
}