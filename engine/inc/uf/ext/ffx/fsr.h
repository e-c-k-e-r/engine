#pragma once

#include <uf/config.h>

#include <uf/utils/math/matrix.h>

#if UF_USE_FFX_FSR
namespace ext {
	namespace fsr {
		extern pod::Vector2f UF_API jitter;
		extern float UF_API sharpness;
		extern bool UF_API initialized;

		void UF_API initialize();
		void UF_API tick();
		void UF_API render();
		void UF_API terminate();
		
		pod::Matrix4f UF_API getJitterMatrix();
	}
}
#endif