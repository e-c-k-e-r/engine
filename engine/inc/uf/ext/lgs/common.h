#pragma once
#if UF_USE_LGS
#include <uf/config.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/engine/graph/graph.h>

namespace impl {	
	const float darkToMeters = 0.7f;
	typedef uf::Meshlet_T<uf::graph::mesh::Skinned, uint32_t> Meshlet;

	inline float findWrap( float x ) {
		return -64.0f * std::floor(x / 64.0f);
	};

	inline void encodeRGBE(const pod::Vector3f& color, uint8_t* out) {
		float maxColor = std::max({ color.x, color.y, color.z });

		if ( maxColor < 1e-6f ) {
			out[0] = 0;
			out[1] = 0;
			out[2] = 0;
			out[3] = 0;
			return;
		}

		int exponent;
		float mantissa = std::frexp(maxColor, &exponent);

		float scale = std::exp2(-(float)(exponent));
		pod::Vector3f rgb = color * scale;

		out[0] = (uint8_t)(std::clamp(rgb.x * 255.f, 0.f, 255.f));
		out[1] = (uint8_t)(std::clamp(rgb.y * 255.f, 0.f, 255.f));
		out[2] = (uint8_t)(std::clamp(rgb.z * 255.f, 0.f, 255.f));
		out[3] = (uint8_t)(std::clamp(exponent + 128, 0, 255));
	}

	inline pod::Vector3f hsvToRgb(float h, float s, float v) {
		if (s <= 0.0f) return {v, v, v};

		h = std::fmod(h, 1.0f) * 6.0f;
		int i = (int)std::floor(h);
		float f = h - (float)i;

		float p = v * (1.0f - s);
		float q = v * (1.0f - s * f);
		float t = v * (1.0f - s * (1.0f - f));

		switch (i) {
			case 0: return {v, t, p};
			case 1: return {q, v, p};
			case 2: return {p, v, t};
			case 3: return {p, q, v};
			case 4: return {t, p, v};
			default: return {v, p, q};
		}
	}

	inline pod::Vector3f convertPos_NewDark( const pod::Vector3f& v, float scale = impl::darkToMeters ) {
		return pod::Vector3f{ v.x, v.z, v.y } * scale;
	}
}
#endif