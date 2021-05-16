#version 450
#pragma shader_stage(vertex)

layout (constant_id = 0) const uint PASSES = 6;
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUv;
layout (location = 2) in vec2 inSt;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in vec4 inTangent;
layout (location = 5) in ivec2 inId;
layout (location = 6) in ivec4 inJoints;
layout (location = 7) in vec4 inWeights;

layout( push_constant ) uniform PushBlock {
  uint pass;
  uint draw;
} PushConstant;

layout (binding = 3) uniform UBO {
	mat4 view[PASSES];
	mat4 projection[PASSES];
} ubo;

layout (std140, binding = 4) readonly buffer Models {
	mat4 models[];
};

layout (std140, binding = 5) readonly buffer Joints {
	mat4 joints[];
};

layout (location = 0) out vec2 outUv;
layout (location = 1) out vec2 outSt;
layout (location = 2) out vec4 outColor;
layout (location = 3) out vec3 outNormal;
layout (location = 4) out mat3 outTBN;
layout (location = 7) out vec3 outPosition;
layout (location = 8) out ivec4 outId;

vec4 snap(vec4 vertex, vec2 resolution) {
    vec4 snappedPos = vertex;
    snappedPos.xyz = vertex.xyz / vertex.w;
    snappedPos.xy = floor(resolution * snappedPos.xy) / resolution;
    snappedPos.xyz *= vertex.w;
    return snappedPos;
}

void main() {
	outUv = inUv;
	outSt = inSt;
	outColor = vec4(1.0);
	outId = ivec4(inId, PushConstant.pass, PushConstant.draw);

	mat4 model = models.length() <= 0 ? mat4(1.0) : models[int(inId.x)];
	mat4 skinnedMatrix = joints.length() <= 0 ? mat4(1.0) : 
		inWeights.x * joints[int(inJoints.x)] +
		inWeights.y * joints[int(inJoints.y)] +
		inWeights.z * joints[int(inJoints.z)] +
		inWeights.w * joints[int(inJoints.w)];

//	outPosition = vec3(ubo.projection[PushConstant.pass] * ubo.view[PushConstant.pass] * model * skinnedMatrix * vec4(inPos.xyz, 1.0));
//	outPosition = vec3(ubo.view[PushConstant.pass] * model * skinnedMatrix * vec4(inPos.xyz, 1.0));
	outPosition = vec3(model * skinnedMatrix * vec4(inPos.xyz, 1.0));

//	outNormal = vec3(ubo.view[PushConstant.pass] * model * vec4(inNormal.xyz, 0.0));
	outNormal = vec3(model * vec4(inNormal.xyz, 0.0));
	outNormal = normalize(outNormal);


	{
		vec3 T = vec3(ubo.view[PushConstant.pass] * model * vec4(inTangent.xyz, 0.0));
		vec3 N = outNormal;
		vec3 B = cross(N, T) * inTangent.w;
		outTBN = mat3( T, B, N );
	}

	gl_Position = ubo.projection[PushConstant.pass] * ubo.view[PushConstant.pass] * model * skinnedMatrix * vec4(inPos.xyz, 1.0);
}