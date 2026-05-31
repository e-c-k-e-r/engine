#version 450
#pragma shader_stage(fragment)

layout (binding = 1) uniform sampler2D samplerTexture;

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec4 inColor;

layout (location = 0) out vec4 outColor;

void main() {
	vec4 color = texture( samplerTexture, inUv ) * inColor;
	if ( color.a == 0.0f ) discard;
	
	outColor.rgb = color.rgb;
	outColor.a = color.a;
}