#version 450
#pragma shader_stage(vertex)

#define INSTANCED 1
#define SKINNED 0
#define BAKING 1
#if 1
#include "../base.vert.h"
#else
#extension GL_ARB_shader_draw_parameters : enable
#include "../../common/structs.h"

layout (constant_id = 0) const uint PASSES = 6;
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUv;
layout (location = 2) in vec2 inSt;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in vec4 inTangent;
#if SKINNED
	layout (location = 5) in uvec4 inJoints;
	layout (location = 6) in vec4 inWeights;
#endif

layout( push_constant ) uniform PushBlock {
  uint pass;
  uint draw;
} PushConstant;

/*
layout (std140, binding = 0) readonly buffer DrawCommands {
	DrawCommand drawCommands[];
};
*/
layout (std140, binding = 1) readonly buffer Instances {
	Instance instances[];
};

#if SKINNED
	layout (std140, binding = 2) readonly buffer Joints {
		mat4 joints[];
	};
#endif

layout (location = 0) out vec2 outUv;
layout (location = 1) out vec2 outSt;
layout (location = 2) out vec4 outColor;
layout (location = 3) out vec3 outNormal;
layout (location = 4) out mat3 outTBN;
layout (location = 7) out vec3 outPosition;
layout (location = 8) flat out uvec4 outId;

out gl_PerVertex {
    vec4 gl_Position;   
};

void main() {
	outUv = inUv;
	outSt = inSt;
	
#if SKINNED 
	const mat4 skinned = joints.length() <= 0 ? mat4(1.0) : inWeights.x * joints[int(inJoints.x)] + inWeights.y * joints[int(inJoints.y)] + inWeights.z * joints[int(inJoints.z)] + inWeights.w * joints[int(inJoints.w)];
#else
	const mat4 skinned = mat4(1.0);
#endif
	const uint drawID = gl_DrawIDARB;
//	const DrawCommand drawCommand = drawCommands[drawID];
	const uint instanceID = gl_InstanceIndex;
	const Instance instance = instances[instanceID];
	const uint materialID = instance.materialID;
	const mat4 model = instances.length() <= 0 ? skinned : (instance.model * skinned);

	outId = ivec4(drawID, instanceID, materialID, PushConstant.pass);
	outColor = instance.color;
	outPosition = vec3(model * vec4(inPos.xyz, 1.0));
	outNormal = vec3(model * vec4(inNormal.xyz, 0.0));
	outNormal = normalize(outNormal);

	vec3 T = vec3(model * vec4(inTangent.xyz, 0.0));
	vec3 N = outNormal;
	vec3 B = cross(N, T) * inTangent.w;
	outTBN = mat3( T, B, N );

	gl_Position = vec4(inSt * 2.0 - 1.0, 0.0, 1.0);
}
#endif