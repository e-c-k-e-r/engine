#if defined(UF_USE_GLFW) && UF_USE_GLFW == 1

#include <uf/ext/glfw/glfw.h>
#include <uf/utils/window/window.h>
#include <uf/ext/vulkan/vulkan.h>
#include <uf/utils/hook/hook.h>
#include <uf/utils/serialize/serializer.h>

namespace {
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	// 	auto klass = reinterpret_cast<ext::glfw::Window*>(glfwGetWindowUserPointer(window));
		ext::vulkan::resizedFramebuffer = true;
	}
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
		if ( key == GLFW_KEY_W ) {
			uf::Serializer json;
			std::string hook = "window:Key.W";
			json["type"] = hook;
			json["invoker"] = "os";
			json["state"] = action != 0;
			uf::hooks.call( hook, json );
		}
	}
}

void ext::glfw::Window::initialize( size_t width, size_t height, const std::string& title ) {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	this->m_window = glfwCreateWindow(this->m_width = width, this->m_height = height, (this->m_title = title).c_str(), nullptr, nullptr);
	glfwSetWindowUserPointer(this->m_window, this);
	glfwSetFramebufferSizeCallback(this->m_window, framebufferResizeCallback);
	glfwSetKeyCallback(this->m_window, keyCallback);
}
void ext::glfw::Window::createSurface( VkInstance instance, VkSurfaceKHR& surface ) {
	if ( glfwCreateWindowSurface( instance, this->m_window, nullptr, &surface ) != VK_SUCCESS )
		throw std::runtime_error("failed to create window surface!");
}
bool ext::glfw::Window::loop() {
	return !glfwWindowShouldClose( this->m_window );
}
void ext::glfw::Window::poll() {
	glfwPollEvents();
}
void ext::glfw::Window::destroy() {
	glfwDestroyWindow( this->m_window );
	glfwTerminate();
}
bool ext::glfw::Window::minimized() {
	int width = 0;
	int height = 0;
	glfwGetFramebufferSize(this->m_window, &width, &height);
	glfwWaitEvents();
	return width == 0 || height == 0;
}
ext::glfw::Window::operator GLFWwindow*(){
	return this->m_window;
}
std::vector<const char*> ext::glfw::Window::getExtensions( bool enableValidationLayers ) {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> supportedExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	
	if ( enableValidationLayers ) supportedExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
/*
	std::cout << "available GLFW extensions ("<< glfwExtensionCount <<"):" << std::endl;
	for ( auto i = 0; i < glfwExtensionCount; ++i ) {
		std::cout << "\t" << glfwExtensions[i] << std::endl;
	}
*/
	return supportedExtensions;
}
#endif