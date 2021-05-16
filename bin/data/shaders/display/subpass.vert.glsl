#version 450
#pragma shader_stage(vertex)

layout (location = 0) in vec2 inPos;
layout (location = 1) in vec2 inUv;

layout (location = 0) out vec2 outUv;
layout (location = 1) out flat uint outPushConstantPass;

layout( push_constant ) uniform PushBlock {
  uint pass;
  uint draw;
} PushConstant;

out gl_PerVertex {
    vec4 gl_Position;
};


void main() {
	outUv = inUv;
	outPushConstantPass = PushConstant.pass;
	
	gl_Position = vec4(inPos.xy, 0.0, 1.0);
}