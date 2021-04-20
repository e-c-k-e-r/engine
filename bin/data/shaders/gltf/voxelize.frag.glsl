#version 450

#define DEFERRED_SAMPLING 0
#define USE_LIGHTMAP 1

#define PI 3.1415926536f

layout (constant_id = 0) const uint TEXTURES = 1;

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
	int indexAtlas;
	int indexLightmap;
	int modeAlpha;
};
struct Texture {
	int index;
	int samp;
	int remap;
	float blend;

	vec4 lerp;
};
layout (std140, binding = 0) readonly buffer Materials {
	Material materials[];
};
layout (std140, binding = 1) readonly buffer Textures {
	Texture textures[];
};


layout (location = 0) in vec2 inUv;
layout (location = 1) in vec2 inSt;
layout (location = 2) in vec4 inColor;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in mat3 inTBN;
layout (location = 7) in vec3 inPosition;
layout (location = 8) flat in ivec4 inId;

layout (binding = 7) uniform sampler2D samplerTextures[TEXTURES];
layout (binding = 8, rg16ui) uniform volatile coherent uimage3D voxelID;
layout (binding = 9, rg16f) uniform volatile coherent image3D voxelNormal;
layout (binding = 10, rg16f) uniform volatile coherent image3D voxelUv;
layout (binding = 11, rgba8) uniform volatile coherent image3D voxelAlbedo;

layout (location = 0) out uvec2 outId;
layout (location = 1) out vec2 outNormals;
#if DEFERRED_SAMPLING
	layout (location = 2) out vec2 outUvs;
#else
	layout (location = 2) out vec4 outAlbedo;
#endif

vec2 encodeNormals( vec3 n ) {
//	return n.xy / sqrt(n.z*8+8) + 0.5;
	return (vec2(atan(n.y,n.x)/PI, n.z)+1.0)*0.5;
}
float wrap( float i ) {
	return fract(i);
}
vec2 wrap( vec2 uv ) {
	return vec2( wrap( uv.x ), wrap( uv.y ) );
}
float mipLevel( in vec2 uv ) {
	const vec2 dx_vtc = dFdx(uv);
	const vec2 dy_vtc = dFdy(uv);
	return 0.5 * log2(max(dot(dx_vtc, dx_vtc), dot(dy_vtc, dy_vtc)));
}
bool validTextureIndex( int textureIndex ) {
	return 0 <= textureIndex && textureIndex < textures.length();
}

void main() {
	const vec3 P = inPosition;
	if ( !(abs(P.x) < 1.0 && abs(P.y) < 1 && abs(P.z) < 1) ) discard;

	vec4 A = vec4(0, 0, 0, 0);
	const vec3 N = inNormal;
	const vec2 uv = wrap(inUv.xy);
	const float mip = mipLevel(inUv.xy);
	const int materialId = int(inId.y);
	const Material material = materials[materialId];

	const float M = material.factorMetallic;
	const float R = material.factorRoughness;
	const float AO = material.factorOcclusion;
	
	// sample albedo
	const bool useAtlas = validTextureIndex( material.indexAtlas );
	Texture textureAtlas;
	if ( useAtlas ) textureAtlas = textures[material.indexAtlas];
	if ( !validTextureIndex( material.indexAlbedo ) ) discard; {
		Texture t = textures[material.indexAlbedo];
		A = textureLod( samplerTextures[(useAtlas) ? textureAtlas.index : t.index], (useAtlas) ? mix( t.lerp.xy, t.lerp.zw, uv ) : uv, mip );
		// alpha mode OPAQUE
		if ( material.modeAlpha == 0 ) {
			A.a = 1;
		// alpha mode BLEND
		} else if ( material.modeAlpha == 1 ) {

		// alpha mode MASK
		} else if ( material.modeAlpha == 2 ) {
			if ( A.a < abs(material.factorAlphaCutoff) ) discard;
			A.a = 1;
		}
		if ( A.a == 0 ) discard;
	}
#if USE_LIGHTMAP
	if ( validTextureIndex( material.indexLightmap ) ) {
	#if DEFERRED_SAMPLING
		outUvs = inSt;
	#else
		Texture t = textures[material.indexLightmap];
		const float gamma = 1.6;
		const vec4 L = pow(textureLod( samplerTextures[t.index], inSt, mip ), vec4(1.0 / gamma));
		A *= L;
	#endif
	}
#endif

	const vec4 		outAlbedo = A * inColor;
	const uvec2 	outId = uvec2(inId.w+1, inId.y+1);
	const vec2 		outNormals = encodeNormals( normalize( N ) );
	const vec2 		outUvs = wrap(inUv.xy);

	imageStore(voxelID, ivec3(P * imageSize(voxelID)), uvec4(outId, 0, 0));
	imageStore(voxelNormal, ivec3(P * imageSize(voxelNormal)), vec4(outNormals, 0, 0));
	imageStore(voxelUv, ivec3(P * imageSize(voxelUv)), vec4(outUvs, 0, 0));
	imageStore(voxelAlbedo, ivec3(P * imageSize(voxelAlbedo)), outAlbedo);
}