#pragma once

#if defined(UF_USE_VULKAN) && UF_USE_VULKAN == 1
	#include "vulkan.h"
#elif defined(UF_USE_OPENGL) && UF_USE_OPENGL == 1
	#include "opengl.h"
#endif