#version 450
#pragma shader_stage(vertex)

#include "../../common/macros.h"
#include "../../common/structs.h"

layout (constant_id = 0) const uint PASSES = 1;

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUv;
layout (location = 2) in vec4 inColor;
layout (location = 3) in vec2 inOffset;

layout( push_constant ) uniform PushBlock {
  uint pass;
  uint draw;
} PushConstant;

layout (binding = 0) uniform Camera {
	Viewport viewport[PASSES];
} camera;

layout (location = 0) out vec2 outUv;
layout (location = 1) out vec4 outColor;

void main() {
	outUv = inUv;
	outColor = inColor;
	
	vec4 clipPos = camera.viewport[PushConstant.pass].projection * camera.viewport[PushConstant.pass].view * vec4(inPos.xyz, 1.0);
	//clipPos.xy += inOffset * (clipPos.w / max(clipPos.w, 1.0));
	clipPos.xy += inOffset * min(clipPos.w, 1.0);
	gl_Position = clipPos;}