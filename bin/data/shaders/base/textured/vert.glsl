#version 450
#pragma shader_stage(vertex)

#extension GL_EXT_multiview : enable

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
  uint aux;
} PushConstant;

layout (binding = 0) uniform Camera {
	Viewport viewport[PASSES];
} camera;

layout (location = 0) out vec2 outUv;
layout (location = 1) out vec4 outColor;

void main() {
	uint viewportIndex = gl_ViewIndex;
	if ( PushConstant.aux == 1 ) viewportIndex += 2;

	outUv = inUv;
	outColor = inColor;
	
	vec4 clipPos = camera.viewport[viewportIndex].projection * camera.viewport[viewportIndex].view * vec4(inPos.xyz, 1.0);
	//clipPos.xy += inOffset * (clipPos.w / max(clipPos.w, 1.0));
	clipPos.xy += inOffset * min(clipPos.w, 1.0);
	gl_Position = clipPos;}