#version 450

layout (location = 0) in vec3 inPos;

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

layout (location = 0) out vec4 outColor;

out gl_PerVertex {
    vec4 gl_Position;   
};


void main() {
	outColor = ubo.color;

//	outPosition = vec3(ubo.matrices.view[PushConstant.pass] * ubo.matrices.model * vec4(inPos.xyz, 1.0));

	gl_Position = ubo.matrices.projection[PushConstant.pass] * ubo.matrices.view[PushConstant.pass] * ubo.matrices.model * vec4(inPos.xyz, 1.0);
}