#pragma once

#include <uf/utils/memory/vector.h>

namespace uf {
	namespace traits {
		template <typename T, typename = void>
		struct has_hash_method : std::false_type {};

		template <typename T>
		struct has_hash_method<T, std::void_t<decltype(std::declval<T>().hash())>> : std::true_type {};
	}
}

namespace uf {
	namespace algo {
		template<size_t> struct FNV1a;

		template<> struct FNV1a<4> {
			static constexpr uint32_t basis = 2166136261ul;
			static constexpr uint32_t prime = 16777619ul;
		};

		template<> struct FNV1a<8> {
			static constexpr uint64_t basis = 1469598103934665603ull;
			static constexpr uint64_t prime = 1099511628211ull;
		};

		using FNV = FNV1a<sizeof(size_t)>;

		template<typename T, std::enable_if_t<uf::traits::has_hash_method<T>::value, int> = 0>
		inline size_t fnv1a(const T& v, size_t hash = FNV::basis) {
			return (hash ^ v.hash()) * FNV::prime;
		}

		constexpr size_t fnv1a(const char* str, size_t hash = FNV::basis) {
			return *str ? fnv1a(str + 1, (hash ^ static_cast<size_t>(static_cast<uint8_t>(*str))) * FNV::prime) : hash;
		}

		constexpr size_t fnv1a(const uf::stl::string_view str, size_t hash = FNV::basis) {
			for (char c : str) hash = (hash ^ static_cast<size_t>(c)) * FNV::prime;
			return hash;
		}
		inline size_t fnv1a(const uf::stl::string& str, size_t hash = FNV::basis) {
			return fnv1a(uf::stl::string_view(str), hash);
		}

		template<typename T, std::enable_if_t<
			std::is_trivially_copyable_v<T> &&
			!std::is_array_v<T> &&
			!std::is_pointer_v<T> &&
			!uf::traits::has_hash_method<T>::value,
			int
		> = 0>
		inline size_t fnv1a(const T& v, size_t hash = FNV::basis) {
			const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&v);
			for (size_t i = 0; i < sizeof(T); ++i) {
				hash = (hash ^ static_cast<size_t>(bytes[i])) * FNV::prime;
			}
			return hash;
		}

		template<size_t N>
		constexpr size_t fnv1a(const char (&str)[N], size_t hash = FNV::basis) {
			return fnv1a(uf::stl::string_view(str, N - 1), hash);
		}

		template<typename T, std::enable_if_t<std::is_pointer_v<T>, int> = 0>
		inline size_t fnv1a( const T v, size_t hash = FNV::basis ) {
			return fnv1a(reinterpret_cast<size_t>(v), hash);
		}

		template<typename T>
		inline size_t fnv1a(const uf::stl::vector<T>& values, size_t hash = FNV::basis) {
			if constexpr (std::is_same_v<T, bool>) {
				for (bool b : values) {
					hash = (hash ^ static_cast<size_t>(b)) * FNV::prime;
				}
			} else if constexpr (std::is_trivially_copyable_v<T>) {
				const uint8_t* bytes = reinterpret_cast<const uint8_t*>(values.data());
				size_t len = values.size() * sizeof(T);
				for (size_t i = 0; i < len; ++i) {
					hash = (hash ^ static_cast<size_t>(bytes[i])) * FNV::prime;
				}
			} else {
				for ( const T& v : values ) {
					hash = (hash ^ fnv1a(v)) * FNV::prime;
				}
			}
			return hash;
		}

		struct hasher {
			using is_transparent = void;
			template<typename T> inline size_t operator()( const T& val ) const noexcept {
				return uf::algo::fnv1a(val);
			}
		};
	}


	inline void hash(size_t& seed) { }

	template <typename T, typename... Rest>
	inline void hash(size_t& seed, const T& v, Rest... rest) {
		seed ^= uf::algo::fnv1a(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		hash(seed, rest...);
	}
}