#pragma once

#include <uf/config.h>
#include <vector>

#if !defined(EXT_STATIC)
	#if defined(UF_ENV_WINDOWS)
		// Windows compilers need specific (and different) keywords for export and import
		#define EXT_API_EXPORT __declspec(dllexport)
		#define EXT_API_IMPORT __declspec(dllimport)
		// For Visual C++ compilers, we also need to turn off this annoying C4251 warning
		#ifdef _MSC_VER
			#pragma warning(disable : 4251)
		#endif
	#else // Linux, FreeBSD, Mac OS X
		#if __GNUC__ >= 4
			// GCC 4 has special keywords for showing/hidding symbols,
			// the same keyword is used for both importing and exporting
			#define EXT_API_EXPORT __attribute__ ((__visibility__ ("default")))
			#define EXT_API_IMPORT __attribute__ ((__visibility__ ("default")))
		#else
			// GCC < 4 has no mechanism to explicitely hide symbols, everything's exported
			#define EXT_API_EXPORT
			#define EXT_API_IMPORT
		#endif
	#endif
#else
	// Static build doesn't need import/export macros
	#define EXT_API_EXPORT
	#define EXT_API_IMPORT
#endif

#ifdef EXT_EXPORTS
	#define EXT_API EXT_API_EXPORT
#else
	#define EXT_API EXT_API_IMPORT
#endif

#include <string>
#include <uf/utils/serialize/serializer.h>
namespace ext {
	extern bool EXT_API ready;
	extern std::vector<std::string> EXT_API arguments;
	extern uf::Serializer EXT_API config;
	
	extern void EXT_API load();
	extern void EXT_API initialize();
	extern bool EXT_API running();
	extern void EXT_API tick();
	extern void EXT_API render();
	extern void EXT_API terminate();
}