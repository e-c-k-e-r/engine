#pragma once

#include <uf/config.h>

#include <uf/utils/memory/vector.h>
#include <uf/utils/memory/string.h>
#include <uf/utils/math/hash.h>
#include <stdint.h>
#include <picosha2.h>

namespace uf {
	namespace string {
		template<typename T> uf::stl::string sha256( const T& input ) { return picosha2::hash256_hex_string(input); }
	}

	namespace literals {
		constexpr uint32_t operator""_hash(const char* str, size_t) { return uf::algo::fnv1a(str); }
	}

	struct hashed_string {
		typedef uf::stl::string string_t;
		typedef size_t hash_t;

		size_t hash;
		uf::stl::string string;

		constexpr hashed_string() : hash(0), string("NULL") {}
		constexpr hashed_string(size_t h) : hash(h), string("NULL") {}
		constexpr hashed_string(const char* s) : hash(uf::algo::fnv1a(s)), string(s) {}
		inline hashed_string(const uf::stl::string& s) : hash(uf::algo::fnv1a(s)), string(s) {}

		constexpr operator size_t() const { return hash; }

		constexpr bool operator==(const hashed_string& other) const { return hash == other.hash; }
		constexpr bool operator!=(const hashed_string& other) const { return hash != other.hash; }
	};
}

namespace std {
	template<>
	struct hash<uf::hashed_string> {
		size_t operator()(const uf::hashed_string& hs) const noexcept {
			return hs.hash;
		}
	};
}

using namespace uf::literals;