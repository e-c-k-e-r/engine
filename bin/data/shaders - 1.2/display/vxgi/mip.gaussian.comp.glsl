#version 450
#pragma shader_stage(compute)
#extension GL_EXT_samplerless_texture_functions : require

layout (local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout (constant_id = 0) const uint CASCADES = 16;
layout (constant_id = 1) const uint MIPS = 16;

layout( push_constant ) uniform PushBlock {
  uint cascade;
  uint mip;
} PushConstant;

layout (binding = 1, rg16f) uniform volatile coherent image3D voxelRadiance[CASCADES * MIPS];

const float gaussianWeights[] = {
	//Top slice
	1 / 64.0f,
	1 / 32.0f,
	1 / 64.0f,
	1 / 32.0f,
	1 / 16.0f,
	1 / 32.0f,
	1 / 64.0f,
	1 / 32.0f,
	1 / 64.0f,

	//Center slice
	1 / 32.0f,
	1 / 16.0f,
	1 / 32.0f,
	1 / 16.0f,
	1 / 4.0f,
	1 / 16.0f,
	1 / 32.0f,
	1 / 16.0f,
	1 / 32.0f,

	//Bottom slice
	1 / 64.0f,
	1 / 32.0f,
	1 / 64.0f,
	1 / 32.0f,
	1 / 16.0f,
	1 / 32.0f,
	1 / 64.0f,
	1 / 32.0f,
	1 / 64.0f,
};

void main() {
	const ivec3 inUVW = ivec3(gl_GlobalInvocationID.xyz) * 2;
	const ivec3 outUVW = ivec3(gl_GlobalInvocationID.xyz);
	const uint CASCADE_IN = PushConstant.cascade * CASCADES + PushConstant.mip;
	const uint CASCADE_OUT = PushConstant.cascade * CASCADES + (PushConstant.mip + 1);
	
	vec4 color = vec4(0);
	for ( int z = -1; z <= 1; ++z ) {
	for ( int y = -1; y <= 1; ++y ) {
	for ( int x = -1; x <= 1; ++x ) {
		color += imageLoad( voxelRadiance[CASCADE_IN], inUVW + ivec3(x,y,z) ) * gaussianWeights[x + 1 + (y + 1) * 3 + (z + 1) * 9];
	}
	}
	}
	imageStore(voxelRadiance[CASCADE_OUT], ivec3(outUVW), vec4(color));
}