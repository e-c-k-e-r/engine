#pragma once

#include <uf/config.h>
#include "./allocator.h"

#include <deque>

namespace uf {
	namespace stl {
		template<
			class T,
		#if UF_MEMORYPOOL_USE_ALLOCATOR
			class Allocator = uf::Allocator<T>
		#else
			class Allocator = std::allocator<T>
		#endif
		>
		using deque = std::deque<T, Allocator>;
	}
}