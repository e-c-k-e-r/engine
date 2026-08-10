#pragma once

#include <uf/config.h>
#include "./allocator.h"

#include <unordered_map>
#include "vector.h"
#include <uf/utils/math/hash.h>

namespace uf {
	namespace stl {
		template<
			class Key,
			class T,
			class Hash = uf::algo::hasher,
			class KeyEqual = std::equal_to<Key>,
		#if UF_MEMORYPOOL_USE_ALLOCATOR
			class Allocator = uf::Allocator<std::pair<const Key, T>>
		#else
			class Allocator = std::allocator<std::pair<const Key, T>>
		#endif
		>
		using unordered_map = std::unordered_map<Key, T, Hash, KeyEqual, Allocator>;

		template<typename Key, typename T>
		uf::stl::vector<Key> keys( const uf::stl::unordered_map<Key, T>& map ) {
			uf::stl::vector<Key> keys; keys.reserve( map.size() );
			for ( auto pair : map ) keys.emplace_back( pair.first );
			return keys;
		}

		template<typename Key, typename T>
		uf::stl::vector<T> values( const uf::stl::unordered_map<Key, T>& map ) {
			uf::stl::vector<T> values; values.reserve( map.size() );
			for ( auto pair : map ) values.emplace_back( pair.second );
			return values;
		}
	}
}