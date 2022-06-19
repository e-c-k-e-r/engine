#pragma once

namespace uf {
	inline void hash(std::size_t& seed) { }

	template <typename T, typename... Rest>
	inline void hash(std::size_t& seed, const T& v, Rest... rest) {
	    seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
	    hash(seed, rest...);
	}
}