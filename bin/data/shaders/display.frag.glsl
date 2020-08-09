#version 450

layout (input_attachment_index = 0, binding = 1) uniform subpassInput samplerOutput;

layout (location = 0) in vec2 inUv;
layout (location = 0) out vec4 outFragColor;

void main() {
	outFragColor = subpassLoad(samplerOutput);
}