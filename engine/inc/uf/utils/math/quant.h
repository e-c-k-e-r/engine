#pragma once

#include <uf/config.h>
#include <uf/utils/math/vector.h>

namespace uf {
	namespace quant {

		uint16_t UF_API quantize_f32u16( float );
		float UF_API dequantize_u16f32( uint16_t );

		inline uint16_t quantize( float f ) { return quantize_f32u16( f ); }
		inline float dequantize( uint16_t h ) { return dequantize_u16f32( h ); }
	}
}