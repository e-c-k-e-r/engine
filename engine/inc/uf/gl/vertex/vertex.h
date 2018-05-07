#pragma once

#include <uf/config.h>
#include <uf/gl/gl.h>

#if defined(UF_USE_VULKAN)
	#include "vulkan.h"
#elif defined(UF_USE_OPENGL)
	#include "opengl.h"
#elif defined(UF_USE_DIRECTX)
	#include "directx.h"
#endif
namespace uf {
	typedef Vertices<int, 1> Vertices1i;
	typedef Vertices<uint, 1> Vertices1ui;
	typedef Vertices<float, 1> Vertices1f;

	typedef Vertices<int, 2> Vertices2i;
	typedef Vertices<uint, 2> Vertices2ui;
	typedef Vertices<float, 2> Vertices2f;

	typedef Vertices<int, 3> Vertices3i;
	typedef Vertices<uint, 3> Vertices3ui;
	typedef Vertices<float, 3> Vertices3f;

	typedef Vertices<int, 4> Vertices4i;
	typedef Vertices<uint, 4> Vertices4ui;
	typedef Vertices<float, 4> Vertices4f;
}
