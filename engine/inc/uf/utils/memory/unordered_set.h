#pragma once

#include <uf/config.h>
#include "./allocator.h"

#include <unordered_set>
#include "vector.h"

namespace uf {
	namespace stl {
		template<
			class Key,
			class Hash = std::hash<Key>,
			class KeyEqual = std::equal_to<Key>,
		#if UF_MEMORYPOOL_USE_ALLOCATOR
			class Allocator = std::allocator<Key>
		#else
			class Allocator = uf::Allocator<Key>
		#endif
		>
		using unordered_set = std::unordered_set<Key, Hash, KeyEqual, Allocator>;
	}
}