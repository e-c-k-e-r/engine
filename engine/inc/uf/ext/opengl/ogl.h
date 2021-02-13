#pragma once

#include <uf/config.h>
#include <GL/glew.h>
#if defined(UF_ENV_WINDOWS)
	// The Visual C++ version of gl.h uses WINGDIAPI and APIENTRY but doesn't define them
	#ifdef _MSC_VER
		#include <windows.h>
	#endif
	#include <GL/gl.h>
	#include <GL/glu.h>
	#include <GL/wglext.h>
#elif defined(UF_ENV_LINUX) || defined(UF_ENV_FREEBSD)
	#include <GL/gl.h>
	#include <GL/glu.h>
#elif defined(UF_ENV_MACOS)
	#include <OpenGL/gl.h>
	#include <OpenGL/glu.h>
#endif
#include <GL/glext.h>

#define GL_NULL_HANDLE NULL
#define GL_WHOLE_SIZE -1
#define GLhandle(x) size_t
#define GLenumerator(x) 0