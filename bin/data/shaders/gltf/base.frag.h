#extension GL_EXT_nonuniform_qualifier : enable

#define CUBEMAPS 1
#define MAX_TEXTURES textures.length()
layout (constant_id = 0) const uint TEXTURES = 1;

#include "../common/macros.h"
#include "../common/structs.h"

layout (binding = 0) uniform sampler2D samplerTextures[TEXTURES];

layout (std140, binding = 1) readonly buffer Materials {
	Material materials[];
};
layout (std140, binding = 2) readonly buffer Textures {
	Texture textures[];
};

#include "../common/functions.h"

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec2 inSt;
layout (location = 2) in vec4 inColor;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in mat3 inTBN;
layout (location = 7) in vec3 inPosition;
layout (location = 8) flat in ivec4 inId;

layout (location = 0) out uvec2 outId;
layout (location = 1) out vec2 outNormals;
#if DEFERRED_SAMPLING
	layout (location = 2) out vec2 outUvs;
#else
	layout (location = 2) out vec4 outAlbedo;
#endif

void main() {
	const float mip = mipLevel(inUv.xy);
	const vec2 uv = wrap(inUv.xy);
	const vec3 P = inPosition;
	vec3 N = inNormal;
	vec4 A = vec4(0, 0, 0, 0);
#if DEFERRED_SAMPLING
	vec4 outAlbedo = vec4(0,0,0,0);
#endif
#if !DEFERRED_SAMPLING || CAN_DISCARD
	const int materialId = int(inId.y);
	Material material = materials[materialId];
	
	float M = material.factorMetallic;
	float R = material.factorRoughness;
	float AO = material.factorOcclusion;
	
	// sample albedo
	const bool useAtlas = validTextureIndex( material.indexAtlas );
	Texture textureAtlas;
	if ( useAtlas ) textureAtlas = textures[material.indexAtlas];
	if ( !validTextureIndex( material.indexAlbedo ) ) discard; {
		Texture t = textures[material.indexAlbedo];
		A = textureLod( samplerTextures[nonuniformEXT((useAtlas) ? textureAtlas.index : t.index)], (useAtlas) ? mix( t.lerp.xy, t.lerp.zw, uv ) : uv, mip );
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
		const vec4 L = pow(textureLod( samplerTextures[nonuniformEXT(t.index)], inSt, mip ), vec4(1.0 / gamma));
		A *= L;
	#endif
	}
#endif
#endif
#if !DEFERRED_SAMPLING
	// sample normal
	if ( validTextureIndex( material.indexNormal ) ) {
		Texture t = textures[material.indexNormal];
		N = inTBN * normalize( textureLod( samplerTextures[nonuniformEXT((useAtlas)?textureAtlas.index:t.index)], ( useAtlas ) ? mix( t.lerp.xy, t.lerp.zw, uv ) : uv, mip ).xyz * 2.0 - vec3(1.0));
	}
	#if 0
		// sample metallic/roughness
		if ( validTextureIndex( material.indexMetallicRoughness ) ) {
			Texture t = textures[material.indexMetallicRoughness];
			const vec4 sampled = textureLod( samplerTextures[nonuniformEXT((useAtlas)?textureAtlas.index:t.index)], ( useAtlas ) ? mix( t.lerp.xy, t.lerp.zw, uv ) : uv, mip );
			M = sampled.b;
			R = sampled.g;
		}
		// sample ao
		AO = material.factorOcclusion;
		if ( validTextureIndex( material.indexOcclusion ) ) {
			Texture t = textures[material.indexOcclusion];
			AO = textureLod( samplerTextures[nonuniformEXT((useAtlas)?textureAtlas.index:t.index)], ( useAtlas ) ? mix( t.lerp.xy, t.lerp.zw, uv ) : uv ).r;
		}
	#endif
	outAlbedo = A * inColor;
#else
	outUvs = wrap(inUv.xy);
#endif
	outNormals = encodeNormals( N );
	outId = ivec2(inId.w+1, inId.y+1);
}