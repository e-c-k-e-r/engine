#pragma once

#include <uf/config.h>
#include <cstdint>
#include "hash.h"

#if UF_ENV_DREAMCAST
	#include "sh4.h"
	#include <dc/matrix.h>
	#define UF_EZ_VEC4(vec, size) vec[0], size > 1 ? vec[1] : 0, size > 2 ? vec[2] : 0, size > 3 ? vec[3] : 0

	#undef M_PI
#endif


#define NUM pod::Math::num_t
#define M_PI 3.141592653589793f
#define EPS 1.0e-6f
#define EPS2 (EPS * EPS)

#define RAD_2_DEG (180.0f / M_PI)
#define DEG_2_RAD (M_PI / 180.0f)

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