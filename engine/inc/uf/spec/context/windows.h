#pragma once

#include "universal.h"

#if !UF_USE_OPENGL || UF_USE_OPENGL_GLDC
#include "unknown.h"
namespace spec {
	typedef spec::unknown::Context Context;
}
#else
namespace spec {
	namespace win32 {
		class UF_API_VAR Context : public spec::uni::Context {
		public:
			typedef HDC 	dc_t;
			typedef HGLRC 	context_t;
		protected:
			Context::dc_t		m_deviceContext;
			Context::context_t 	m_context;
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
	typedef spec::win32::Context Context;
}
#endif