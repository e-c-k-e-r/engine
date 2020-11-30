#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUv;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec4 inTangent;
layout (location = 4) in uint inId;
layout (location = 5) in vec4 inJoints;
layout (location = 6) in vec4 inWeights;

layout( push_constant ) uniform PushBlock {
  uint pass;
} PushConstant;

struct Matrices {
	mat4 model;
	mat4 view[2];
	mat4 projection[2];
};
layout (binding = 0) uniform UBO {
	Matrices matrices;
	vec4 color;
} ubo;

layout (std140, binding = 1) buffer Joints {
	mat4 joints[];
};


layout (location = 0) noperspective out vec2 outUv;
layout (location = 1) out vec4 outColor;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out mat3 outTBN;
layout (location = 6) out vec3 outPosition;
layout (location = 7) flat out uint outId;
layout (location = 8) out float affine;

out gl_PerVertex {
    vec4 gl_Position;   
};

vec4 snap(vec4 vertex, vec2 resolution) {
    vec4 snappedPos = vertex;
    snappedPos.xyz = vertex.xyz / vertex.w;
    snappedPos.xy = floor(resolution * snappedPos.xy) / resolution;
    snappedPos.xyz *= vertex.w;
    return snappedPos;
}

void main() {
	outUv = inUv;
	outColor = ubo.color;

	mat4 skinnedMatrix = joints.length() <= 0 ? mat4(1.0) : 
		inWeights.x * joints[int(inJoints.x)] +
		inWeights.y * joints[int(inJoints.y)] +
		inWeights.z * joints[int(inJoints.z)] +
		inWeights.w * joints[int(inJoints.w)];

	outPosition = vec3(ubo.matrices.view[PushConstant.pass] * ubo.matrices.model * skinnedMatrix * vec4(inPos.xyz, 1.0));
	outNormal = vec3(ubo.matrices.view[PushConstant.pass] * ubo.matrices.model * vec4(inNormal.xyz, 0.0));
	outNormal = normalize(outNormal);

	outId = inId;

	{
		vec3 T = vec3(ubo.matrices.view[PushConstant.pass] * ubo.matrices.model * vec4(inTangent.xyz, 0.0));
		vec3 N = outNormal;
		vec3 B = cross(N, T) * inTangent.w;
		outTBN = mat3( T, B, N );
	}

	gl_Position = ubo.matrices.projection[PushConstant.pass] * ubo.matrices.view[PushConstant.pass] * ubo.matrices.model * skinnedMatrix * vec4(inPos.xyz, 1.0);
//	gl_Position = snap( gl_Position, vec2(320.0, 240.0) );
//	gl_Position = snap( gl_Position, vec2(480.0, 270.0) );
//	gl_Position = snap( gl_Position, vec2(640.0, 480.0) );

	affine = 1;
}