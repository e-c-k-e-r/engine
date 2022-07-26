#version 450
#pragma shader_stage(fragment)

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;

layout (location = 0) out uvec2 outId;
layout (location = 1) out vec2 outNormals;
layout (location = 2) out vec4 outAlbedo;

void main() {
	outAlbedo = vec4(inColor, 1);
}