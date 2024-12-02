#pragma once

#include <uf/config.h>
#include "./allocator.h"

#include <vector>

namespace uf {
	namespace stl {
		template<
			class T,
		#if UF_MEMORYPOOL_USE_STL_ALLOCATOR
			class Allocator = std::allocator<T>
		#else
			class Allocator = uf::Allocator<T>
		#endif
		>
		using vector = std::vector<T, Allocator>;

		template<typename T>
		T& random( uf::stl::vector<T>& v ) {
			return v[rand() % v.size()];		
		}

		template<typename T>
		T random_it( T begin, T end ) {
			uf::stl::vector<T> its;
			for ( auto it = begin; it != end; ++it ) its.emplace_back( it );
			return random( its );
		}
	}
}