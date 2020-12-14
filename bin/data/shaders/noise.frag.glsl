#version 450

layout (binding = 1) uniform sampler3D samplerColor;

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec4 inColor;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inPosition;

layout (location = 0) out vec4 outAlbedoSpecular;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outPosition;

layout (binding = 2) uniform UBO {
	float slice;
} ubo;

void main() {
	outAlbedoSpecular.rgb = vec3(texture(samplerColor, vec3(inUv,ubo.slice)).r);
	outAlbedoSpecular.a = 1;	
	outNormal = vec4(inNormal,1);
	outPosition = vec4(inPosition,1);
}