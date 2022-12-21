#pragma once

#include <uf/config.h>

namespace pod {
	template<typename T>
	struct UF_API Resolvable {
		size_t uid = 0;
		T* pointer = NULL;

		pod::Resolvable<T>& operator=( const T& t ) {
			this->pointer = &t;
			return *this;
		}
		operator bool() const {
			return uid || pointer;
		}
	};
}