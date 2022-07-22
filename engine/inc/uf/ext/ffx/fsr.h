#pragma once

#include <uf/config.h>

#if UF_USE_FFX_FSR
namespace ext {
	namespace fsr {
		extern bool UF_API initialized;

		void UF_API initialize();
		void UF_API tick();
		void UF_API terminate();
	}
}
#endif