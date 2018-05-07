#pragma once

#include <uf/config.h>
#include "universal.h"

#if defined(UF_ENV_WINDOWS) && (!defined(UF_USE_SFML) || (defined(UF_USE_SFML) && UF_USE_SFML == 0))

namespace spec {
	namespace win32 {
		class UF_API Window : public spec::uni::Window {
		public:
			typedef HWND 							handle_t;
		/*
			typedef spec::uni::Window::title_t 		title_t;
			typedef spec::uni::Window::vector_t 	vector_t;
		*/
		protected:
			spec::win32::Window::handle_t 			m_handle;
			LONG_PTR 								m_callback;
			HCURSOR									m_cursor;
			HICON 									m_icon;
			
			spec::win32::Window::vector_t 			m_lastSize;
			bool 									m_keyRepeatEnabled;
			bool 									m_resizing;
			bool 									m_mouseInside;
			bool 									m_mouseGrabbed;
			uint16_t 								m_surrogate;
			bool 									m_syncParse;
			bool 									m_asyncParse;
		public:
		// 	C-tors
			UF_API_CALL Window();
			UF_API_CALL Window( spec::win32::Window::handle_t );
			UF_API_CALL Window( const spec::win32::Window::vector_t& size, const spec::win32::Window::title_t& title = L"Window" );
			/*virtual*/ void UF_API_CALL create( const spec::win32::Window::vector_t& size, const spec::win32::Window::title_t& title = L"Window" );
		// 	D-tors
			/*virtual*/ ~Window();
			void UF_API_CALL terminate();
		// 	Gets
			spec::win32::Window::handle_t UF_API_CALL getHandle() const;
			/*virtual*/ spec::win32::Window::vector_t UF_API_CALL getPosition() const;
			/*virtual*/ spec::win32::Window::vector_t UF_API_CALL getSize() const;
		// 	Attribute modifiers
			/*virtual*/ void UF_API_CALL setPosition( const spec::win32::Window::vector_t& position );
			/*virtual*/ void UF_API_CALL centerWindow();
			/*virtual*/ void UF_API_CALL setMousePosition( const spec::win32::Window::vector_t& position );
			/*virtual*/ void UF_API_CALL setSize( const spec::win32::Window::vector_t& size );
			/*virtual*/ void UF_API_CALL setTitle( const spec::win32::Window::title_t& title );
			/*virtual*/ void UF_API_CALL setIcon( const spec::win32::Window::vector_t& size, uint8_t* pixels );
			/*virtual*/ void UF_API_CALL setVisible( bool visibility );
			/*virtual*/ void UF_API_CALL setCursorVisible( bool visibility );
			/*virtual*/ void UF_API_CALL setKeyRepeatEnabled( bool state );
			/*virtual*/ void UF_API_CALL setMouseGrabbed(bool state);
		
			/*virtual*/ void UF_API_CALL requestFocus();
			/*virtual*/ bool UF_API_CALL hasFocus() const;
		// 	Update
			/*virtual*/ void UF_API_CALL processEvents();
			static /*virtual*/ bool UF_API_CALL isKeyPressed(const std::string&);
			bool UF_API_CALL pollEvents( bool block = false );
		// 	Win32 specific functions
			void UF_API_CALL registerWindowClass();
			void UF_API_CALL processEvent(UINT message, WPARAM wParam, LPARAM lParam);
			void UF_API_CALL grabMouse(bool state);
			
			void UF_API_CALL setTracking(bool state);
			void UF_API_CALL switchToFullscreen();
			
			static std::string UF_API_CALL getKey(WPARAM key, LPARAM flags);
		//	static bool UF_API_CALL isKeyPressed( const std::string& key );
			static LRESULT CALLBACK globalOnEvent(spec::win32::Window::handle_t handle, UINT message, WPARAM wParam, LPARAM lParam);
		};
	}
	typedef spec::win32::Window Window;
/*
	namespace imp {
		typedef spec::win32::Window window_t;
	}
*/
}
#elif defined(UF_USE_SFML) && UF_USE_SFML == 1
#include <uf/ext/sfml/window.h>
namespace spec {
	typedef ext::sfml::Window Window;
}
#endif