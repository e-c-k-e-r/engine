#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUv;
layout (location = 2) in vec3 inNormal;

layout( push_constant ) uniform PushBlock {
  uint pass;
} PushConstant;

struct Matrices {
	mat4 model;
	mat4 view[2];
	mat4 projection[2];
};

layout (binding = 0) uniform UBO {
	Matrices matrices;
	vec4 color;
} ubo;

layout (location = 0) out vec2 outUv;
layout (location = 1) out vec4 outColor;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out vec3 outPosition;

out gl_PerVertex {
    vec4 gl_Position;   
};


void main() {
	outUv = inUv;
	outColor = ubo.color;

	outPosition = vec3(ubo.matrices.view[PushConstant.pass] * ubo.matrices.model * vec4(inPos.xyz, 1.0));
	outNormal = vec3(ubo.matrices.view[PushConstant.pass] * ubo.matrices.model * vec4(inNormal.xyz, 0.0));
	outNormal = normalize(outNormal);

	gl_Position = ubo.matrices.projection[PushConstant.pass] * ubo.matrices.view[PushConstant.pass] * ubo.matrices.model * vec4(inPos.xyz, 1.0);
}