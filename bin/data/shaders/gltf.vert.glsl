#version 450

layout (constant_id = 0) const uint PASSES = 6;
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUv;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec4 inTangent;
layout (location = 4) in ivec2 inId;

layout( push_constant ) uniform PushBlock {
  uint pass;
  uint draw;
} PushConstant;

struct Matrices {
	mat4 model;
	mat4 view[PASSES];
	mat4 projection[PASSES];
};

layout (binding = 3) uniform UBO {
	Matrices matrices;
	vec4 color;
} ubo;

layout (location = 0) out vec2 outUv;
layout (location = 1) out vec4 outColor;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out mat3 outTBN;
layout (location = 6) out vec3 outPosition;
layout (location = 7) out ivec4 outId;

out gl_PerVertex {
    vec4 gl_Position;   
};

vec4 snap(vec4 vertex, vec2 resolution) {
    vec4 snappedPos = vertex;
    snappedPos.xyz = vertex.xyz / vertex.w;
    snappedPos.xy = floor(resolution * snappedPos.xy) / resolution;
    snappedPos.xyz *= vertex.w;
    return snappedPos;
}

void main() {
	outUv = inUv;
	outColor = ubo.color;
	outId = ivec4(inId, PushConstant.pass, PushConstant.draw);

	outPosition = vec3(ubo.matrices.view[PushConstant.pass] * ubo.matrices.model * vec4(inPos.xyz, 1.0));
	outNormal = vec3(ubo.matrices.view[PushConstant.pass] * ubo.matrices.model * vec4(inNormal.xyz, 0.0));
	outNormal = normalize(outNormal);


	{
		vec3 T = vec3(ubo.matrices.view[PushConstant.pass] * ubo.matrices.model * vec4(inTangent.xyz, 0.0));
		vec3 N = outNormal;
		vec3 B = cross(N, T) * inTangent.w;
		outTBN = mat3( T, B, N );
	}

	gl_Position = ubo.matrices.projection[PushConstant.pass] * ubo.matrices.view[PushConstant.pass] * ubo.matrices.model * vec4(inPos.xyz, 1.0);
//	gl_Position = snap( gl_Position, vec2(320.0, 240.0) );
}