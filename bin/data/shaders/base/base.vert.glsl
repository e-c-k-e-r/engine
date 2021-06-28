#version 450
#pragma shader_stage(vertex)

#include "../common/structs.h"

layout (constant_id = 0) const uint PASSES = 6;
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUv;
layout (location = 2) in vec3 inNormal;

layout( push_constant ) uniform PushBlock {
  uint pass;
  uint draw;
} PushConstant;

layout (binding = 0) uniform UBO {
	mat4 model;
// 	vec4 color;
} ubo;

layout (binding = 1) uniform Camera {
	Viewport viewport[PASSES];
} camera;

layout (location = 0) out vec2 outUv;
layout (location = 1) out vec4 outColor;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out vec3 outPosition;

void main() {
	outUv = inUv;
	outColor = vec4(1); // ubo.color;

	outPosition = vec3(camera.viewport[PushConstant.pass].view * ubo.model * vec4(inPos.xyz, 1.0));
	outNormal = vec3(camera.viewport[PushConstant.pass].view * ubo.model * vec4(inNormal.xyz, 0.0));
	outNormal = normalize(outNormal);

	gl_Position = camera.viewport[PushConstant.pass].projection * camera.viewport[PushConstant.pass].view * ubo.model * vec4(inPos.xyz, 1.0);
}