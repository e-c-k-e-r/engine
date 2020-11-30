#version 450

layout (constant_id = 0) const uint TEXTURES = 1;
layout (binding = 2) uniform sampler2D samplerTextures[TEXTURES];

struct Material {
	vec4 colorBase;
	vec4 colorEmissive;
	float factorMetallic;
	float factorRoughness;
	float factorOcclusion;
	float factorMappedBlend;
	int indexAlbedo;
	int indexNormal;
	int indexEmissive;
	int indexOcclusion;
	int indexMetallicRoughness;
	int indexMappedTarget;
};
layout (std140, binding = 3) buffer Materials {
	Material materials[];
};

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec4 inColor;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in mat3 inTBN;
layout (location = 6) in vec3 inPosition;
layout (location = 7) flat in uint inId;
layout (location = 8) in float inAffine;

layout (location = 0) out vec4 outAlbedoMetallic;
layout (location = 1) out vec4 outNormalRoughness;
layout (location = 2) out vec4 outPositionAO;

bool validIndex( int index ) {
	return 0 <= index && index <= TEXTURES;
}
vec4 sampleTexture( int index ) {
	return texture(samplerTextures[index], inUv.xy / inAffine);
}

void main() {
	int materialId = int(inId);
	
	vec4 C = vec4(1, 0, 1, 1);
	vec3 N = inNormal;
	vec3 P = inPosition;
	float M = 1;
	float R = 1;
	float AO = 1;

	if ( 0 <= materialId && materialId < materials.length() ) {
		Material material = materials[materialId];

		// sample albedo
		C = material.colorBase;
		if ( validIndex( material.indexAlbedo ) ) {
			C = sampleTexture( material.indexAlbedo );
			if ( material.indexAlbedo != material.indexMappedTarget && validIndex( material.indexMappedTarget ) ) {
				C = mix( C, sampleTexture( material.indexMappedTarget ), material.factorMappedBlend );
			}
		}
		// sample normal
		if ( validIndex( material.indexNormal ) ) {
			N = inTBN * normalize( sampleTexture( material.indexNormal ).xyz * 2.0 - vec3(1.0));
		}
		// sample metallic/roughness
		M = material.factorMetallic;
		R = material.factorRoughness;
		if ( validIndex( material.indexMetallicRoughness ) ) {
			vec4 sampled = sampleTexture( material.indexMetallicRoughness );
			M = sampled.b;
			R = sampled.g;
		}
		AO = material.factorOcclusion;
		if ( validIndex( material.indexOcclusion ) ) {
			AO = sampleTexture( material.indexOcclusion ).r;
		}
	}
	if ( C.a < 0.001 ) discard;
	C.rgb *= inColor.rgb;
	
	outAlbedoMetallic = vec4(C.rgb,M);
	outNormalRoughness = vec4(N,R);
	outPositionAO = vec4(P,AO);
}