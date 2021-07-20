#pragma once

#include <uf/config.h>
#include "./allocator.h"

#include <map>

namespace uf {
	namespace stl {
		template<
			class Key,
			class T,
			class Compare = std::less<Key>,
		#if UF_MEMORYPOOL_USE_STL_ALLOCATOR
			class Allocator = std::allocator<std::pair<const Key, T>>
		#else
			class Allocator = uf::Allocator<std::pair<const Key, T>>
		#endif
		>
		using map = std::map<Key, T, Compare, Allocator>;
	}
}