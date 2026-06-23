#version 450
#pragma shader_stage(vertex)

#include "../../common/macros.h"
#include "../../common/structs.h"

layout (location = 0) out vec2 outUv;
layout (location = 1) out flat uint outPass;

layout( push_constant ) uniform PushBlock {
  uint pass;
  uint draw;
} PushConstant;

layout (binding = 0) uniform Camera {
	Viewport viewport[6];
} camera;

void main() {
	outUv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	outPass = PushConstant.pass;
	gl_Position = camera.viewport[PushConstant.pass].projection * camera.viewport[PushConstant.pass].view * vec4(outUv * 2.0f + -1.0f, 0.0f, 1.0f);
}