#version 450
#pragma shader_stage(compute)

//#extension GL_EXT_nonuniform_qualifier : enable

layout (constant_id = 0) const uint MIPS = 6;
layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

#define COMPUTE 1

#include "../../common/macros.h"
#include "../../common/structs.h"

layout( push_constant ) uniform PushBlock {
  uint _;
  uint pass;
} PushConstant;

layout (binding = 0) uniform sampler2D samplerDepth;
layout (binding = 1) uniform sampler2D inImage[MIPS];
layout (binding = 2, r32f) uniform writeonly image2D outImage[MIPS];

void main() {
	int mip = int(PushConstant.pass);

	float depth;
	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
	if ( mip == 0 ) {
		depth = texelFetch(samplerDepth, pos, 0).r;
	} else {
		depth = texture(inImage[mip - 1], (vec2(gl_GlobalInvocationID.xy) + vec2(0.5)) / imageSize( outImage[mip] )).x;
	}

	imageStore(outImage[mip], pos, vec4(depth));
}