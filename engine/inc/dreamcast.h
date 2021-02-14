#pragma once

#include <string>
#include <sstream>
#include <cstdlib>
#include <math.h>

namespace std {
	template<typename T>
	std::string to_string( const T& val ) {
		std::stringstream ss;
		ss << val;
		return ss.str();
	}

	inline float strtof(const char* str, char** endptr) { return ::strtof(str, endptr); }
	inline double strtold(const char* str, char** endptr) { return ::strtold(str, endptr); }
	inline long long int strtoll(const char* str, char** endptr, int base) { return ::strtoll(str, endptr, base); } 
	inline unsigned long long int strtoull(const char* str, char** endptr, int base) { return ::strtoull(str, endptr, base); } 

	template<typename... Args>
	inline int snprintf( char* s, size_t n, const char* format, Args... args ) {
		return ::snprintf(s, n, format, args...);
	}

	inline unsigned long long stoull( const std::string& s, std::size_t* pos = 0, int base = 10 ) {
		std::wstring ws(s.begin(), s.end());
		return std::__cxx11::stoull( ws, pos, base );
	}
	inline int stoi( const std::string& s, std::size_t* pos = 0, int base = 10 ) {
		std::wstring ws(s.begin(), s.end());
		return std::__cxx11::stoi( ws, pos, base );
	}
	inline double cbrt( double x ) {
		return ::cbrt( x );
	}
}