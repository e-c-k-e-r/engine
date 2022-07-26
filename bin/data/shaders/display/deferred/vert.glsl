#version 450
#pragma shader_stage(vertex)

#if 0
layout (location = 0) in vec2 inPos;
layout (location = 1) in vec2 inUv;
#endif
layout (location = 0) out vec2 outUv;
layout (location = 1) out flat uvec2 outPushConstantPass;

layout( push_constant ) uniform PushBlock {
  uint pass;
  uint draw;
} PushConstant;

out gl_PerVertex {
    vec4 gl_Position;
};


void main() {
	outPushConstantPass = uvec2(PushConstant.pass, PushConstant.draw);
	outUv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(outUv * 2.0f + -1.0f, 0.0f, 1.0f);
#if 0
	outUv = inUv;
	gl_Position = vec4(inPos.xy, 0.0, 1.0);
#endif
}