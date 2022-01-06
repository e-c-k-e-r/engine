#pragma once

#include <uf/config.h>
#if UF_USE_GLEW
	#include <GL/glew.h>
#endif
#if UF_USE_OPENGL_GLDC
	#include <GLdc/gl.h>
	#include <GLdc/glu.h>
	#include <GLdc/glkos.h>
	#include <GLdc/glext.h>

	#define GL_NONE 0
	#define GLsizeiptr GLsizei
	
	#define UF_USE_OPENGL_12 1
	#define UF_USE_OPENGL_FIXED_FUNCTION 1
#elif defined(UF_ENV_WINDOWS)
	// The Visual C++ version of gl.h uses WINGDIAPI and APIENTRY but doesn't define them
	#ifdef _MSC_VER
		#include <windows.h>
	#endif
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
	#include <GL/glut.h>
	#include <GL/glext.h>

	#define glIsTexture(x) ( x != 0 )
	#define GL_NONE 0
	#define GLsizeiptr GLsizei
	
	#define UF_USE_OPENGL_12 1
	#define UF_USE_OPENGL_FIXED_FUNCTION 1

	#define glClientActiveTexture glClientActiveTextureARB
	#define glActiveTexture glActiveTextureARB
	#define glCompressedTexImage2DARB glCompressedTexImage2D
#endif

#include <typeinfo>

#define GL_NULL_HANDLE 0
#define GL_WHOLE_SIZE -1
#define GLhandle(x) size_t
#define GLenumerator(x) 0

#define UF_USE_OPENGL_IMMEDIATE_MODE 0

#define GL_DEBUG_MESSAGE(...) UF_MSG_ERROR(__VA_ARGS__);
#define GL_VALIDATION_MESSAGE(...) if ( ext::opengl::settings::validation ) GL_DEBUG_MESSAGE(__VA_ARGS__);

#if UF_ENV_DREAMCAST
	#define GL_VALIDATION_ENABLED 0
#else
	#define GL_VALIDATION_ENABLED 1
#endif

#if GL_VALIDATION_ENABLED
	#define GL_ERROR_CHECK(f) {																									\
		{f;}																													\
		if ( ext::opengl::settings::validation ) {																				\
			GLenum res = glGetError();																							\
			if (res != GL_NO_ERROR) GL_DEBUG_MESSAGE("[Validation Error] " << #f << ": " << ext::opengl::errorString( res )); 	\
		}																														\
	}
#else
	#define GL_ERROR_CHECK(f) {f;}
#endif
#if 0
	#define GL_MUTEX_LOCK() ext::opengl::mutex.lock();
	#define GL_MUTEX_UNLOCK() ext::opengl::mutex.unlock();
#else
	#define GL_MUTEX_LOCK();
	#define GL_MUTEX_UNLOCK();
#endif