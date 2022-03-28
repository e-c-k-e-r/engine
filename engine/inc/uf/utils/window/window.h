#pragma once

#include <uf/config.h>
#include <uf/spec/window/window.h>
#include <uf/spec/context/context.h>

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
		void UF_API_CALL create( const spec::uni::Window::vector_t& size, const spec::uni::Window::title_t& title = L"Window", const spec::Context::Settings& settings = spec::Context::Settings() );
	// 	D-tors
		~Window();
		void UF_API_CALL terminate();
	// 	Gets
		spec::uni::Window::vector_t UF_API_CALL getPosition() const;
		spec::uni::Window::vector_t UF_API_CALL getSize() const;
		size_t UF_API_CALL getRefreshRate() const;
	// 	Attribute modifiers
		void UF_API_CALL setPosition( const spec::uni::Window::vector_t& position );
		void UF_API_CALL centerWindow();
		void UF_API_CALL setMousePosition( const spec::uni::Window::vector_t& position );
		spec::uni::Window::vector_t UF_API_CALL getMousePosition();
		void UF_API_CALL setSize( const spec::uni::Window::vector_t& size );
		void UF_API_CALL setTitle( const spec::uni::Window::title_t& title );
		void UF_API_CALL setIcon( const spec::uni::Window::vector_t& size, uint8_t* pixels );
		void UF_API_CALL setVisible( bool visibility );
		void UF_API_CALL setCursorVisible( bool visibility );
		void UF_API_CALL setKeyRepeatEnabled( bool state );
		void UF_API_CALL setMouseGrabbed( bool state );
	
		void UF_API_CALL requestFocus();
		bool UF_API_CALL hasFocus() const;
		static pod::Vector2ui UF_API_CALL getResolution();
		void UF_API_CALL toggleFullscreen( bool borderless = false );

	#if defined(UF_USE_VULKAN) && UF_USE_VULKAN == 1
		uf::stl::vector<uf::stl::string> getExtensions( bool validationEnabled = true );
		void createSurface( VkInstance instance, VkSurfaceKHR& surface );
	#endif

		static bool focused;
		static bool UF_API_CALL isKeyPressed(const uf::stl::string&);

		void UF_API_CALL bufferInputs();
		void UF_API_CALL processEvents();
		bool UF_API_CALL pollEvents(bool block = false);
		bool UF_API_CALL setActive( bool active = true );
		void UF_API_CALL display();
		Window::window_t* UF_API_CALL getHandle();
		const Window::window_t* UF_API_CALL getHandle() const;
	};
}