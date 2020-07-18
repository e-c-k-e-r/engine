#version 450

layout (location = 0) in vec2 inPos;
layout (location = 1) in vec2 inUv;

struct Matrices {
	mat4 model;
};
struct Gui {
	vec4 offset;
	vec4 color;
	int mode;
	float depth;
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

	gl_Position = ubo.matrices.model * vec4(inPos.xy, ubo.gui.depth, 1.0);
}