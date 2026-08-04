#pragma once

#include <uf/utils/memory/vector.h>
#include <cstdint>
#include <type_traits>

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
		template<size_t ByteSize> struct fnv_constants_by_size;

		template<> struct fnv_constants_by_size<4> {
			static constexpr uint32_t basis = 2166136261ul;
			static constexpr uint32_t prime = 16777619ul;
		};

		template<> struct fnv_constants_by_size<8> {
			static constexpr uint64_t basis = 1469598103934665603ull;
			static constexpr uint64_t prime = 1099511628211ull;
		};

		template<typename HashT>
		struct fnv_constants {
			using size_traits = fnv_constants_by_size<sizeof(HashT)>;
			static constexpr HashT basis = static_cast<HashT>(size_traits::basis);
			static constexpr HashT prime = static_cast<HashT>(size_traits::prime);
		};

		template<typename HashT>
		struct fnv1a_impl {
			using constants = fnv_constants<HashT>;

			template<typename T, std::enable_if_t<uf::traits::has_hash_method<T>::value, int> = 0>
			static inline HashT hash(const T& v, HashT seed = constants::basis) {
				return (seed ^ static_cast<HashT>(v.hash())) * constants::prime;
			}

			static constexpr HashT hash(const char* str, HashT seed = constants::basis) {
				return *str ? hash(str + 1, (seed ^ static_cast<HashT>(static_cast<uint8_t>(*str))) * constants::prime) : seed;
			}

			static constexpr HashT hash(const uf::stl::string_view str, HashT seed = constants::basis) {
				for (char c : str) {
					seed = (seed ^ static_cast<HashT>(c)) * constants::prime;
				}
				return seed;
			}

			static inline HashT hash(const uf::stl::string& str, HashT seed = constants::basis) {
				return hash(uf::stl::string_view(str), seed);
			}

			template<typename T, std::enable_if_t<
				std::is_trivially_copyable_v<T> &&
				!std::is_array_v<T> &&
				!std::is_pointer_v<T> &&
				!uf::traits::has_hash_method<T>::value,
				int
			> = 0>
			static inline HashT hash(const T& v, HashT seed = constants::basis) {
				const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&v);
				for (size_t i = 0; i < sizeof(T); ++i) {
					seed = (seed ^ static_cast<HashT>(bytes[i])) * constants::prime;
				}
				return seed;
			}

			template<size_t N>
			static constexpr HashT hash(const char (&str)[N], HashT seed = constants::basis) {
				return hash(uf::stl::string_view(str, N - 1), seed);
			}

			template<typename T, std::enable_if_t<std::is_pointer_v<T>, int> = 0>
			static inline HashT hash(const T v, HashT seed = constants::basis) {
				return hash(reinterpret_cast<size_t>(v), seed);
			}

			template<typename T>
			static inline HashT hash(const uf::stl::vector<T>& values, HashT seed = constants::basis) {
				if constexpr (std::is_same_v<T, bool>) {
					for (bool b : values) {
						seed = (seed ^ static_cast<HashT>(b)) * constants::prime;
					}
				} else if constexpr (std::is_trivially_copyable_v<T>) {
					const uint8_t* bytes = reinterpret_cast<const uint8_t*>(values.data());
					size_t len = values.size() * sizeof(T);
					for (size_t i = 0; i < len; ++i) {
						seed = (seed ^ static_cast<HashT>(bytes[i])) * constants::prime;
					}
				} else {
					for (const T& v : values) {
						seed = (seed ^ hash(v)) * constants::prime;
					}
				}
				return seed;
			}
		};

		template<typename HashT = size_t, typename T>
		inline constexpr HashT fnv1a(const T& val) {
			return fnv1a_impl<HashT>::hash(val);
		}

		template<typename HashT = size_t, typename T>
		inline constexpr HashT fnv1a(const T& val, HashT seed) {
			return fnv1a_impl<HashT>::hash(val, seed);
		}

		struct hasher {
			using is_transparent = void;
			template<typename T> inline size_t operator()(const T& val) const noexcept {
				return uf::algo::fnv1a<size_t>(val);
			}
		};
	}

	inline void hash(size_t& seed) { }

	template <typename T, typename... Rest>
	inline void hash(size_t& seed, const T& v, Rest... rest) {
		seed ^= uf::algo::fnv1a<size_t>(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		hash(seed, rest...);
	}
}