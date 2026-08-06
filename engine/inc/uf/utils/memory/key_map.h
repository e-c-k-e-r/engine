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
			const T& operator[]( const Key& key ) const;
			
			void reserve( size_t i );
			size_t id( const Key& );
			uf::stl::vector<T> flatten() const;
			uf::stl::vector<T> flattenByIndex() const;
			void clear();
			void merge( KeyMap<T, Key>&& other );
			void import( const KeyMap<T, Key>& other );
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
const T& uf::stl::KeyMap<T,Key>::operator[]( const Key& key ) const {
	UF_ASSERT( map.count( key ) > 0 );
	return map.at(key);
}

template<typename T, typename Key>
void uf::stl::KeyMap<T,Key>::reserve( size_t i ) {
	keys.reserve(i);
	indices.reserve(i);
	map.reserve(i);
}

template<typename T, typename Key>
size_t uf::stl::KeyMap<T,Key>::id( const Key& key ) {
	if ( indices.count( key ) > 0 ) return indices[key];
	
	size_t newIndex = keys.size();
	
	indices[key] = newIndex;
	keys.emplace_back( key );
	map[key];

	return newIndex;
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

template<typename T, typename Key>
uf::stl::vector<T> uf::stl::KeyMap<T,Key>::flattenByIndex() const {
	vector<T> res(indices.size());

	for (auto& [key, index] : indices) {
		res[index] = map.at(key);
	}

	return res;
}

template<typename T, typename Key>
void uf::stl::KeyMap<T,Key>::merge( KeyMap<T, Key>&& other ) {
	this->reserve( this->keys.size() + other.keys.size() );

	for ( auto& key : other.keys ) {
		T&& value = std::move(other.map.at(key));

		if ( this->map.count(key) == 0 ) {
			this->indices[key] = this->keys.size();
			this->keys.emplace_back(key);
		}

		this->map[key] = std::move(value);
	}

	other.clear();
}

template<typename T, typename Key>
void uf::stl::KeyMap<T,Key>::import( const KeyMap<T, Key>& other ) {
	this->reserve( this->keys.size() + other.keys.size() );

	for ( auto& key : other.keys ) {
		(*this)[key] = other.map.at(key);
	}
}