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

layout (binding = 0) uniform UBO {
	uint jointID;
	uint padding1;
	uint padding2;
	uint padding3;
} ubo;
layout (std140, binding = 1) readonly buffer Joints {
	mat4 joints[];
};
layout (binding = 2) readonly buffer VertexInput {
	Vertex verticesIn[];
};
layout (binding = 3) buffer VertexOutput {
	Vertex verticesOut[];
};

void main() {
	const uint i = gl_GlobalInvocationID.x;
	
	if ( i >= verticesIn.length() || i >= verticesOut.length() ) return;

	const vec3 inPos = verticesIn[i].position;
	const uvec4 inJoints = uvec2_16x4(verticesIn[i].joints);
	const vec4 inWeights = verticesIn[i].weights;

	const mat4 skinned = inWeights.x * joints[ubo.jointID + int(inJoints.x)] + inWeights.y * joints[ubo.jointID + int(inJoints.y)] + inWeights.z * joints[ubo.jointID + int(inJoints.z)] + inWeights.w * joints[ubo.jointID + int(inJoints.w)];

	verticesOut[i].position = vec3(skinned * vec4(verticesIn[i].position,1));
}