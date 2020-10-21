#pragma once

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
		template<typename T>
		T lerp( const T& a, const T& b, double f ) {
			return a + f * (b - a);
		}
	}
}