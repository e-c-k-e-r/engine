#pragma once

#include <uf/ext/vulkan/device.h>

namespace ext {
	namespace vulkan {
		struct UF_API Swapchain {
			typedef struct {
				VkImage image;
				VkImageView view;
			} SwapChainBuffer;

			Device* device = nullptr;
			VkSurfaceKHR surface;

			VkFormat colorFormat;
			VkColorSpaceKHR colorSpace;
			/** @brief Handle to the current swap chain, required for recreation */
			VkSwapchainKHR swapChain = VK_NULL_HANDLE;	
			uint32_t imageCount;
			std::vector<VkImage> images;
			std::vector<SwapChainBuffer> buffers;
			/** @brief Queue family index of the detected graphics and presenting device queue */
			uint32_t queueNodeIndex = UINT32_MAX;

			VkSemaphore presentCompleteSemaphore;
			VkSemaphore renderCompleteSemaphore;
			std::vector<VkFence> waitFences;

			bool commandBufferSet = false;
			std::vector<VkCommandBuffer> drawCommandBuffers;
			VkSubmitInfo submitInfo;

			VkRenderPass renderPass;
			VkPipelineCache pipelineCache;
			VkFormat depthFormat;

			struct {
				VkImage image;
				VkDeviceMemory mem;
				VkImageView view;
			} depthStencil;
			std::vector<VkFramebuffer> frameBuffers;

			bool rebuild = false;
			bool vsync = true;

			// helpers
			VkResult acquireNextImage( uint32_t *imageIndex );
			VkResult queuePresent( VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore = VK_NULL_HANDLE );
			
			// RAII
			void initialize( Device& device, size_t width = 0, size_t height = 0, bool vsync = true );
			void destroy();
		};
	}
}