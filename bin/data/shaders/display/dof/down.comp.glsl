#version 450
#pragma shader_stage(compute)

#extension GL_KHR_shader_subgroup_quad : require
#extension GL_KHR_shader_subgroup_arithmetic : require
#extension GL_EXT_samplerless_texture_functions : enable

#define COMPUTE 1
#define SPD 1

#include "../../common/macros.h"
#include "../../common/structs.h"
#include "../../common/functions.h"

layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

layout (constant_id = 0) const uint MIPS = 6;

layout(push_constant) uniform PushBlock {
	uint mips;
	uint numWorkGroups;
	uint workGroupOffset;
} PushConstant_;

layout (binding = 0, rgba16f) uniform image2D imageColor;
layout (binding = 1, r32f)	uniform image2D imageDepth;
layout (binding = 2, rgba16f) uniform image2D imageBright; // yucky, needed for making things happy
layout (binding = 3, rgba16f) coherent uniform image2D outImage[MIPS];

layout (binding = 4, std430) buffer AtomicCounter {
	uint counter;
} spdCounter;

layout (binding = 5) uniform UBO {
	float focusDistance;
	float focusRange;
	float maxCoC;
	float nearPlane;
} ubo;

#define A_GLSL 1
#define A_GPU 1
#define SPD_NO_WAVE_OPERATIONS 0
#include "../../ext/ffx_a.h"

shared AU1 spd_counter;
shared AF4 spd_intermediate[16][16];

float linearizeDepth( float d ) {
	return ubo.nearPlane / max(d, 0.000001);
}

// 0.0 = perfectly in focus, 1.0 = maximum blur
float calculateCoC(float depth) {
	float dist = abs(linearizeDepth(depth) - ubo.focusDistance);
	float coc = clamp(dist / ubo.focusRange, 0.0, 1.0);
	return coc * ubo.maxCoC;
}

AF4 SpdReduce4(AF4 v0, AF4 v1, AF4 v2, AF4 v3);

AF4 SpdLoadSourceImage(ASU2 p, AU1 slice) {
	ivec2 size = imageSize(imageColor);

	// sample color and depth
	vec3 c0 = p.x < size.x && p.y < size.y ? imageLoad(imageColor, p + ivec2(0, 0)).rgb : vec3(0.0);
	vec3 c1 = p.x + 1 < size.x && p.y < size.y ? imageLoad(imageColor, p + ivec2(1, 0)).rgb : vec3(0.0);
	vec3 c2 = p.x < size.x && p.y + 1 < size.y ? imageLoad(imageColor, p + ivec2(0, 1)).rgb : vec3(0.0);
	vec3 c3 = p.x + 1 < size.x && p.y + 1 < size.y ? imageLoad(imageColor, p + ivec2(1, 1)).rgb : vec3(0.0);

	float d0 = p.x < size.x && p.y < size.y ? imageLoad(imageDepth, p + ivec2(0, 0)).r : 0.0;
	float d1 = p.x + 1 < size.x && p.y < size.y ? imageLoad(imageDepth, p + ivec2(1, 0)).r : 0.0;
	float d2 = p.x < size.x && p.y + 1 < size.y ? imageLoad(imageDepth, p + ivec2(0, 1)).r : 0.0;
	float d3 = p.x + 1 < size.x && p.y + 1 < size.y ? imageLoad(imageDepth, p + ivec2(1, 1)).r : 0.0;

	// calculate CoC
	vec4 p0 = vec4(c0, calculateCoC(d0));
	vec4 p1 = vec4(c1, calculateCoC(d1));
	vec4 p2 = vec4(c2, calculateCoC(d2));
	vec4 p3 = vec4(c3, calculateCoC(d3));

	// store mip 0
	if (p.x < size.x && p.y < size.y)		 imageStore(outImage[0], p + ivec2(0, 0), p0);
	if (p.x + 1 < size.x && p.y < size.y)	 imageStore(outImage[0], p + ivec2(1, 0), p1);
	if (p.x < size.x && p.y + 1 < size.y)	 imageStore(outImage[0], p + ivec2(0, 1), p2);
	if (p.x + 1 < size.x && p.y + 1 < size.y) imageStore(outImage[0], p + ivec2(1, 1), p3);

	return SpdReduce4(p0, p1, p2, p3);
}

AF4 SpdLoad(ASU2 p, AU1 slice) {
	uint loadMip = min(6u - 1, MIPS - 1);
	return imageLoad(outImage[loadMip + 1], p);
}

void SpdStore(ASU2 p, AF4 value, AU1 mip, AU1 slice) {
	if ( mip + 1 < MIPS ) {
		imageStore(outImage[mip + 1], p, value);
	}
}

AF4 SpdReduce4(AF4 v0, AF4 v1, AF4 v2, AF4 v3) {
	float maxCoC = max(max(v0.a, v1.a), max(v2.a, v3.a));

	float w0 = v0.a + 0.0001;
	float w1 = v1.a + 0.0001;
	float w2 = v2.a + 0.0001;
	float w3 = v3.a + 0.0001;
	float wSum = w0 + w1 + w2 + w3;

	vec3 weightedColor = (v0.rgb * w0 + v1.rgb * w1 + v2.rgb * w2 + v3.rgb * w3) / wSum;

	return vec4(weightedColor, maxCoC);
}

AF4 SpdLoadIntermediate(AU1 x, AU1 y) { return spd_intermediate[x][y]; }
void SpdStoreIntermediate(AU1 x, AU1 y, AF4 value) { spd_intermediate[x][y] = value; }

void SpdIncreaseAtomicCounter(AU1 slice) { spd_counter = atomicAdd(spdCounter.counter, 1); }
AU1 SpdGetAtomicCounter() { return spd_counter; }
void SpdResetAtomicCounter(AU1 slice) { spdCounter.counter = 0; }

#include "../../ext/ffx_spd.h"

void main() {
	SpdDownsample(
		AU2(gl_WorkGroupID.xy),
		AU1(gl_LocalInvocationIndex),
		AU1(PushConstant_.mips - 1),
		AU1(PushConstant_.numWorkGroups),
		AU1(PushConstant_.workGroupOffset)
	);
}