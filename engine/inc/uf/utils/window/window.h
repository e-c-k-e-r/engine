#pragma once

#include <uf/config.h>
#include <uf/spec/window/window.h>
#include <uf/spec/context/context.h>

#if !UF_USE_SFML
namespace uf {
	class UF_API Window : public spec::uni::Window {
	public:
		typedef spec::Window window_t;
		typedef spec::Context context_t;
	protected:
		Window::window_t* m_window;
		Window::context_t* m_context;
	public:
	// 	C-tors
		UF_API_CALL Window();
		UF_API_CALL Window( const spec::uni::Window::vector_t& size, const spec::uni::Window::title_t& title = L"Window", const spec::Context::Settings& settings = spec::Context::Settings() );
		/*virtual*/ void UF_API_CALL create( const spec::uni::Window::vector_t& size, const spec::uni::Window::title_t& title = L"Window", const spec::Context::Settings& settings = spec::Context::Settings() );
	// 	D-tors
		/*virtual*/ ~Window();
		/*virtual*/ void UF_API_CALL terminate();
	// 	Gets
		/*virtual*/ spec::uni::Window::vector_t UF_API_CALL getPosition() const;
		/*virtual*/ spec::uni::Window::vector_t UF_API_CALL getSize() const;
		/*virtual*/ size_t UF_API_CALL getRefreshRate() const;
	// 	Attribute modifiers
		/*virtual*/ void UF_API_CALL setPosition( const spec::uni::Window::vector_t& position );
		/*virtual*/ void UF_API_CALL centerWindow();
		/*virtual*/ void UF_API_CALL setMousePosition( const spec::uni::Window::vector_t& position );
		/*virtual*/ spec::uni::Window::vector_t UF_API_CALL getMousePosition();
		/*virtual*/ void UF_API_CALL setSize( const spec::uni::Window::vector_t& size );
		/*virtual*/ void UF_API_CALL setTitle( const spec::uni::Window::title_t& title );
		/*virtual*/ void UF_API_CALL setIcon( const spec::uni::Window::vector_t& size, uint8_t* pixels );
		/*virtual*/ void UF_API_CALL setVisible( bool visibility );
		/*virtual*/ void UF_API_CALL setCursorVisible( bool visibility );
		/*virtual*/ void UF_API_CALL setKeyRepeatEnabled( bool state );
		/*virtual*/ void UF_API_CALL setMouseGrabbed( bool state );
	
		/*virtual*/ void UF_API_CALL requestFocus();
		/*virtual*/ bool UF_API_CALL hasFocus() const;
		static pod::Vector2ui UF_API_CALL getResolution();
		/*virtual*/ void UF_API_CALL switchToFullscreen( bool borderless = false );
	// 	Update
	#if defined(UF_USE_VULKAN) && UF_USE_VULKAN == 1
		std::vector<std::string> getExtensions( bool validationEnabled = true );
		void createSurface( VkInstance instance, VkSurfaceKHR& surface );
	#endif
		static bool focused;
		static /*virtual*/ bool UF_API_CALL isKeyPressed(const std::string&);
		/*virtual*/ void UF_API_CALL processEvents();
		/*virtual*/ bool UF_API_CALL pollEvents(bool block = false);
		/*virtual*/ bool UF_API_CALL setActive( bool active = true );
		/*virtual*/ void UF_API_CALL display();
	};
}
#elif UF_USE_SFML
namespace uf {
	typedef ext::sfml::Window Window;	
}
#elif UF_USE_GLFW
#include <uf/ext/glfw/glfw.h>
namespace uf {
	typedef ext::glfw::Window Window;
}
#endif