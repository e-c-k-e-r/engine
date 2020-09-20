#version 450

layout (binding = 0) uniform sampler2D samplerColor;
layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outFragColor;

void main() {
	vec4 sampledColor = texture(samplerColor, inUV);
	if ( sampledColor.r <= 0.000001 && sampledColor.g <= 0.000001 && sampledColor.b <= 0.000001 ) discard;
	outFragColor = sampledColor;
}