#pragma once

#include <uf/config.h>
#include "./allocator.h"
#include "./deque.h"

#include <queue>

namespace uf {
	namespace stl {
		template<
    		class T,
    		class Container = uf::stl::deque<T>
		>
		using queue = std::queue<T, Container>;
	}
}