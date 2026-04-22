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

layout (binding = 0) uniform sampler2D samplerDepth;
layout (binding = 1, r32f) coherent uniform image2D outImage[MIPS];

layout (binding = 2, std430) buffer AtomicCounter {
	uint counter;
} spdCounter;


#define A_GLSL 1
#define A_GPU 1
#define SPD_NO_WAVE_OPERATIONS 0
#include "../../ext/ffx_a.h"

shared AU1 spd_counter;
shared AF1 spd_intermediate[16][16];

AF4 SpdLoadSourceImage(ASU2 p, AU1 slice) {
	ivec2 size = imageSize(outImage[0]);

	// sample depth if in bound, else 0 (0 for reverse-z projection, use 1 if normal projection)
	float d0 = p.x < size.x && p.y < size.y ? texelFetch(samplerDepth, p + ivec2(0, 0), 0).x : 0.0;
	float d1 = p.x + 1 < size.x && p.y < size.y ? texelFetch(samplerDepth, p + ivec2(1, 0), 0).x : 0.0;
	float d2 = p.x < size.x && p.y + 1 < size.y ? texelFetch(samplerDepth, p + ivec2(0, 1), 0).x : 0.0;
	float d3 = p.x + 1 < size.x && p.y + 1 < size.y ? texelFetch(samplerDepth, p + ivec2(1, 1), 0).x : 0.0;

	// store to mip 0
	if (p.x < size.x && p.y < size.y) imageStore(outImage[0], p + ivec2(0, 0), vec4(d0));
	if (p.x + 1 < size.x && p.y < size.y) imageStore(outImage[0], p + ivec2(1, 0), vec4(d1));
	if (p.x < size.x && p.y + 1 < size.y) imageStore(outImage[0], p + ivec2(0, 1), vec4(d2));
	if (p.x + 1 < size.x && p.y + 1 < size.y) imageStore(outImage[0], p + ivec2(1, 1), vec4(d3));

	return AF4(d0, d1, d2, d3);
}

AF4 SpdLoad(ASU2 p, AU1 slice) {
	uint loadMip = min(6u, MIPS - 1);
	float d = imageLoad(outImage[loadMip], p).r;
	return AF4(d, d, d, d);
}

void SpdStore(ASU2 p, AF4 value, AU1 mip, AU1 slice) {
	if ( mip + 1 < MIPS ) {
		imageStore(outImage[mip + 1], p, vec4(value.x));
	}
}

AF4 SpdLoadIntermediate(AU1 x, AU1 y) {
		float d = spd_intermediate[x][y];
		return AF4(d, d, d, d);
}
void SpdStoreIntermediate(AU1 x, AU1 y, AF4 value) { spd_intermediate[x][y] = value.x; }
void SpdIncreaseAtomicCounter(AU1 slice) { spd_counter = atomicAdd(spdCounter.counter, 1); }
AU1 SpdGetAtomicCounter() { return spd_counter; }
void SpdResetAtomicCounter(AU1 slice) { spdCounter.counter = 0; }

// min filter
AF4 SpdReduce4(AF4 v0, AF4 v1, AF4 v2, AF4 v3) {
	float minVal = min(min(v0.x, v1.x), min(v2.x, v3.x));
	return AF4(minVal, minVal, minVal, minVal);
}

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