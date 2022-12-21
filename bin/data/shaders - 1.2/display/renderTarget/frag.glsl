#version 450
#pragma shader_stage(fragment)
#extension GL_EXT_samplerless_texture_functions : require

layout (location = 0) in vec2 inUv;
layout (location = 1) flat in uint inPass;

layout (location = 0) out vec4 outColor;

layout (binding = 0) uniform sampler2D  samplerColor;

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

void main() {
	outColor = texture( samplerColor, inUv );
#if TONE_MAP
	toneMap(outColor, ubo.exposure);
#endif
#if GAMMA_CORRECT
	gammaCorrect(outColor, ubo.gamma);
#endif
}