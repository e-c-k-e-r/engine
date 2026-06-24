#version 450
#pragma shader_stage(vertex)

#include "../../common/macros.h"
#include "../../common/structs.h"

layout (location = 0) out vec2 outUv;
layout (location = 1) out flat uint outPass;

layout( push_constant ) uniform PushBlock {
  uint pass;
  uint draw;
  uint aux;
} PushConstant;

layout (binding = 0) uniform UBO {
	Viewport viewport[6];
} camera;

const vec2 positions[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2(-1.0,  1.0),
    vec2(-1.0,  1.0),
    vec2( 1.0, -1.0),
    vec2( 1.0,  1.0)
);

const vec2 uvs[6] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(0.0, 1.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0)
);

void main() {
	outUv = uvs[gl_VertexIndex];
	outPass = PushConstant.pass;
	gl_Position = camera.viewport[PushConstant.pass].projection * camera.viewport[PushConstant.pass].view * vec4(positions[gl_VertexIndex], 0.0f, 1.0f);
}