layout (constant_id = 0) const uint PASSES = 6;
#extension GL_ARB_shader_draw_parameters : enable
#if LAYERED
#extension GL_ARB_shader_viewport_layer_array : enable
#endif
#include "../common/structs.h"

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUv;
layout (location = 2) in vec4 inColor;
layout (location = 3) in vec2 inSt;
layout (location = 4) in vec3 inNormal;
layout (location = 5) in vec4 inTangent;
#if SKINNED
	layout (location = 6) in uvec4 inJoints;
	layout (location = 7) in vec4 inWeights;
#endif

layout( push_constant ) uniform PushBlock {
  uint pass;
  uint draw;
} PushConstant;

#if !BAKING
layout (binding = 0) uniform Camera {
	Viewport viewport[PASSES];
} camera;
#endif
layout (std140, binding = 1) readonly buffer DrawCommands {
	DrawCommand drawCommands[];
};
layout (std140, binding = 2) readonly buffer Instances {
	Instance instances[];
};

#if SKINNED
	layout (std140, binding = 3) readonly buffer Joints {
		mat4 joints[];
	};
#endif

layout (location = 0) out vec2 outUv;
layout (location = 1) out vec2 outSt;
layout (location = 2) out vec4 outColor;
layout (location = 3) out vec3 outNormal;
layout (location = 4) out mat3 outTBN;
layout (location = 7) out vec3 outPosition;
layout (location = 8) out uvec4 outId;
#if LAYERED
	layout (location = 9) out uint outLayer;
#endif

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
	const uint drawID = gl_DrawIDARB;
	const DrawCommand drawCommand = drawCommands[drawID];
	const uint instanceID = gl_InstanceIndex;
	const Instance instance = instances[instanceID];
	const uint materialID = instance.materialID;
	const uint jointID = instance.jointID;

#if BAKING
	const mat4 view = mat4(1);
	const mat4 projection = mat4(1);
#else
	const mat4 view = camera.viewport[PushConstant.pass].view;
	const mat4 projection = camera.viewport[PushConstant.pass].projection;
#endif
#if SKINNED 
	const mat4 skinned = joints.length() <= 0 || jointID < 0 ? mat4(1.0) : inWeights.x * joints[jointID + int(inJoints.x)] + inWeights.y * joints[jointID + int(inJoints.y)] + inWeights.z * joints[jointID + int(inJoints.z)] + inWeights.w * joints[jointID + int(inJoints.w)];
#else
	const mat4 skinned = mat4(1.0);
#endif
//	const mat4 model = instances.length() <= 0 ? skinned : (instance.model * skinned);
	const mat4 model = instance.model * skinned;

	outId = ivec4(drawID, instanceID, materialID, PushConstant.pass);
	outColor = instance.color;
	outPosition = vec3(model * vec4(inPos.xyz, 1.0));
	outNormal = vec3(model * vec4(inNormal.xyz, 0.0));
	outNormal = normalize(outNormal);

	vec3 T = vec3(view * model * vec4(inTangent.xyz, 0.0));
	vec3 N = outNormal;
	vec3 B = cross(N, T) * inTangent.w;
	outTBN = mat3( T, B, N );
#if BAKING
	gl_Position = vec4(inSt * 2.0 - 1.0, 0.0, 1.0);
#else
	gl_Position = projection * view * model * vec4(inPos.xyz, 1.0);
#endif

#if LAYERED
//	gl_Layer = int(drawCommand.auxID);
 	outLayer = int(drawCommand.auxID);
#endif
}