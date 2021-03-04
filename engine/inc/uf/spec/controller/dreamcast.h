#pragma once

#include <uf/config.h>
#include "universal.h"

#if UF_ENV_DREAMCAST

namespace spec {
	namespace dreamcast {
		namespace controller {
			void UF_API initialize();
			void UF_API tick();
			void UF_API terminate();

			bool UF_API connected( size_t = 0 );
			bool UF_API pressed( const std::string&, size_t = 0 );
			float UF_API analog( const std::string&, size_t = 0 );
		}
	}
	namespace controller = dreamcast::controller;
}

#endif