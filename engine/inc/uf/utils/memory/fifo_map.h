#pragma once

#include <uf/config.h>
#include "./allocator.h"

#include <nlohmann/fifo_map.hpp>

namespace uf {
	namespace stl {
		template<
			class Key,
			class T,
			class Compare = nlohmann::fifo_map_compare<Key>,
		#if UF_MEMORYPOOL_USE_STL_ALLOCATOR
			class Allocator = std::allocator<std::pair<const Key, T>>
		#else
			class Allocator = uf::Allocator<std::pair<const Key, T>>
		#endif
		>
		using fifo_map = nlohmann::fifo_map<Key, T, Compare, Allocator>;
	}
}