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
layout (binding = 1, rgba16f) uniform image2D imageBright; // yucky
layout (binding = 2, rgba16f) coherent uniform image2D outImage[MIPS];

layout (binding = 3, std430) buffer AtomicCounter {
	uint counter;
} spdCounter;

layout (binding = 4) uniform UBO {
	float threshold;
	float smoothness;
	uint size;
	float padding1;

	float weights[32];
} ubo;

#define A_GLSL 1
#define A_GPU 1
#define SPD_NO_WAVE_OPERATIONS 0
#include "../../ext/ffx_a.h"

shared AU1 spd_counter;
shared AF4 spd_intermediate[16][16];

vec3 applySoftKnee(vec3 color, float luminance) {
	float rq = clamp(luminance - ubo.threshold + ubo.smoothness, 0.0, 2.0 * ubo.smoothness);
	rq = (rq * rq) / (4.0 * ubo.smoothness + 0.0001);

	float value = max(rq, luminance - ubo.threshold);

	return color * (value / (max(luminance, 0.0001)));
}

AF4 SpdLoadSourceImage(ASU2 p, AU1 slice) {
	ivec2 size = imageSize(imageColor);

	// sample color if in bound, else black
	vec3 c0 = p.x < size.x && p.y < size.y ? imageLoad(imageColor, p + ivec2(0, 0)).rgb : vec3(0.0);
	vec3 c1 = p.x + 1 < size.x && p.y < size.y ? imageLoad(imageColor, p + ivec2(1, 0)).rgb : vec3(0.0);
	vec3 c2 = p.x < size.x && p.y + 1 < size.y ? imageLoad(imageColor, p + ivec2(0, 1)).rgb : vec3(0.0);
	vec3 c3 = p.x + 1 < size.x && p.y + 1 < size.y ? imageLoad(imageColor, p + ivec2(1, 1)).rgb : vec3(0.0);

	// get luma
	float b0 = luma(c0);
	float b1 = luma(c1);
	float b2 = luma(c2);
	float b3 = luma(c3);

    // soften
	c0 = applySoftKnee(c0, b0);
	c1 = applySoftKnee(c1, b1);
	c2 = applySoftKnee(c2, b2);
	c3 = applySoftKnee(c3, b3);

	// karis luma weighted average
	float w0 = 1.0 / (b0 + 1.0);
	float w1 = 1.0 / (b1 + 1.0);
	float w2 = 1.0 / (b2 + 1.0);
	float w3 = 1.0 / (b3 + 1.0);
	float inv_wsum = 1.0 / (w0 + w1 + w2 + w3);

	// store to mip 0
	if (p.x < size.x && p.y < size.y) imageStore(outImage[0], p + ivec2(0, 0), vec4(c0, 1.0));
	if (p.x + 1 < size.x && p.y < size.y) imageStore(outImage[0], p + ivec2(1, 0), vec4(c1, 1.0));
	if (p.x < size.x && p.y + 1 < size.y) imageStore(outImage[0], p + ivec2(0, 1), vec4(c2, 1.0));
	if (p.x + 1 < size.x && p.y + 1 < size.y) imageStore(outImage[0], p + ivec2(1, 1), vec4(c3, 1.0));

    // average
	return AF4((c0 * w0 + c1 * w1 + c2 * w2 + c3 * w3) * inv_wsum, 1.0);
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

// average filter
AF4 SpdReduce4(AF4 v0, AF4 v1, AF4 v2, AF4 v3) {
	return (v0 + v1 + v2 + v3) * 0.25;
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