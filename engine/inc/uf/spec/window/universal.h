#pragma once

#include <uf/config.h>

#include <uf/utils/math/vector.h>
#include <uf/utils/hook/hook.h>
#include <uf/utils/string/string.h>
#include <uf/utils/memory/string.h>
#include <queue>

#if UF_USE_VULKAN
	// handles only; vulkan.h stays out of here on purpose (its platform branches pull in windowing headers)
	typedef struct VkInstance_T* VkInstance;
	typedef struct VkSurfaceKHR_T* VkSurfaceKHR;
#endif

namespace spec {
	namespace uni {
		class UF_API Window {
		public:
			typedef void* 									handle_t;
			typedef void* 									context_t;
			typedef uf::stl::string 						title_t;
			typedef pod::Vector2i 							vector_t;

			struct Event {
				uf::Hooks::name_t name; // ugh
				pod::Hook::userdata_t payload;
			};
		protected:
			std::queue<Event> m_events;
			static Window* 										live; // most recently constructed window; static helpers forward to it
		public:
			void pushEvent( const uf::Hooks::name_t& name, const pod::Hook::userdata_t& payload );
			template<typename T> void pushEvent( const uf::Hooks::name_t& name, const T& payload );
			static bool focused;

			UF_API_CALL Window();

			// runtime backend selection: null window when uf::headless, OS window otherwise
			static Window* UF_API_CALL create_instance( const vector_t& size, const title_t& title );

			virtual UF_API_CALL ~Window();
			virtual void UF_API_CALL create( const vector_t& size, const title_t& title = "Window" ) = 0;
			virtual void UF_API_CALL terminate() = 0;

			virtual handle_t UF_API_CALL getHandle() const = 0;
			virtual vector_t UF_API_CALL getPosition() const = 0;
			virtual vector_t UF_API_CALL getSize() const = 0;
			virtual size_t UF_API_CALL getRefreshRate() const = 0;

			virtual void UF_API_CALL setPosition( const vector_t& position ) = 0;
			virtual void UF_API_CALL centerWindow() = 0;
			virtual void UF_API_CALL setMousePosition( const vector_t& position ) = 0;
			virtual vector_t UF_API_CALL getMousePosition() = 0;
			virtual void UF_API_CALL setSize( const vector_t& size ) = 0;
			virtual void UF_API_CALL setTitle( const title_t& title ) = 0;
			virtual void UF_API_CALL setIcon( const vector_t& size, uint8_t* pixels ) = 0;
			virtual void UF_API_CALL setVisible( bool visibility ) = 0;
			virtual void UF_API_CALL setCursorVisible( bool visibility ) = 0;
			virtual void UF_API_CALL setKeyRepeatEnabled( bool state ) = 0;
			virtual void UF_API_CALL setMouseGrabbed( bool state ) = 0;

			virtual void UF_API_CALL requestFocus() = 0;
			virtual bool UF_API_CALL hasFocus() const = 0;

			virtual void UF_API_CALL bufferInputs() = 0;
			virtual void UF_API_CALL processEvents() = 0;
			virtual bool UF_API_CALL pollEvents( bool block = false ) = 0;
			virtual void UF_API_CALL grabMouse( bool state ) = 0;
			virtual void UF_API_CALL toggleFullscreen( bool borderless = false ) = 0;
			virtual void UF_API_CALL display() = 0;

		#if UF_USE_VULKAN
			virtual uf::stl::vector<uf::stl::string> UF_API_CALL getExtensions( bool validationEnabled ) = 0;
			virtual void UF_API_CALL createSurface( VkInstance instance, VkSurfaceKHR& surface ) = 0;
		#endif

			// kept static so existing call sites survive; they forward to the live window's virtuals
			static bool UF_API_CALL isKeyPressed( const uf::stl::string& key );
			static pod::Vector2ui UF_API_CALL getResolution();
			// the live window (null when none); engine-side accessors don't get to hold client::window
			static Window* UF_API_CALL get();
		protected:
			virtual bool UF_API_CALL isKeyPressed_v( const uf::stl::string& key ) = 0;
			virtual pod::Vector2ui UF_API_CALL getResolution_v() = 0;
		};
	}
}

#include "universal.inl"