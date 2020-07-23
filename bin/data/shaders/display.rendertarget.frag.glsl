#version 450

layout (binding = 1) uniform sampler2D samplerTexture;

layout (location = 0) in vec2 inUv;
layout (location = 0) out vec4 outFragColor;

void main() {
	outFragColor = texture(samplerTexture, inUv);
//	outFragColor.a = 1.0f;
//	outFragColor = vec4( inUv, 0.0f, 1.0f );
}