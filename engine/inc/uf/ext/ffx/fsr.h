#pragma once

#include <uf/config.h>

#include <uf/utils/math/matrix.h>
#include <uf/utils/renderer/renderer.h>

#if UF_USE_FFX_FSR || UF_USE_FFX_SDK
namespace ext {
	namespace fsr {
		extern bool UF_API initialized;
		extern bool UF_API frameUpscale;
		extern bool UF_API frameInterpolation;
		
		extern uf::stl::string UF_API preset;

		extern float UF_API sharpness;
		extern pod::Vector2f UF_API jitter;
		extern float UF_API jitterScale;

		void UF_API initialize();
		void UF_API tick();
		void UF_API render();
		void UF_API terminate();

		uf::renderer::Texture& getRenderTarget();
		
		pod::Matrix4f UF_API getJitterMatrix();
	}
}
#endif