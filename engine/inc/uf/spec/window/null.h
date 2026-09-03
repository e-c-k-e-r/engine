#pragma once

#include <uf/config.h>
#include "universal.h"

// these macros (from X11, pulled in by vulkan.h when the platform defines are set) interfere with other things
#ifdef Success
	#undef Success
#endif
#ifdef None
	#undef None
#endif

namespace spec {
	namespace null {
		// No windowing system; gives the engine a size to render at and
		// (when the driver supports it) a headless surface to render into
		class UF_API Window : public spec::uni::Window {
		protected:
			vector_t					m_size;
			title_t						m_title;
			vector_t					m_mousePosition;

		public:
			UF_API_CALL Window();
			UF_API_CALL Window( const vector_t& size, const title_t& title = "Window" );
			~Window();

			void UF_API_CALL create( const vector_t& size, const title_t& title = "Window" );
			void UF_API_CALL terminate();

			handle_t UF_API_CALL getDisplay() const;
			handle_t UF_API_CALL getHandle() const;
			vector_t UF_API_CALL getPosition() const;
			vector_t UF_API_CALL getSize() const;
			size_t UF_API_CALL getRefreshRate() const;

			void UF_API_CALL setPosition( const vector_t& position );
			void UF_API_CALL centerWindow();
			void UF_API_CALL setMousePosition( const vector_t& position );
			vector_t UF_API_CALL getMousePosition();
			void UF_API_CALL setSize( const vector_t& size );
			void UF_API_CALL setTitle( const title_t& title );
			void UF_API_CALL setIcon( const vector_t& size, uint8_t* pixels );
			void UF_API_CALL setVisible( bool visibility );
			void UF_API_CALL setCursorVisible( bool visibility );
			void UF_API_CALL setKeyRepeatEnabled( bool state );
			void UF_API_CALL setMouseGrabbed( bool state );

			void UF_API_CALL requestFocus();
			bool UF_API_CALL hasFocus() const;

			void UF_API_CALL bufferInputs();
			void UF_API_CALL processEvents();
			bool UF_API_CALL pollEvents( bool block = false );
			void UF_API_CALL grabMouse( bool state );

			void UF_API_CALL toggleFullscreen( bool borderless = false );

		#if UF_USE_VULKAN
			uf::stl::vector<uf::stl::string> UF_API_CALL getExtensions( bool validationEnabled );
			void UF_API_CALL createSurface( VkInstance instance, VkSurfaceKHR& surface );
		#endif

			void display();

		protected:
			bool UF_API_CALL isKeyPressed_v( const uf::stl::string& key );
			pod::Vector2ui UF_API_CALL getResolution_v();
		};
	}
}
