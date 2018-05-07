#pragma once

#include <uf/config.h>
#if defined(UF_USE_VULKAN)
	#include <uf/ext/vulkan/vulkan.h>
#elif defined(UF_USE_OPENGL)
	#include <uf/ext/ogl/ogl.h>
#elif defined(UF_USE_DIRECTX)
	#include <uf/ext/directx/directx.h>
#endif

#pragma once

namespace ext {
	class UF_API GL {
	protected:
		bool m_initialized;
	public:
		UF_API_CALL GL();
		bool UF_API_CALL initialize();
		void UF_API_CALL terminate();
		const GLubyte* getError();
	};
	extern UF_API ext::GL gl;
}
