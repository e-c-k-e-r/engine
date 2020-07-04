#pragma once

#include <uf/config.h>

#include <uf/utils/math/vector.h>
#include <uf/utils/hook/hook.h>
#include <uf/utils/string/string.h>
#include <string>
#include <queue>

#ifndef UF_USE_SFML
	#define UF_USE_SFML 0
#endif

namespace spec {
	namespace uni {
		class UF_API Window {
		public:
			typedef void* 									handle_t;
			typedef uf::String 								title_t;
			typedef pod::Vector2i 							vector_t;

			struct Events {
				template<typename Hook>
				struct Type {
					typedef typename Hook::argument_t 						Argument;
					typedef typename Hook::return_t 						Return;
					typedef typename Hook::name_t 							Name;
					typedef Argument 										argument_t;
					typedef Return 											return_t;
					typedef Name 											name_t;
					struct Event {
						Name name;
						Argument argument;
					};
					
					typedef std::queue<Event> 								events_t;
				};
				Type<uf::ReadableHook>::events_t readable;
				Type<uf::OptimalHook>::events_t optimal;
			};
		protected:
			Window::Events m_events;
	/*
			struct Event {
				std::string name, argument;
			};
			typedef Event 									event_t;
			typedef std::queue<spec::uni::Window::event_t> 	events_t;
		protected:
			spec::uni::Window::events_t m_events;
	*/
		public:
		// 	C-tors
			/*virtual*/ void UF_API_CALL create( const spec::uni::Window::vector_t& size, const spec::uni::Window::title_t& title = L"Window" ) {} ;// = 0;
		// 	D-tors
			/*virtual*/ void UF_API_CALL terminate();/* = 0;*/
		// 	Gets
			/*virtual*/ spec::uni::Window::vector_t UF_API_CALL getPosition() const;/* = 0;*/
			/*virtual*/ spec::uni::Window::vector_t UF_API_CALL getSize() const;/* = 0;*/
		// 	Attribute modifiers
			/*virtual*/ void UF_API_CALL setPosition( const spec::uni::Window::vector_t& position );/* = 0;*/
			/*virtual*/ void UF_API_CALL centerWindow();/* = 0;*/
			/*virtual*/ void UF_API_CALL setMousePosition( const spec::uni::Window::vector_t& position );/* = 0;*/
			/*virtual*/ void UF_API_CALL setSize( const spec::uni::Window::vector_t& size );/* = 0;*/
			/*virtual*/ void UF_API_CALL setTitle( const spec::uni::Window::title_t& title );/* = 0;*/
			/*virtual*/ void UF_API_CALL setIcon( const spec::uni::Window::vector_t& size, uint8_t* pixels );/* = 0;*/
			/*virtual*/ void UF_API_CALL setVisible( bool visibility );/* = 0;*/
			/*virtual*/ void UF_API_CALL setCursorVisible( bool visibility );/* = 0;*/
			/*virtual*/ void UF_API_CALL setKeyRepeatEnabled( bool state );/* = 0;*/
			/*virtual*/ void UF_API_CALL setMouseGrabbed( bool state );/* = 0;*/
		
			/*virtual*/ void UF_API_CALL requestFocus();/* = 0;*/
			/*virtual*/ bool UF_API_CALL hasFocus() const;/* = 0;*/
		// 	Update
			/*virtual*/ void UF_API_CALL processEvents();/* = 0;*/
			void UF_API_CALL pushEvent( const uf::ReadableHook::name_t& name, const uf::ReadableHook::argument_t& argument );
			void UF_API_CALL pushEvent( const uf::OptimalHook::name_t& name, const uf::OptimalHook::argument_t& argument );
			void UF_API_CALL pushEvent( const uf::ReadableHook::argument_t& serialized );
			void UF_API_CALL pushEvent( const uf::OptimalHook::argument_t& userdata );
			bool UF_API_CALL pollEvents( bool block = false );
			static bool UF_API_CALL isKeyPressed( const std::string& );
		};
	}
}