#version 450
#pragma shader_stage(vertex)

layout (constant_id = 0) const uint PASSES = 6;
layout (location = 0) in vec2 inPos;
layout (location = 1) in vec2 inUv;

layout( push_constant ) uniform PushBlock {
  uint pass;
  uint draw;
} PushConstant;

struct Matrices {
	mat4 model[PASSES];
};
struct Gui {
	vec4 offset;
	vec4 color;
	int mode;
	float depth;
	int sdf;
	int shadowbox;
	vec4 stroke;
	float weight;
	int spread;
	float scale;
	float padding;
};

layout (binding = 0) uniform UBO {
	Matrices matrices;
	Gui gui;
} ubo;

layout (location = 0) out vec2 outUv;
layout (location = 1) out flat Gui outGui;

out gl_PerVertex {
    vec4 gl_Position;
};


void main() {
	outUv = inUv;
	outGui = ubo.gui;

	gl_Position = ubo.matrices.model[PushConstant.pass] * vec4(inPos.xy, ubo.gui.depth, 1.0);
}