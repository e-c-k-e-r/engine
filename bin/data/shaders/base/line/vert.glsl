#version 450
#pragma shader_stage(vertex)

#include "../../common/macros.h"
#include "../../common/structs.h"

layout (constant_id = 0) const uint PASSES = 1;

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec4 inColor;

layout( push_constant ) uniform PushBlock {
  uint pass;
  uint draw;
} PushConstant;

layout (binding = 0) uniform Camera {
	Viewport viewport[PASSES];
} camera;

layout (location = 0) out vec4 outColor;

void main() {
	outColor = inColor;
	gl_Position = camera.viewport[PushConstant.pass].projection * camera.viewport[PushConstant.pass].view * vec4(inPos.xyz, 1.0);
}