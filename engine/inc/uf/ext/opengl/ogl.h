#pragma once

#include <uf/config.h>
#if defined(UF_ENV_WINDOWS)
	// The Visual C++ version of gl.h uses WINGDIAPI and APIENTRY but doesn't define them
	#ifdef _MSC_VER
		#include <windows.h>
	#endif
	#include <GL/glew.h>
	#include <GL/gl.h>
	#include <GL/glu.h>
	#include <GL/wglext.h>
	#include <GL/glext.h>
#elif defined(UF_ENV_LINUX) || defined(UF_ENV_FREEBSD)
	#include <GL/gl.h>
	#include <GL/glu.h>
#elif defined(UF_ENV_MACOS)
	#include <OpenGL/gl.h>
	#include <OpenGL/glu.h>
#elif defined(UF_ENV_DREAMCAST)
	#include <GL/gl.h>
	#include <GL/glu.h>
	#include <GL/glkos.h>

	#define GL_NONE 0
	#define GLsizeiptr GLsizei
#endif

#define GL_NULL_HANDLE NULL
#define GL_WHOLE_SIZE -1
#define GLhandle(x) size_t
#define GLenumerator(x) 0

#define GL_DEBUG_MESSAGE(...)\
	uf::iostream << "[" << __FUNCTION__ << "@" << __FILE__ ":" << __LINE__ << "] " << __VA_ARGS__ << "\n"

#define GL_VALIDATION_MESSAGE(...)\
	if ( ext::opengl::settings::validation ) GL_DEBUG_MESSAGE(__VA_ARGS__);
	