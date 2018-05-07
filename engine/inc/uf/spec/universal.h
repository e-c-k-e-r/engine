#pragma once
#include <uf/config.h>

// Moved to uf/config.h
/*

#if defined(UF_ENV)
	#warning Specific variables already defined; undefining...

	#undef UF_ENV
	#undef UF_ENV_WINDOWS
	#undef UF_ENV_UNKNOWN
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
	#define UF_ENV "Windows"
	#define UF_ENV_WINDOWS 1
	#define UF_ENV_HEADER "windows.h"
	#if defined(__CYGWIN__)
		#define to_string(var) string(var)
	#endif
//	#warning Using "Windows"
#else
	#define UF_ENV "Unknown"
	#define UF_ENV_UNKNOWN 1
	#define UF_ENV_HEADER "unknown.h"
//	#warning Using "unknown"
#endif

*/

#include UF_ENV_HEADER

// define some universal shit
// #define UF_USE_SFML 1