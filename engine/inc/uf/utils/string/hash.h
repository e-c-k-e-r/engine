#pragma once

#include <uf/config.h>

#include <uf/utils/memory/vector.h>
#include <uf/utils/memory/string.h>
#include <uf/utils/math/hash.h>
#include <stdint.h>
#include <picosha2.h>

namespace uf {
	namespace string {
		template<typename T>
		uf::stl::string sha256( const T& input ) {
			unsigned char raw_hash[32];
			picosha2::hash256(input.begin(), input.end(), raw_hash, raw_hash + 32);

			uf::stl::string result;
			result.reserve(64);
			const char hex_chars[] = "0123456789abcdef";
			for (int i = 0; i < 32; ++i) {
				result.push_back(hex_chars[(raw_hash[i] >> 4) & 0x0F]);
				result.push_back(hex_chars[raw_hash[i] & 0x0F]);
			}

			return result;
		}
	}

	namespace literals {
		constexpr uint32_t operator""_hash(const char* str, size_t) { return uf::algo::fnv1a(str); }
	}

	struct hashed_string {
		typedef uf::stl::string string_t;
		typedef size_t hash_t;

		size_t hash;

		constexpr hashed_string() : hash(0) {}
		constexpr hashed_string(size_t h) : hash(h) {}
		constexpr hashed_string(const char* s) : hash(uf::algo::fnv1a(s)) {}
		template<size_t N> constexpr hashed_string(const char (&s)[N]) : hash(uf::algo::fnv1a(s)) {}
		inline hashed_string(const uf::stl::string& s) : hash(uf::algo::fnv1a(s)) {}
		inline hashed_string(const uf::stl::string_view& s) : hash(uf::algo::fnv1a(s)) {}

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