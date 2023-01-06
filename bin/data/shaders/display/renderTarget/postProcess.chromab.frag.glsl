#version 450
#pragma shader_stage(fragment)
#extension GL_EXT_samplerless_texture_functions : require

layout (location = 0) in vec2 inUv;
layout (location = 1) flat in uint inPass;

layout (location = 0) out vec4 outColor;

layout (binding = 0) uniform sampler2D	samplerColor;

layout (binding = 1) uniform UBO {
	float curTime;
	float gamma;
	float exposure;
	uint padding;
} ubo;

#define TONE_MAP 1
#define GAMMA_CORRECT 1
#define TEXTURES 1

#include "../../common/macros.h"
#include "../../common/structs.h"
#include "../../common/functions.h"

vec2 barrelDistortion(vec2 coord, float amt) {
	vec2 cc = coord - 0.5;
	float dist = dot(cc, cc);
	return coord + cc * dist * amt;
}

float sat( float t ) {
	return clamp( t, 0.0, 1.0 );
}

float linterp( float t ) {
	return sat( 1.0 - abs( 2.0*t - 1.0 ) );
}

float remap( float t, float a, float b ) {
	return sat( (t - a) / (b - a) );
}

vec4 spectrumOffset( float t ) {
	vec4 ret;
	float lo = step(t,0.5);
	float hi = 1.0-lo;
	float w = linterp( remap( t, 1.0/6.0, 5.0/6.0 ) );
	ret = vec4(lo,1.0,hi, 1.) * vec4(1.0-w, w, 1.0-w, 1.);

	return pow( ret, vec4(1.0/2.2) );
}

const float MAX_DISTORT = 0.125;
const int ITERATIONS = 32;
const float ITERATIONS_RECIP = 1.0 / float(ITERATIONS);

void main() {
	const vec2 screenResolution = textureSize( samplerColor, 0 );
	const vec2 uv = (inUv.xy); // * 2.0 - 1.0);

	vec4 colorSigma = vec4(0.0);
	vec4 wSigma = vec4(0.0);	
	for ( uint i=0; i < ITERATIONS;++i ) {
		float t = float(i) * ITERATIONS_RECIP;
		vec4 w = spectrumOffset( t );
		wSigma += w;
		colorSigma += w * texture(samplerColor, barrelDistortion(uv, MAX_DISTORT * t * 0.6 ));
	}

	outColor = colorSigma / wSigma;

#if TONE_MAP
	toneMap(outColor, ubo.exposure);
#endif
#if GAMMA_CORRECT
	gammaCorrect(outColor, ubo.gamma);
#endif
}