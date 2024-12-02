#pragma once

#include <uf/config.h>
#include "./allocator.h"

#include <unordered_map>
#include "vector.h"

namespace uf {
	namespace stl {
		template<
			class Key,
			class T,
			class Hash = std::hash<Key>,
			class KeyEqual = std::equal_to<Key>,
		#if UF_MEMORYPOOL_USE_STL_ALLOCATOR
			class Allocator = std::allocator<std::pair<const Key, T>>
		#else
			class Allocator = uf::Allocator<std::pair<const Key, T>>
		#endif
		>
		using unordered_map = std::unordered_map<Key, T, Hash, KeyEqual, Allocator>;

		template<typename Key, typename T>
		uf::stl::vector<Key> keys( const uf::stl::unordered_map<Key, T>& map ) {
			uf::stl::vector<Key> keys;
			for ( auto pair : map ) keys.emplace_back( pair.first );
			return keys;
		}
	}
}