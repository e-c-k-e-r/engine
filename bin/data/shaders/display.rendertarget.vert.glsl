#version 450

layout (location = 0) in vec2 inPos;
layout (location = 1) in vec2 inUv;

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
} PushConstant;

out gl_PerVertex {
    vec4 gl_Position;
};

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
	outUv = inUv;
	outCursor = ubo.cursor;
	outAlpha = 1;

	gl_Position = ubo.matrices.model[PushConstant.pass] * vec4(inPos.xy, 0.0, 1.0);
}