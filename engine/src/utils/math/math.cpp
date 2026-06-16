#include <uf/utils/math/math.h>
#include <uf/utils/math/vector.h>

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

uf::stl::vector<pod::Vector3f> uf::math::fibonacciSphere( int num ) {
	uf::stl::vector<pod::Vector3f> points;
	points.reserve( num );

	const float goldenAngle = M_PI * ( 3.0f - std::sqrt( 5.0f ) );
	for ( auto i = 0; i < num; ++i ) {
		float y = 1.0f - ( i / float( num - 1 ) ) * 2.0f;
		float radius = std::sqrt( 1.0f - y * y );
		float theta = goldenAngle * i;

		points.emplace_back( pod::Vector3f{
			.x = std::cos( theta ) * radius,
			.y = y,
			.z = std::sin( theta ) * radius,
		});
	}

	return points;
}