#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUv;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec4 inTangent;
layout (location = 4) in ivec2 inId;

layout( push_constant ) uniform PushBlock {
  uint pass;
} PushConstant;

layout (binding = 2) uniform UBO {
	mat4 view[2];
	mat4 projection[2];
} ubo;

layout (std140, binding = 3) buffer Models {
	mat4 models[];
};

layout (location = 0) noperspective out vec2 outUv;
layout (location = 1) out vec4 outColor;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out mat3 outTBN;
layout (location = 6) out vec3 outPosition;
layout (location = 7) flat out uint outId;
layout (location = 8) out float affine;

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
	outColor = vec4(1.0);

	mat4 model = models.length() <= 0 ? mat4(1.0) : models[int(inId.x)];
	outPosition = vec3(ubo.view[PushConstant.pass] * model * vec4(inPos.xyz, 1.0));
	outNormal = vec3(ubo.view[PushConstant.pass] * model * vec4(inNormal.xyz, 0.0));
	outNormal = normalize(outNormal);

	outId = inId.y;

	{
		vec3 T = vec3(ubo.view[PushConstant.pass] * model * vec4(inTangent.xyz, 0.0));
		vec3 N = outNormal;
		vec3 B = cross(N, T) * inTangent.w;
		outTBN = mat3( T, B, N );
	}

	gl_Position = ubo.projection[PushConstant.pass] * ubo.view[PushConstant.pass] * model * vec4(inPos.xyz, 1.0);

//	gl_Position = snap( gl_Position, vec2(320.0, 240.0) );

	affine = 1;
}