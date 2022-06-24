#version 450
#pragma shader_stage(vertex)

struct Cursor {
	vec2 position;
	vec2 radius;
	vec4 color;
};

layout (location = 0) out vec2 outUv;
layout (location = 1) out float outAlpha;
layout (location = 2) out flat Cursor outCursor;

layout( push_constant ) uniform PushBlock {
  uint pass;
  uint draw;
} PushConstant;

struct Matrices {
	mat4 model[2];
};
layout (binding = 0) uniform UBO {
	Matrices matrices;
	Cursor cursor;
//	float alpha;
//	float padding;
} ubo;

void main() {
	outUv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	outCursor = ubo.cursor;
	outAlpha = 1;
	gl_Position = ubo.matrices.model[PushConstant.pass] * vec4(outUv * 2.0f + -1.0f, 0.0f, 1.0f);
}