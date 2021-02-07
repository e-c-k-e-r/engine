#version 450

#define UF_DEFERRED_SAMPLING 0
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
	int modeAlpha;
	int padding1;
	int padding2;
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

float wrap( float i ) {
	return fract(i);
}
vec2 wrap( vec2 uv ) {
	return vec2( wrap( uv.x ), wrap( uv.y ) );
}
float mipLevel( in vec2 uv ) {
	vec2 dx_vtc = dFdx(uv);
	vec2 dy_vtc = dFdy(uv);
	return 0.5 * log2(max(dot(dx_vtc, dx_vtc), dot(dy_vtc, dy_vtc)));
}
bool validTextureIndex( int textureIndex ) {
	return 0 <= textureIndex && textureIndex < TEXTURES;
}
void main() {
	float mip = mipLevel(inUv.xy);
	vec2 uv = wrap(inUv.xy);
	vec4 C = vec4(0, 0, 0, 0);
	vec3 P = inPosition;
	vec3 N = inNormal;
#if UF_DEFERRED_SAMPLING
	outUvs = wrap(inUv.xy);
	vec4 outAlbedo = vec4(0,0,0,0);
#endif
#if !UF_DEFERRED_SAMPLING || UF_CAN_DISCARD
	int materialId = int(inId.y);
	Material material = materials[materialId];
	
	float M = material.factorMetallic;
	float R = material.factorRoughness;
	float AO = material.factorOcclusion;

	// sample albedo
	if ( !validTextureIndex( material.indexAlbedo ) ) discard; {
		Texture t = textures[material.indexAlbedo + 1];
		C = textureLod( samplerTextures[0], mix( t.lerp.xy, t.lerp.zw, uv ), mip );
		// alpha mode MASK
		if ( material.modeAlpha == 0 ) C.a = 1;
		else if ( material.modeAlpha == 2 && C.a < abs(material.factorAlphaCutoff) ) discard;
		// alpha mode OPAQUE
	}
#endif

#if !UF_DEFERRED_SAMPLING
	// sample normal
	if ( validTextureIndex( material.indexNormal ) ) {
		Texture t = textures[material.indexNormal + 1];
		N = inTBN * normalize( textureLod( samplerTextures[0], mix( t.lerp.xy, t.lerp.zw, uv ), mip ).xyz * 2.0 - vec3(1.0));
	}

#if 0
	// sample metallic/roughness
	if ( validTextureIndex( material.indexMetallicRoughness ) ) {
		Texture t = textures[material.indexMetallicRoughness + 1];
		vec4 sampled = texture( samplerTextures[0], mix( t.lerp.xy, t.lerp.zw, uv ), mip );
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
	outAlbedo = C * inColor;
#endif

	outNormals = encodeNormals( N );
	outId = ivec2(inId.w+1, inId.y+1);
}