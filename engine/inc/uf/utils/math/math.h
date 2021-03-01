#pragma once

#include <uf/config.h>
#include <cstdint>

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
		T lerp( const T& a, const T& b, double f ) {
			return a + f * (b - a);
		}
	}
}