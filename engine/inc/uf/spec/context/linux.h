#pragma once

#include "universal.h"

#if !UF_USE_OPENGL || UF_USE_OPENGL_GLDC
#include "unknown.h"
namespace spec {
	typedef spec::unknown::Context Context;
}
#else

#include <GL/glx.h>

namespace spec {
	namespace x11 {
		class UF_API_VAR Context : public spec::uni::Context {
		public:
			typedef ::Display*	  dc_t;
			typedef ::GLXContext  context_t;
		protected:
			dc_t	   m_display;
			context_t  m_context;
			::Window   m_window;
		public:
			UF_API_CALL Context( uni::Context* shared, const Context::Settings& settings = Settings() );
			UF_API_CALL Context( uni::Context* shared, const Context::Settings& settings, const Context::window_t& window );
			UF_API_CALL Context( uni::Context* shared, const Context::Settings& settings, unsigned int width, unsigned int height );
			~Context();

			virtual void UF_API_CALL terminate();

			virtual bool UF_API_CALL makeCurrent();
			virtual void UF_API_CALL setVerticalSyncEnabled(bool enabled);
			virtual void UF_API_CALL display();
		protected:
			virtual void UF_API_CALL create( uni::Context* shared );
		};
	}
	typedef spec::x11::Context Context;
}
#endif