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

layout (binding = 2) readonly buffer VertexInputPosition {
	/*vec3 verticesInPos[];*/
	float verticesInPos[];
};
layout (binding = 3) readonly buffer VertexInputJoints {
	uvec2 verticesInJoints[];
};
layout (binding = 4) readonly buffer VertexInputWeights {
	vec4 verticesInWeights[];
};

layout (binding = 5) buffer VertexOutputPosition {
	/*vec3 verticesOutPos[];*/
	float verticesOutPos[];
};

void main() {
	const uint i = gl_GlobalInvocationID.x;
	
	if ( i * 3 >= verticesInPos.length() || i * 3 >= verticesOutPos.length() ) return;

	const vec3 inPos = vec3( verticesInPos[i * 3 + 0], verticesInPos[i * 3 + 1], verticesInPos[i * 3 + 2] );
	const uvec4 inJoints = uvec2_16x4(verticesInJoints[i]);
	const vec4 inWeights = verticesInWeights[i];

	const mat4 skinned = inWeights.x * joints[ubo.jointID + int(inJoints.x)] + inWeights.y * joints[ubo.jointID + int(inJoints.y)] + inWeights.z * joints[ubo.jointID + int(inJoints.z)] + inWeights.w * joints[ubo.jointID + int(inJoints.w)];

	const vec3 outPos = vec3(skinned * vec4(inPos, 1));
	verticesOutPos[i * 3 + 0] = outPos[0];
	verticesOutPos[i * 3 + 1] = outPos[1];
	verticesOutPos[i * 3 + 2] = outPos[2];
}