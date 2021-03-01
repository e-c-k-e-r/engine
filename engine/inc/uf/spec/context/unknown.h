#pragma once

#include "context.h"

namespace spec {
	namespace unknown {
		class UF_API_VAR Context : public spec::uni::Context {
		public:
			UF_API_CALL Context( uni::Context* shared, const Context::Settings& settings = Settings() ) {}
			UF_API_CALL Context( uni::Context* shared, const Context::Settings& settings, const Context::window_t& window ) {}
			UF_API_CALL Context( uni::Context* shared, const Context::Settings& settings, unsigned int width, unsigned int height ) {}
			~Context() {}

			virtual void UF_API_CALL terminate() {}

			virtual bool UF_API_CALL makeCurrent() { return false; }
			virtual void UF_API_CALL setVerticalSyncEnabled(bool enabled) {}
			virtual void UF_API_CALL display() {}
		protected:
			virtual void UF_API_CALL create( uni::Context* shared ){}
		};
	}
	typedef spec::unknown::Context Context;
}