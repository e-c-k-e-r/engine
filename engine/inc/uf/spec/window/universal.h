#pragma once

#include <uf/config.h>

#include <uf/utils/math/vector.h>
#include <uf/utils/hook/hook.h>
#include <uf/utils/string/string.h>
#include <uf/utils/memory/string.h>
#include <queue>

namespace spec {
	namespace uni {
		class UF_API Window {
		public:
			typedef void* 									handle_t;
			typedef uf::String 								title_t;
			typedef pod::Vector2i 							vector_t;

			struct Event {
				uf::stl::string name;
				pod::Hook::userdata_t payload;
			};
		protected:
		//	Window::Events m_events;
			std::queue<Event> m_events;
		public:
		// 	C-tors
			void create( const spec::uni::Window::vector_t& size, const spec::uni::Window::title_t& title = L"Window" ) {} ;// = 0;
		// 	D-tors
			void terminate();/* = 0;*/
		// 	Gets
			spec::uni::Window::vector_t getPosition() const;/* = 0;*/
			spec::uni::Window::vector_t getSize() const;/* = 0;*/
			size_t getRefreshRate() const;/* = 0;*/
		// 	Attribute modifiers
			void setPosition( const spec::uni::Window::vector_t& position );/* = 0;*/
			void centerWindow();/* = 0;*/
			void setMousePosition( const spec::uni::Window::vector_t& position );/* = 0;*/
			void setSize( const spec::uni::Window::vector_t& size );/* = 0;*/
			void setTitle( const spec::uni::Window::title_t& title );/* = 0;*/
			void setIcon( const spec::uni::Window::vector_t& size, uint8_t* pixels );/* = 0;*/
			void setVisible( bool visibility );/* = 0;*/
			void setCursorVisible( bool visibility );/* = 0;*/
			void setKeyRepeatEnabled( bool state );/* = 0;*/
			void setMouseGrabbed( bool state );/* = 0;*/
		
			void requestFocus();/* = 0;*/
			bool hasFocus() const;/* = 0;*/
		// 	Update
			void bufferInputs();/* = 0;*/
			void processEvents();/* = 0;*/

			void pushEvent( const uf::Hooks::name_t& name, const uf::stl::string& payload );
			void pushEvent( const uf::Hooks::name_t& name, const ext::json::Value& payload );
			void pushEvent( const uf::Hooks::name_t& name, const uf::Serializer& payload );
			void pushEvent( const uf::Hooks::name_t& name, const uf::Hooks::argument_t& payload );
			void pushEvent( const uf::Hooks::argument_t& payload );
			template<typename T> void pushEvent( const uf::Hooks::name_t& name, const T& payload );

			bool pollEvents( bool block = false );
			static bool isKeyPressed( const uf::stl::string& );
		};
	}
}

#include "universal.inl"