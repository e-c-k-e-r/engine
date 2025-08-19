#pragma once

#include <string>
#include <sstream>
#include <cstdlib>
#include <math.h>

// to-do: check if this is necessary

namespace std {
#ifndef UF_OVERRIDE_STD_TOSTRING
	#define UF_OVERRIDE_STD_TOSTRING 1
#endif

#define RP3D_NO_EXCEPTIONS 1

#if UF_OVERRIDE_STD_TOSTRING
	#ifdef RP3D_DEFINE_TOSTRING
		#undef RP3D_DEFINE_TOSTRING
	#endif

	template<typename T>
	std::string to_string( const T& val ) {
		std::stringstream ss;
		ss << val;
		return ss.str();
	}
#endif
}