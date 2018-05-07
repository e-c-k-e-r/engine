#pragma once

#include <uf/config.h>
#if defined(UF_USE_SFML)

#include <uf/spec/window/universal.h>
#include <uf/spec/context/universal.h>
#include <SFML/Window.hpp>

namespace ext {
	namespace sfml {
		class UF_API Window : public spec::uni::Window {
		public:
			typedef sf::Window 						handle_t;
			typedef spec::uni::Window::title_t 		title_t;
			typedef spec::uni::Window::vector_t 	vector_t;
		protected:
			ext::sfml::Window::handle_t m_handle;
			bool m_syncParse = false;
			bool m_asyncParse = true;
		public:
		// 	C-tors
			UF_API_CALL Window();
			UF_API_CALL Window( ext::sfml::Window::handle_t handle );
			UF_API_CALL Window( const spec::uni::Window::vector_t& size, const spec::uni::Window::title_t& title = L"Window", const spec::uni::Context::Settings& settings = spec::uni::Context::Settings() );
			/*virtual*/ void UF_API_CALL create( const spec::uni::Window::vector_t& size, const spec::uni::Window::title_t& title = L"Window", const spec::uni::Context::Settings& settings = spec::uni::Context::Settings() );
		// 	D-tors
			/*virtual*/ ~Window();
			void UF_API_CALL terminate();
		// 	Gets
			const ext::sfml::Window::handle_t& UF_API_CALL getHandle() const;
			/*virtual*/ ext::sfml::Window::vector_t UF_API_CALL getPosition() const;
			/*virtual*/ ext::sfml::Window::vector_t UF_API_CALL getSize() const;
		// 	Attribute modifiers
			/*virtual*/ void UF_API_CALL setPosition( const ext::sfml::Window::vector_t& position );
			/*virtual*/ void UF_API_CALL centerWindow();
			/*virtual*/ void UF_API_CALL setMousePosition( const ext::sfml::Window::vector_t& position );
			/*virtual*/ void UF_API_CALL setSize( const ext::sfml::Window::vector_t& size );
			/*virtual*/ void UF_API_CALL setTitle( const ext::sfml::Window::title_t& title );
			/*virtual*/ void UF_API_CALL setIcon( const ext::sfml::Window::vector_t& size, uint8_t* pixels );
			/*virtual*/ void UF_API_CALL setVisible( bool visibility );
			/*virtual*/ void UF_API_CALL setCursorVisible( bool visibility );
			/*virtual*/ void UF_API_CALL setKeyRepeatEnabled( bool state );
			/*virtual*/ void UF_API_CALL setMouseGrabbed( bool state );
			static /*virtual*/ bool UF_API_CALL isKeyPressed( const std::string& );
		
			/*virtual*/ void UF_API_CALL requestFocus();
			/*virtual*/ bool UF_API_CALL hasFocus() const;
		// 	Update
			/*virtual*/ void UF_API_CALL processEvents();
			/*virtual*/ void UF_API_CALL display();
		};
	}
}

#endif