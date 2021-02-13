#pragma once

#include <uf/config.h>

#if UF_USE_OPENGL
	#include <uf/ext/opengl/ogl.h>
#endif

#if UF_USE_SFML
	#include <uf/spec/window/universal.h>
#else
	#include <uf/spec/window/window.h>
#endif

namespace spec {
	namespace uni {
		class UF_API_VAR Context {
		public:
			#if UF_USE_SFML
				typedef spec::uni::Window window_t;
			#else
				typedef spec::Window window_t;
			#endif
			struct Settings {
				unsigned int depthBits;
				unsigned int stencilBits;
				unsigned int bitsPerPixel;
				unsigned int antialiasingLevel;
				unsigned int majorVersion;
				unsigned int minorVersion;

				explicit Settings(unsigned int depth = 24, unsigned int stencil = 4, unsigned int antialiasing = 0, unsigned int bitsPerPixel = 8, unsigned int major = 2, unsigned int minor = 0) :
					depthBits 			(depth),
					stencilBits 		(stencil),
					bitsPerPixel 		(bitsPerPixel),
					antialiasingLevel 	(antialiasing),
					majorVersion 		(major),
					minorVersion 		(minor)
				{

				}
			};
		protected:
			Context::window_t::handle_t 	m_window;
			bool							m_ownsWindow;
			Context::Settings 				m_settings;
		public:
			UF_API_CALL Context();
			UF_API_CALL Context( Context::window_t::handle_t window, bool ownsWindow, const Context::Settings& settings );
			virtual ~Context();
			virtual void UF_API_CALL create( uni::Context* shared ) = 0;
			virtual void UF_API_CALL terminate() = 0;

			virtual bool UF_API_CALL makeCurrent() = 0;
			virtual void UF_API_CALL setVerticalSyncEnabled(bool enabled) = 0;
			virtual void UF_API_CALL display() = 0;
			virtual int UF_API_CALL evaluateFormat( const Context::Settings& settings, int colorBits, int depthBits, int stencilBits, int antialiasing );

		// 
			bool UF_API_CALL setActive(bool active);
			const Context::Settings& UF_API_CALL getSettings() const;
			void UF_API_CALL setSettings( const Context::Settings& );
		//
			void UF_API_CALL initialize();
			static void UF_API_CALL globalInit();
			static void UF_API_CALL globalCleanup();
			static void UF_API_CALL ensureContext();
			static Context* UF_API_CALL create();
			static Context* UF_API_CALL create(const Context::Settings& settings, const Context::window_t& owner);
			static Context* UF_API_CALL create(const Context::Settings& settings, unsigned int width, unsigned int height);
		//
		};
	}
}