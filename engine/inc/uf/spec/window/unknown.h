#pragma once

#include <uf/config.h>
#include "universal.h"

#ifdef UF_ENV_UNKNOWN
namespace spec {
	namespace unknown {
		class UF_API Window : public spec::uni::Window {
		public:
			typedef void* 							handle_t;
			typedef spec::uni::Window::title_t 		title_t;
			typedef spec::uni::Window::vector_t 	vector_t;
		
		protected:

		public:
		// 	C-tors
			UF_API_CALL Window( spec::unknown::Window::handle_t ) {}
			UF_API_CALL Window( const spec::unknown::Window::vector_t& size, const spec::unknown::Window::title_t& title = L"Window" ) {}
			/*virtual*/ void UF_API_CALL create( const spec::unknown::Window::vector_t& size, const spec::unknown::Window::title_t& title = L"Window" ) {}
		// 	D-tors
			/*virtual*/ ~Window() {}
			void UF_API_CALL terminate() {}
		// 	Gets
			spec::unknown::Window::handle_t UF_API_CALL getHandle() const { return NULL; }
			/*virtual*/ spec::unknown::Window::vector_t UF_API_CALL getPosition() const { return spec::unknown::Window::vector_t(0,0); }
			/*virtual*/ spec::unknown::Window::vector_t UF_API_CALL getSize() const { return spec::unknown::Window::vector_t(0,0); }
		// 	Attribute modifiers
			/*virtual*/ void UF_API_CALL setPosition( const spec::unknown::Window::vector_t& position ) {}
			/*virtual*/ void UF_API_CALL centerWindow() {}
			/*virtual*/ void UF_API_CALL setMousePosition( const spec::unknown::Window::vector_t& position ) {}
			/*virtual*/ void UF_API_CALL setSize( const spec::unknown::Window::vector_t& size ) {}
			/*virtual*/ void UF_API_CALL setTitle( const spec::unknown::Window::title_t& title ) {}
			/*virtual*/ void UF_API_CALL setIcon( const spec::unknown::Window::vector_t& size, uint8_t* pixels ) {}
			/*virtual*/ void UF_API_CALL setVisible( bool visibility ) {}
			/*virtual*/ void UF_API_CALL setCursorVisible( bool visibility ) {}
			/*virtual*/ void UF_API_CALL setKeyRepeatEnabled( bool state ) {}
			/*virtual*/ void UF_API_CALL setMouseGrabbed( bool state ) {}
		
			/*virtual*/ void UF_API_CALL requestFocus() {}
			/*virtual*/ bool UF_API_CALL hasFocus() const { return false; }
		// 	Update
			/*virtual*/ void UF_API_CALL processEvents() {}
		};
	}
	typedef spec::unknown::Window Window;
}
#endif