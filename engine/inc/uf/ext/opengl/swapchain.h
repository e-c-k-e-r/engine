#pragma once

#include <uf/ext/opengl/device.h>
#include <uf/ext/opengl/rendertarget.h>

namespace ext {
	namespace opengl {
		struct UF_API Swapchain {
			Device* device = nullptr;
			GLhandle(VkSurfaceKHR) surface;
			GLhandle(VkSwapchainKHR) swapChain = GL_NULL_HANDLE;

			bool initialized = false;
			bool vsync = true;
			uint32_t buffers;

			GLhandle(VkSemaphore) presentCompleteSemaphore;
			GLhandle(VkSemaphore) renderCompleteSemaphore;

			// helpers
			GLhandle(VkResult) acquireNextImage( uint32_t* imageIndex, GLhandle(VkSemaphore) );
			GLhandle(VkResult) queuePresent( GLhandle(VkQueue) queue, uint32_t imageIndex, GLhandle(VkSemaphore) waitSemaphore = GL_NULL_HANDLE );
			
			// RAII
			void initialize( Device& device );
			void destroy();
		};
	}
}