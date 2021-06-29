#pragma once

#include <uf/config.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <uf/utils/memory/vector.h>
#include <uf/utils/memory/string.h>

namespace ext {
	namespace glfw {
		class UF_API Window {
		protected:
			size_t m_width;
			size_t m_height;
			uf::stl::string m_title;
			GLFWwindow* m_window;
		public:
			void initialize( size_t width = 800, size_t height = 600, const uf::stl::string& title = "Window" );
			void createSurface( VkInstance instance, VkSurfaceKHR& surface );
			void destroy();
			bool loop();
			void poll();
			bool minimized();
			uf::stl::vector<const char*> getExtensions( bool );
			operator GLFWwindow*();
		};
	}
}