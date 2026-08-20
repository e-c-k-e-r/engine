#pragma once

#include <uf/config.h>
#include "universal.h"

#if UF_ENV_LINUX
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>

#if UF_USE_VULKAN
	#include <vulkan/vulkan.h>
	// #include <vulkan/vulkan_xlib.h>
#elif UF_USE_OPENGL
	// #include <GL/glx.h>
#endif

// these macros interfere with other things
#ifdef Success
	#undef Success
#endif
#ifdef None
	#undef None
#endif

namespace spec {
	namespace x11 {
		class UF_API Window : public spec::uni::Window {
		public:
			typedef ::Window				handle_t;
			typedef void*					context_t;

		protected:
			Display*						m_display;
			int								m_screen;
			handle_t						m_handle;
			GC								m_gc;
			XIM								m_xim;
			XIC								m_xic;
			context_t*						m_context;

			vector_t						m_lastSize;
			bool							m_keyRepeatEnabled;
			bool							m_resizing;
			bool							m_mouseGrabbed;
			bool							m_syncParse;
			bool							m_asyncParse;

		public:
			UF_API_CALL Window();
			UF_API_CALL Window( const vector_t& size, const title_t& title = "Window" );
			~Window();

			void UF_API_CALL create(const vector_t& size, const title_t& title = "Window");
			void UF_API_CALL terminate();

			Display* UF_API_CALL getDisplay() const;
			handle_t UF_API_CALL getHandle() const;
			vector_t UF_API_CALL getPosition() const;
			vector_t UF_API_CALL getSize() const;
			size_t UF_API_CALL getRefreshRate() const;

			void UF_API_CALL setPosition(const vector_t& position);
			void UF_API_CALL centerWindow();
			void UF_API_CALL setMousePosition(const vector_t& position);
			vector_t UF_API_CALL getMousePosition();
			void UF_API_CALL setSize(const vector_t& size);
			void UF_API_CALL setTitle(const title_t& title);
			void UF_API_CALL setIcon(const vector_t& size, uint8_t* pixels);
			void UF_API_CALL setVisible(bool visibility);
			void UF_API_CALL setCursorVisible(bool visibility);
			void UF_API_CALL setKeyRepeatEnabled(bool state);
			void UF_API_CALL setMouseGrabbed(bool state);

			static bool UF_API_CALL isKeyPressed( const uf::stl::string& key );

			void UF_API_CALL requestFocus();
			bool UF_API_CALL hasFocus() const;

			void UF_API_CALL bufferInputs();
			void UF_API_CALL processEvents();
			bool UF_API_CALL pollEvents(bool block = false);
			void UF_API_CALL grabMouse(bool state);

			static pod::Vector2ui UF_API_CALL getResolution();
			void UF_API_CALL toggleFullscreen(bool borderless = false);

		#if UF_USE_VULKAN
			uf::stl::vector<uf::stl::string> UF_API_CALL getExtensions(bool validationEnabled);
			void UF_API_CALL createSurface(VkInstance instance, VkSurfaceKHR& surface);
		#endif

			void display();
		};
	}
	typedef spec::x11::Window Window;
}

namespace uf {
	using Window = spec::x11::Window;
}

#endif
