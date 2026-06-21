layout (constant_id = 0) const uint PASSES = 6;
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_EXT_multiview : enable

#include "../../common/macros.h"
#include "../../common/structs.h"

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
  uint aux;
} PushConstant;

layout (binding = 0) uniform Camera {
	Viewport viewport[6];
} camera;

layout (std140, binding = 1) readonly buffer DrawCommands {
	DrawCommand drawCommands[];
};
layout (std140, binding = 2) readonly buffer Instances {
	Instance instances[];
};
layout (std140, binding = 3) readonly buffer Objects {
	Object objects[];
};

#if SKINNED
	layout (std140, binding = 4) readonly buffer Joints {
		mat4 joints[];
	};
#endif

layout (location = 0) flat out uvec4 outId;
layout (location = 1) flat out vec4 outPOS0;
layout (location = 2) out vec4 outPOS1;
layout (location = 3) out vec3 outPosition;
layout (location = 4) out vec2 outUv;
layout (location = 5) out vec4 outColor;
layout (location = 6) out vec2 outSt;
layout (location = 7) out vec3 outNormal;
layout (location = 8) out vec3 outTangent;
layout (location = 9) out vec3 outBitangent;

vec4 snap(vec4 vertex, vec2 resolution) {
    vec4 snappedPos = vertex;
    snappedPos.xyz = vertex.xyz / vertex.w;
    snappedPos.xy = floor(resolution * snappedPos.xy) / resolution;
    snappedPos.xyz *= vertex.w;
    return snappedPos;
}

void main() {
	const uint drawID = gl_DrawIDARB;
	const uint triangleID = gl_VertexIndex / 3;
	const DrawCommand drawCommand = drawCommands[drawID];
	const uint instanceID = gl_InstanceIndex; // drawCommand.instanceID; // gl_InstanceIndex;
	const Instance instance = instances[instanceID];
	const Object object = objects[instance.objectID];
	const uint jointID = instance.jointID;

	uint viewportIndex = gl_ViewIndex;
	if ( PushConstant.aux == 1 ) viewportIndex += 2;

#if BAKING
	const mat4 view = mat4(1);
	const mat4 projection = mat4(1);
#else
	const mat4 view = camera.viewport[viewportIndex].view;
	const mat4 projection = camera.viewport[viewportIndex].projection;
#endif
#if SKINNED 
	const mat4 skinned = joints.length() <= 0 || jointID < 0 ? mat4(1.0) : inWeights.x * joints[jointID + int(inJoints.x)] + inWeights.y * joints[jointID + int(inJoints.y)] + inWeights.z * joints[jointID + int(inJoints.z)] + inWeights.w * joints[jointID + int(inJoints.w)];
#else
	const mat4 skinned = mat4(1.0);
#endif
	const mat4 model = instances.length() <= 0 ? skinned : (object.model * skinned);
//	const mat4 model = object.model * skinned;


#if BAKING
	gl_Position = vec4(inSt * 2.0 - 1.0, 0.0, 1.0);
#else
	gl_Position = projection * view * model * vec4(inPos.xyz, 1.0);
#endif
	outPOS0 = gl_Position;
	outPOS1 = gl_Position;

	outId = uvec4(triangleID, drawID, instanceID, /*PushConstant.pass*/gl_ViewIndex);
	
	outPosition = vec3(model * vec4(inPos.xyz, 1.0));
	outUv = inUv;
	outSt = inSt;
	outColor = inColor * object.color;
	outNormal = normalize(vec3(model * vec4(inNormal.xyz, 0.0)));
	outTangent = normalize(vec3(model * vec4(inTangent)));
	outBitangent = normalize(vec3(model * vec4(cross( inNormal.xyz, inTangent.xyz ), 0.0)));
}