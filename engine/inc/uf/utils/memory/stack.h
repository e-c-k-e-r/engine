#pragma once

#include <uf/config.h>
#include "./allocator.h"
#include "./deque.h"

#include <stack>

namespace uf {
	namespace stl {
		template<
    		class T,
    		class Container = uf::stl::deque<T>
		>
		using stack = std::stack<T, Container>;
	}
}