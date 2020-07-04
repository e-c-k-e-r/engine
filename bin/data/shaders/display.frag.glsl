#version 450

layout (set = 0, binding = 1) uniform texture2D tex;
layout (set = 0, binding = 2) uniform sampler samp;

layout (location = 0) in vec2 inUv;
layout (location = 0) out vec4 outFragColor;

void main() {
	outFragColor = texture(sampler2D(tex, samp), inUv);
//	outFragColor = vec4(inUv, 1.0f, 0.0f);
}