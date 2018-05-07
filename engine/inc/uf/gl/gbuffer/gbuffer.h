#pragma once

#include <uf/config.h>
#include <uf/gl/gl.h>
#include <uf/gl/texture/texture.h>

#if defined(UF_USE_OPENGL)
	#include "opengl.h"
#elif defined(UF_USE_DIRECTX)
	#include "directx.h"
#endif

