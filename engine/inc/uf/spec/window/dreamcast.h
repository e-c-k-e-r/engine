#pragma once

#include <uf/config.h>
#include "universal.h"
#include <kos.h>

#if UF_ENV_DREAMCAST
namespace spec {
	namespace dreamcast {
		class UF_API Window : public spec::uni::Window {
		public:
		// 	C-tors
			UF_API Window();
			/*virtual*/ void UF_API create( const spec::dreamcast::Window::vector_t& size, const spec::dreamcast::Window::title_t& title = "" );
		// 	D-tors
			/*virtual*/ ~Window();
			void UF_API terminate();
		// 	Gets
			spec::dreamcast::Window::handle_t UF_API getHandle() const;
			/*virtual*/ spec::dreamcast::Window::vector_t UF_API getPosition() const;
			/*virtual*/ spec::dreamcast::Window::vector_t UF_API getSize() const;
			/*virtual*/ size_t UF_API getRefreshRate() const;
		// 	Attribute modifiers
			/*virtual*/ void UF_API setPosition( const spec::dreamcast::Window::vector_t& position );
			/*virtual*/ void UF_API centerWindow();
			/*virtual*/ void UF_API setMousePosition( const spec::dreamcast::Window::vector_t& position );
			/*virtual*/ spec::dreamcast::Window::vector_t UF_API getMousePosition();
			/*virtual*/ void UF_API setSize( const spec::dreamcast::Window::vector_t& size );
			/*virtual*/ void UF_API setTitle( const spec::dreamcast::Window::title_t& title );
			/*virtual*/ void UF_API setIcon( const spec::dreamcast::Window::vector_t& size, uint8_t* pixels );
			/*virtual*/ void UF_API setVisible( bool visibility );
			/*virtual*/ void UF_API setCursorVisible( bool visibility );
			/*virtual*/ void UF_API setKeyRepeatEnabled( bool state );
			/*virtual*/ void UF_API setMouseGrabbed(bool state);
		
			/*virtual*/ void UF_API requestFocus();
			/*virtual*/ bool UF_API hasFocus() const;
		// 	Update
			/*virtual*/ void UF_API bufferInputs();
			/*virtual*/ void UF_API processEvents();
			static /*virtual*/ bool UF_API isKeyPressed(const uf::stl::string&);
			bool UF_API pollEvents( bool block = false );
		// 	Win32 specific functions
			void UF_API registerWindowClass();
			void UF_API processEvent(/*UINT message, WPARAM wParam, LPARAM lParam*/);
			void UF_API grabMouse(bool state);
			
			void UF_API setTracking(bool state);
			static pod::Vector2ui UF_API getResolution();
			void UF_API toggleFullscreen( bool borderless = false );
		};
	}
	typedef spec::dreamcast::Window Window;
}
#endif