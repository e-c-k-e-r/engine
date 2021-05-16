#version 450
#pragma shader_stage(fragment)

layout (location = 0) in vec3 inColor;

layout (location = 0) out vec4 outAlbedoSpecular;

void main() {
	outAlbedoSpecular.rgb *= inColor.rgb;
	outAlbedoSpecular.a = 1;
}