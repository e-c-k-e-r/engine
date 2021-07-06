#version 450
#pragma shader_stage(vertex)

layout (constant_id = 0) const uint PASSES = 6;
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUv;
layout (location = 2) in vec2 inSt;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in vec4 inTangent;
layout (location = 5) in uvec2 inId;

layout( push_constant ) uniform PushBlock {
  uint pass;
  uint draw;
} PushConstant;

layout (std140, binding = 0) readonly buffer Models {
	mat4 models[];
};

layout (location = 0) out vec2 outUv;
layout (location = 1) out vec4 outColor;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out mat3 outTBN;
layout (location = 6) out vec3 outPosition;
layout (location = 7) out ivec4 outId;

out gl_PerVertex {
    vec4 gl_Position;   
};

void main() {
	outUv = inUv;
	outColor = vec4(1.0);
	outId = ivec4(inId, PushConstant.pass, PushConstant.draw);

	mat4 model = models.length() <= 0 ? mat4(1.0) : models[int(inId.x)];
	outPosition = vec3(model * vec4(inPos.xyz, 1.0));
	outNormal = vec3(model * vec4(inNormal.xyz, 0.0));
	outNormal = normalize(outNormal);

	{
		vec3 T = vec3(model * vec4(inTangent.xyz, 0.0));
		vec3 N = outNormal;
		vec3 B = cross(N, T) * inTangent.w;
		outTBN = mat3( T, B, N );
	}
	
	gl_Position = vec4(inSt * 2.0 - 1.0, 0.0, 1.0);
}