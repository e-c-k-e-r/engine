#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUv;
layout (location = 2) in vec3 inNormal;

struct Matrices {
	mat4 model;
	mat4 view;
	mat4 projection;
};

layout (binding = 0) uniform UBO {
	Matrices matrices;
	vec4 color;
} ubo;

layout (location = 0) out vec2 outUv;
layout (location = 1) out vec3 outPositionEye;
layout (location = 2) out vec3 outNormalEye;
layout (location = 3) out vec4 outColor;

out gl_PerVertex {
    vec4 gl_Position;   
};


void main() {
	outUv = inUv;
	outPositionEye = vec3(ubo.matrices.view * ubo.matrices.model * vec4(inPos.xyz, 1.0));
	outNormalEye = vec3(ubo.matrices.view * ubo.matrices.model * vec4(inNormal.xyz, 0.0));
	outColor = ubo.color;

	gl_Position = ubo.matrices.projection * ubo.matrices.view * ubo.matrices.model * vec4(inPos.xyz, 1.0);
}