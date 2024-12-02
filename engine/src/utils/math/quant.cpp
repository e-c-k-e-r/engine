#include <uf/utils/math/quant.h>

// credit: https://github.com/zeux/meshoptimizer/blob/master/src/quantization.cpp
uint16_t UF_API uf::quant::quantize_f32u16( float v ) {
	union { float f; uint32_t ui; } u = {v};
    uint32_t ui = u.ui;

    int s = (ui >> 16) & 0x8000;
    int em = ui & 0x7fffffff;

    int h = (em - (112 << 23) + (1 << 12)) >> 13;
    h = (em < (113 << 23)) ? 0 : h;
    h = (em >= (143 << 23)) ? 0x7c00 : h;
    h = (em > (255 << 23)) ? 0x7e00 : h;

    uint16_t res = (s | h);
    return res;
}
float UF_API uf::quant::dequantize_u16f32( uint16_t h ) {
	uint32_t s = unsigned(h & 0x8000) << 16;
	int em = h & 0x7fff;
	int r = (em + (112 << 10)) << 13;
	r = (em < (1 << 10)) ? 0 : r;
	r += (em >= (31 << 10)) ? (112 << 23) : 0;

	union { float f; uint32_t ui; } u;
	u.ui = s | r;

	return u.f;
}