#version 450

layout (constant_id = 0) const uint PASSES = 6;
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUv;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in uint inColor;

layout( push_constant ) uniform PushBlock {
  uint pass;
  uint draw;
} PushConstant;

struct Matrices {
	mat4 model;
	mat4 view[PASSES];
	mat4 projection[PASSES];
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
//	outColor = ubo.color;

	outPosition = vec3(ubo.matrices.view[PushConstant.pass] * ubo.matrices.model * vec4(inPos.xyz, 1.0));
	outNormal = vec3(ubo.matrices.view[PushConstant.pass] * ubo.matrices.model * vec4(inNormal.xyz, 0.0));
	outNormal = normalize(outNormal);

	outColor.a = (inColor >>  24u) & 0xFF;
	outColor.b = (inColor >>  16u) & 0xFF;
	outColor.g = (inColor >>   8u) & 0xFF;
	outColor.r = (inColor        ) & 0xFF;
	outColor.rgb /= 256.0;
	outColor.a = 0.5;

	gl_Position = ubo.matrices.projection[PushConstant.pass] * ubo.matrices.view[PushConstant.pass] * ubo.matrices.model * vec4(inPos.xyz, 1.0);
}