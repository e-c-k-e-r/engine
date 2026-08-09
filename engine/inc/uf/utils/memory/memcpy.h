#pragma once
#include <cstring>

#if UF_ENV_DREAMCAST
	#include <dc/sq.h>
#endif

namespace uf {
	namespace stl {
		inline void* memcpy(void* dest, const void* src, size_t n) {
		#if UF_ENV_DREAMCAST
			if ( n >= 64 ) {
				if (((uintptr_t)(dest) & 31) == 0 && (n & 31) == 0 && ((uintptr_t)(src) & 3) == 0) {
					return ::sq_cpy(dest, src, n);
				}
			}
		#endif
			return std::memcpy(dest, src, n);
		}
	}
}