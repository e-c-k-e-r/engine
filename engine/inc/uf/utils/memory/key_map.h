#pragma once

#include <uf/config.h>
#include "./allocator.h"

#include <map>

namespace uf {
	namespace stl {
		template<typename T, typename Key = uf::stl::string>
		struct KeyMap {
		public:
			uf::stl::vector<Key> keys;
			uf::stl::unordered_map<Key, T> map;
			uf::stl::unordered_map<Key, size_t> indices;

			T& operator[]( const Key& key );
			
			void reserve( size_t i );
			uf::stl::vector<T> flatten() const;
			void clear();
		};
	}
}


template<typename T, typename Key>
T& uf::stl::KeyMap<T,Key>::operator[]( const Key& key ) {
	if ( map.count( key ) == 0 ) {
		indices[key] = keys.size();
		keys.emplace_back( key );
	}
	return map[key];
}

template<typename T, typename Key>
void uf::stl::KeyMap<T,Key>::reserve( size_t i ) {
	keys.reserve(i);
	indices.reserve(i);
	map.reserve(i);
}

template<typename T, typename Key>
void uf::stl::KeyMap<T,Key>::clear() {
	keys.clear();
	indices.clear();
	map.clear();
}

template<typename T, typename Key>
uf::stl::vector<T> uf::stl::KeyMap<T,Key>::flatten() const {
	uf::stl::vector<T> res; res.reserve(keys.size());
	for ( auto& key : keys ) {
		if ( map.count( key ) == 0 ) {
		//	UF_EXCEPTION("key not in map: {}", key);
			res.emplace_back();
		} else {
			res.emplace_back(map.at(key));
		}
	}
	return res;
}