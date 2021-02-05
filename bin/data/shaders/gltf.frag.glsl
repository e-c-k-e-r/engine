#version 450

#define UF_DEFERRED_SAMPLING 1
#define UF_CAN_DISCARD 1

layout (constant_id = 0) const uint TEXTURES = 1;
layout (binding = 0) uniform sampler2D samplerTextures[TEXTURES];

struct Material {
	vec4 colorBase;
	vec4 colorEmissive;

	float factorMetallic;
	float factorRoughness;
	float factorOcclusion;
	float factorAlphaCutoff;

	int indexAlbedo;
	int indexNormal;
	int indexEmissive;
	int indexOcclusion;
	
	int indexMetallicRoughness;
	
	int padding1;
	int padding2;
	int padding3;
};
struct Texture {
	int index;
	int samp;
	int remap;
	float blend;

	vec4 lerp;
};
layout (std140, binding = 1) readonly buffer Materials {
	Material materials[];
};
layout (std140, binding = 2) readonly buffer Textures {
	Texture textures[];
};

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec4 inColor;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in mat3 inTBN;
layout (location = 6) in vec3 inPosition;
layout (location = 7) flat in ivec4 inId;

layout (location = 0) out uvec2 outId;
layout (location = 1) out vec2 outNormals;

#if UF_DEFERRED_SAMPLING
	layout (location = 2) out vec2 outUvs;
#else
	layout (location = 2) out vec4 outAlbedo;
#endif

vec2 encodeNormals( vec3 n ) {
	float p = sqrt(n.z*8+8);
	return n.xy/p + 0.5;
}

bool validTextureIndex( int textureIndex ) {
	return 0 <= textureIndex && textureIndex < TEXTURES;
}
vec4 sampleTexture( int textureIndex, vec2 uv, vec4 base ) {
	if  ( !validTextureIndex( textureIndex ) ) return base;
	Texture t = textures[textureIndex+1];
//	if ( 0 <= t.remap && t.remap <= TEXTURES && t.remap != textureIndex  ) t = textures[t.remap+1];
 	return texture( samplerTextures[0], mix( t.lerp.xy, t.lerp.zw, uv ) );
}

void main() {
	vec4 C = vec4(0, 0, 0, 0);
	vec3 P = inPosition;
	vec3 N = inNormal;
#if UF_DEFERRED_SAMPLING
	outUvs = fract(inUv);
	vec4 outAlbedo = vec4(0,0,0,0);
#endif
#if !UF_DEFERRED_SAMPLING || UF_CAN_DISCARD
	int materialId = int(inId.y);
	Material material = materials[materialId];
	
	vec2 uv = fract(inUv.xy);

	float M = material.factorMetallic;
	float R = material.factorRoughness;
	float AO = material.factorOcclusion;

	// sample albedo
	// C = sampleTexture( material.indexAlbedo, uv, material.colorBase );
	if ( !validTextureIndex( material.indexAlbedo ) ) {
		discard;
	}
	{
		Texture t = textures[material.indexAlbedo + 1];
		C = texture( samplerTextures[0], mix( t.lerp.xy, t.lerp.zw, uv ) );
		if ( C.a < abs(material.factorAlphaCutoff) ) discard;
	}
#endif

#if !UF_DEFERRED_SAMPLING
	// sample normal
	if ( validTextureIndex( material.indexNormal ) ) {
		Texture t = textures[material.indexNormal + 1];
		N = inTBN * normalize( texture( samplerTextures[0], mix( t.lerp.xy, t.lerp.zw, uv ) ).xyz * 2.0 - vec3(1.0));
	}

#if 0
	// sample metallic/roughness
	if ( validTextureIndex( material.indexMetallicRoughness ) ) {
		Texture t = textures[material.indexMetallicRoughness + 1];
		vec4 sampled = texture( samplerTextures[0], mix( t.lerp.xy, t.lerp.zw, uv ) );
		M = sampled.b;
		R = sampled.g;
	}
	// sample ao
	AO = material.factorOcclusion;
	if ( validTextureIndex( material.indexOcclusion ) ) {
		Texture t = textures[material.indexMetallicRoughness + 1];
		AO = texture( samplerTextures[0], mix( t.lerp.xy, t.lerp.zw, uv ) ).r;
	}
#endif

	C.rgb *= inColor.rgb * C.a;
	outAlbedo = vec4(C.rgb,1);
#endif

	outNormals = encodeNormals( N );
	outId = ivec2(inId.w, inId.y);
}

/*
layout (location = 0) out vec4 outAlbedoMetallic;
layout (location = 1) out vec4 outNormalRoughness;
layout (location = 2) out vec4 outPositionAO;

bool validIndex( int index ) {
	return 0 <= index && index < TEXTURES;
}
vec4 sampleTexture( int index ) {
	return texture(samplerTextures[index], inUv.xy);
}

void main() {
	int materialId = int(inId.y);
	
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
		} else if ( validIndex( material.indexMappedTarget ) ) {
			C = sampleTexture( material.indexMappedTarget );
		}
		// debug
		if ( material.factorAlphaCutoff < 0 ) {
			float percent = float(material.indexAlbedo) / float(TEXTURES);
			if ( percent < 0.33 ) {
				C.rgb = vec3( percent * 3, 0, 0 );
			} else if ( percent < 0.66 ) {
				C.rgb = vec3( 0, percent * 3, 0 );
			} else {
				C.rgb = vec3( 0, 0, percent * 3 );
			}
		}
		if ( C.a < abs(material.factorAlphaCutoff) ) discard;

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

	C.rgb *= inColor.rgb * C.a;
	
	outAlbedoMetallic = vec4(C.rgb,M);
	outNormalRoughness = vec4(N,R);
	outPositionAO = vec4(P,AO);
}
*/