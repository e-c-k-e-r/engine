#include <uf/spec/window/window.h>

#include <uf/utils/io/iostream.h>

#if UF_USE_VULKAN
	#include <uf/ext/vulkan/vulkan.h>
#endif

namespace spec {
	namespace null {
		Window::Window() : Window( { 1280, 720 } ) {
		}
		Window::Window( const vector_t& size, const title_t& title ) {
			create( size, title );
		}
		Window::~Window() {
		}

		void Window::create( const vector_t& size, const title_t& title ) {
			// there is nothing to create; just remember what the client wants
			m_size = size;
			m_title = title;
			m_mousePosition = { 0, 0 };
		}
		void Window::terminate() {
		}

		Window::handle_t Window::getDisplay() const {
			return nullptr;
		}
		Window::handle_t Window::getHandle() const {
			return nullptr;
		}
		spec::null::Window::vector_t Window::getPosition() const {
			return { 0, 0 };
		}
		spec::null::Window::vector_t Window::getSize() const {
			// drives the swapchain extent fallback for unbounded surfaces
			return m_size;
		}
		size_t Window::getRefreshRate() const {
			// no display to query; pretend 60Hz
			return 60;
		}

		void Window::setPosition( const vector_t& position ) {
		}
		void Window::centerWindow() {
		}
		void Window::setMousePosition( const vector_t& position ) {
			m_mousePosition = position; // headless has no cursor; injected position is what getMousePosition reports
		}
		spec::null::Window::vector_t Window::getMousePosition() {
			return m_mousePosition;
		}
		void Window::setSize( const vector_t& size ) {
			m_size = size;
		}
		void Window::setTitle( const title_t& title ) {
			m_title = title;
		}
		void Window::setIcon( const vector_t& size, uint8_t* pixels ) {
		}
		void Window::setVisible( bool visibility ) {
		}
		void Window::setCursorVisible( bool visibility ) {
		}
		void Window::setKeyRepeatEnabled( bool state ) {
		}
		void Window::setMouseGrabbed( bool state ) {
		}

		bool Window::isKeyPressed_v( const uf::stl::string& key ) {
			return false;
		}

		void Window::requestFocus() {
		}
		bool Window::hasFocus() const {
			return true;
		}

		void Window::bufferInputs() {
		}
		void Window::processEvents() {
		}
		bool Window::pollEvents( bool block ) {
			return false;
		}
		void Window::grabMouse( bool state ) {
		}

		pod::Vector2ui Window::getResolution_v() {
			return { 1280, 720 };
		}
		void Window::toggleFullscreen( bool borderless ) {
		}

	#if UF_USE_VULKAN
		uf::stl::vector<uf::stl::string> Window::getExtensions( bool validationEnabled ) {
			uf::stl::vector<uf::stl::string> exts = { VK_KHR_SURFACE_EXTENSION_NAME, VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME };
			if ( validationEnabled ) exts.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
			return exts;
		}
		void Window::createSurface( VkInstance instance, VkSurfaceKHR& surface ) {
			// the extent is unbounded by spec; the swapchain falls back to getSize()
			VkHeadlessSurfaceCreateInfoEXT info = {};
			info.sType = VK_STRUCTURE_TYPE_HEADLESS_SURFACE_CREATE_INFO_EXT;

			{
				VkResult result = vkCreateHeadlessSurfaceEXT( instance, &info, nullptr, &surface );
				if ( result == VK_ERROR_EXTENSION_NOT_PRESENT ) {
					UF_MSG_ERROR("Driver does not expose VK_EXT_headless_surface; the headless build requires a driver that does (Mesa/AMD/Intel, not NVIDIA proprietary)");
				}
				VK_CHECK_RESULT(result);
			}
		}
	#endif

		void Window::display() {
		}
	}
}
