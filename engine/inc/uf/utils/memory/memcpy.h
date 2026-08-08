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
				if ((dest & 31) == 0 && (n & 31) == 0 && (src & 3) == 0) {
					return ::sq_cpy(dest, src, n);
				}
			}
		#endif
			return std::memcpy(dest, src, n);
		}
	}
}