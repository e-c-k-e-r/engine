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

layout (binding = 3) uniform sampler2D inImage[MIPS];
layout (binding = 4, r32f) uniform volatile coherent image2D outImage[MIPS];

void main() {
	vec2 imageSize =  imageSize(outImage[PushConstant.pass]);
	uvec2 pos = gl_GlobalInvocationID.xy;
	if ( pos.x >= imageSize.x || pos.y >= imageSize.y ) return;

	float depth = texture(inImage[PushConstant.pass], (vec2(pos) + vec2(0.5)) / imageSize).x;

	imageStore(outImage[PushConstant.pass], ivec2(pos), vec4(depth));
}