#version 450

layout (binding = 1) uniform sampler2D samplerColor;

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec4 inColor;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inPosition;

layout (location = 0) out vec4 outAlbedoSpecular;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outPosition;

void main() {
	outAlbedoSpecular = texture(samplerColor, inUv);
	// outAlbedoSpecular.rgb *= inColor.rgb;
	outAlbedoSpecular.a = 1;
	outNormal = vec4(inNormal,1);
	outPosition = vec4(inPosition,1);
}