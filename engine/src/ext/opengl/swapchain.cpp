#if UF_USE_OPENGL

#include <uf/ext/opengl/opengl.h>
#include <uf/ext/opengl/swapchain.h>
#include <uf/ext/opengl/initializers.h>
#include <uf/utils/window/window.h>

GLhandle(VkResult) ext::opengl::Swapchain::acquireNextImage( uint32_t* imageIndex, GLhandle(VkSemaphore) presentCompleteSemaphore ) {
	return false;
}

GLhandle(VkResult) ext::opengl::Swapchain::queuePresent( GLhandle(VkQueue) queue, uint32_t imageIndex, GLhandle(VkSemaphore) waitSemaphore ) {
	return false;
}

void ext::opengl::Swapchain::initialize( Device& device ) {
}

void ext::opengl::Swapchain::destroy() {
}

#endif