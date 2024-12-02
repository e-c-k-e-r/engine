#pragma once

#include <string>
#include <sstream>
#include <cstdlib>
#include <math.h>

namespace std {
#if 1
	inline double log2( double n ) { return log(n) / log(2); }
	inline double cbrt( double x ) { return ::cbrt( x ); }
#endif

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

#if 0
	inline double strtold(const char* str, char** endptr) { return ::strtold(str, endptr); }
#endif
#if 0
	inline int stoi( const std::string& s, std::size_t* pos = 0, int base = 10 ) {
		std::wstring ws(s.begin(), s.end());
		return std::__cxx11::stoi( ws, pos, base );
	}
	inline float strtof(const char* str, char** endptr) { return ::strtof(str, endptr); }
	inline long long int strtoll(const char* str, char** endptr, int base) { return ::strtoll(str, endptr, base); } 
	inline unsigned long long int strtoull(const char* str, char** endptr, int base) { return ::strtoull(str, endptr, base); }
	inline unsigned long long stoull( const std::string& s, std::size_t* pos = 0, int base = 10 ) {
		std::wstring ws(s.begin(), s.end());
		return std::__cxx11::stoull( ws, pos, base );
	}
	template<typename... Args>
	inline int snprintf( char* s, size_t n, const char* format, Args... args ) {
		return ::snprintf(s, n, format, args...);
	}
#endif
}