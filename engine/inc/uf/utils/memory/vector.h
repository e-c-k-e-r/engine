#pragma once

#include <uf/config.h>
#include "./allocator.h"

#include <vector>

namespace uf {
	namespace stl {
		template<
			class T,
		#if UF_MEMORYPOOL_OVERRIDE_NEW_DELETE
			class Allocator = std::allocator<T>
		#else
			class Allocator = uf::Allocator<T>
		#endif
		>
		using vector = std::vector<T, Allocator>;
	}
}