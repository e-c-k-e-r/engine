#pragma once

#if defined(UF_ENV)
	#warning Specific variables already defined; undefining...

	#undef UF_ENV
	#undef UF_ENV_WINDOWS
	#undef UF_ENV_LINUX
	#undef UF_ENV_MACOS
	#undef UF_ENV_FREEBSD
	#undef UF_ENV_DREAMCAST
	#undef UF_ENV_UNKNOWN
#endif

#if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
	// Windows
	#define UF_ENV "Windows"
	#define UF_ENV_WINDOWS 1
	#define UF_ENV_HEADER "windows.h"
	#if defined(__CYGWIN__)
		#define to_string(var) string(var)
	#endif
	#ifndef _WIN32_WINNT
		#define _WIN32_WINNT 0x0600
	#endif
	#ifndef WINVER
		#define WINVER 0x0600
	#endif
	
	#define UF_IO_ROOT "./data/"
#elif defined(linux) || defined(__linux)
	// Linux
	#define UF_ENV "Linux"
	#define UF_ENV_LINUX 1
	#define UF_ENV_HEADER "linux.h"
	
	#define UF_IO_ROOT "./data/"
#elif defined(__APPLE__) || defined(MACOSX) || defined(macintosh) || defined(Macintosh)
	// MacOS
	#define UF_ENV "OSX"
	#define UF_ENV_OSX 1
	#define UF_ENV_HEADER "osx.h"
	
	#define UF_IO_ROOT "./data/"
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
	// FreeBSD
	#define UF_ENV "FreeBSD"
	#define UF_ENV_FREEBSD 1
	#define UF_ENV_HEADER "freebsd.h"
	
	#define UF_IO_ROOT "./data/"
#elif defined(__sh__)
	// Dreamcast
	#define UF_ENV "Dreamcast"
	#define UF_ENV_DREAMCAST 1
	#define UF_ENV_HEADER "dreamcast.h"
	#include UF_ENV_HEADER

	#define _arch_dreamcast

	#define UF_IO_ROOT "/cd/"
#else
	// Unsupported system
	#define UF_ENV "Unknown"
	#define UF_ENV_UNKNOWN 1
	#define UF_ENV_HEADER "unknown.h"
	#warning Using "unknown"
 	#error No support
#endif

#if !defined(UF_STATIC)
	#if defined(UF_ENV_WINDOWS)
		// Windows compilers need specific (and different) keywords for export and import
		#define UF_API_EXPORT __declspec(dllexport)
		#define UF_API_IMPORT __declspec(dllimport)
		// For Visual C++ compilers, we also need to turn off this annoying C4251 warning
		#ifdef _MSC_VER
			#pragma warning(disable : 4251)
		#endif
	#else // Linux, FreeBSD, Mac OS X
		#if __GNUC__ >= 4
			// GCC 4 has special keywords for showing/hidding symbols,
			// the same keyword is used for both importing and exporting
			#define UF_API_EXPORT __attribute__ ((__visibility__ ("default")))
			#define UF_API_IMPORT __attribute__ ((__visibility__ ("default")))
		#else
			// GCC < 4 has no mechanism to explicitely hide symbols, everything's exported
			#define UF_API_EXPORT
			#define UF_API_IMPORT
		#endif
	#endif
#else
	// Static build doesn't need import/export macros
	#define UF_API_EXPORT
	#define UF_API_IMPORT
#endif

#ifdef UF_EXPORTS
	#define UF_API UF_API_EXPORT
#else
	#define UF_API UF_API_IMPORT
#endif

// Legacy support
#define UF_API_VAR UF_API
#define UF_API_CALL //__cdecl

#ifdef UF_DISABLE_ALIGNAS
	#define alignas(X)
#endif

#if UF_ALIGN_FOR_SIMD
	#define ALIGN16 alignas(16)
#else
	#define ALIGN16
#endif

#include "macros.h"
#include "simd.h"
#include "helpers.inl"