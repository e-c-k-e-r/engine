#pragma once

#include <uf/config.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <string>

namespace ext {
	namespace glfw {
		class UF_API Window {
		protected:
			size_t m_width;
			size_t m_height;
			std::string m_title;
			GLFWwindow* m_window;
		public:
			void initialize( size_t width = 800, size_t height = 600, const std::string& title = "Window" );
			void createSurface( VkInstance instance, VkSurfaceKHR& surface );
			void destroy();
			bool loop();
			void poll();
			bool minimized();
			std::vector<const char*> getExtensions( bool );
			operator GLFWwindow*();
		};
	}
}