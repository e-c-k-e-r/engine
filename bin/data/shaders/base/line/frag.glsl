#version 450
#pragma shader_stage(fragment)

layout (location = 0) in vec4 inColor;
layout (location = 0) out vec4 outColor;

void main() {
	// needs to be specified like this for some unknown reason
	outColor.rgb = inColor.rgb;
	outColor.a = inColor.a;
}