#pragma once

#include <uf/config.h>

#include <uf/utils/memory/vector.h>
#include <uf/utils/memory/string.h>
#include <stdint.h>
#include <picosha2.h>

namespace uf {
	namespace string {
		template<typename T>
		uf::stl::string sha256( const T& input ) { return picosha2::hash256_hex_string(input); }

		constexpr uint32_t fnv1a(const char* str, uint32_t hash = 2166136261u) { return *str ? fnv1a(str + 1, (hash ^ static_cast<uint32_t>(*str)) * 16777619u) : hash; }

		static inline uint32_t fnv1a( const uf::stl::string& str ) { return fnv1a(str.c_str()); }
		static inline uint32_t fnv1a( const uf::stl::string_view str ) {
			uint32_t hash = 2166136261u;
			for ( char c : str ) hash = (hash ^ static_cast<uint32_t>(c)) * 16777619u;
			return hash;
		}
	}
	namespace literals {
		constexpr uint32_t operator""_hash(const char* str, size_t) { return uf::string::fnv1a(str); }
	}
}

using namespace uf::literals;