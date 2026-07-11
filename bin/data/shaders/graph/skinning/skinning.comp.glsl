#version 450
#pragma shader_stage(compute)

layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

#define COMPUTE 1

#include "../../common/macros.h"
#include "../../common/structs.h"

uvec4 uvec2_16x4( uvec2 i ) {
	uvec4 converted;

	converted.x = (i.x >>  0) & 0xFFFF;
	converted.y = (i.x >> 16) & 0xFFFF;
	converted.z = (i.y >>  0) & 0xFFFF;
	converted.w = (i.y >> 16) & 0xFFFF;

	return converted;
}

layout (push_constant) uniform SkinningPush {
	uint jointID;
	uint triangleCount;
} push;

layout (std140, binding = 0) readonly buffer Joints {
	mat4 joints[];
};

layout (binding = 1) readonly buffer VertexInputPosition {
	float verticesInPos[];
};
layout (binding = 2) readonly buffer VertexInputJoints {
	uvec2 verticesInJoints[];
};
layout (binding = 3) readonly buffer VertexInputWeights {
	vec4 verticesInWeights[];
};

layout (binding = 4) buffer VertexOutputPosition {
	float verticesOutPos[];
};

void main() {
	const uint i = gl_GlobalInvocationID.x;

	if ( i >= push.triangleCount || i >= push.triangleCount ) return;

	const vec3 inPos = vec3( verticesInPos[i * 3 + 0], verticesInPos[i * 3 + 1], verticesInPos[i * 3 + 2] );
	const uvec4 inJoints = uvec2_16x4(verticesInJoints[i]);
	const vec4 inWeights = verticesInWeights[i];

	vec4 inPos4 = vec4(inPos, 1.0);
    vec3 outPos = (joints[push.jointID + int(inJoints.x)] * inPos4).xyz * inWeights.x
                + (joints[push.jointID + int(inJoints.y)] * inPos4).xyz * inWeights.y
                + (joints[push.jointID + int(inJoints.z)] * inPos4).xyz * inWeights.z
                + (joints[push.jointID + int(inJoints.w)] * inPos4).xyz * inWeights.w;

	verticesOutPos[i * 3 + 0] = outPos[0];
	verticesOutPos[i * 3 + 1] = outPos[1];
	verticesOutPos[i * 3 + 2] = outPos[2];
}