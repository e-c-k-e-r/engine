#include <uf/utils/math/math.h>

uint16_t uf::math::quantizeShort( float v ) {
	union { float f; uint32_t ui; } u = {v};
	uint32_t ui = u.ui;
	int s = (ui >> 16) & 0x8000;
	int em = ui & 0x7fffffff;
	int h = (em - (112 << 23) + (1 << 12)) >> 13;
	h = (em < (113 << 23)) ? 0 : h;
	h = (em >= (143 << 23)) ? 0x7c00 : h;
	h = (em > (255 << 23)) ? 0x7e00 : h;
	return (uint16_t)(s | h);
}
float uf::math::unquantize( uint16_t v ) {
	return v;
}