#pragma once

#include <uf/config.h>
#include <cstdint>
#include "hash.h"

#if UF_ENV_DREAMCAST
	#include "sh4.h"
	#include <dc/matrix.h>
	#define UF_EZ_VEC4(vec, size) vec[0], size > 1 ? vec[1] : 0, size > 2 ? vec[2] : 0, size > 3 ? vec[3] : 0
#endif

namespace pod {
	namespace Math {
		typedef float num_t;
	}
}

namespace {
// Sometimes uint isn't declared
	typedef unsigned int uint;
}
namespace uf {
	namespace math {
		uint16_t UF_API quantizeShort( float );
		float UF_API unquantize( uint16_t );

		template<typename T>
		inline T lerp( const T& a, const T& b, double f ) {
			return a + f * (b - a);
		}
	}
}