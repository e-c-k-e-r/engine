#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUv;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in uint inColor;

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
layout (location = 1) out vec3 outPositionEye;
layout (location = 2) out vec3 outNormalEye;
layout (location = 3) out vec4 outColor;

out gl_PerVertex {
    vec4 gl_Position;   
};


void main() {
	outUv = inUv;
	outPositionEye = vec3(ubo.matrices.view[PushConstant.pass] * ubo.matrices.model * vec4(inPos.xyz, 1.0));
	outNormalEye = vec3(ubo.matrices.view[PushConstant.pass] * ubo.matrices.model * vec4(inNormal.xyz, 0.0));
//	outColor = ubo.color;

	outColor.a = (inColor >>  24u) & 0xFF;
	outColor.b = (inColor >>  16u) & 0xFF;
	outColor.g = (inColor >>   8u) & 0xFF;
	outColor.r = (inColor        ) & 0xFF;
	outColor.rgb /= 256.0;
	outColor.a = 0.5;

	gl_Position = ubo.matrices.projection[PushConstant.pass] * ubo.matrices.view[PushConstant.pass] * ubo.matrices.model * vec4(inPos.xyz, 1.0);
}