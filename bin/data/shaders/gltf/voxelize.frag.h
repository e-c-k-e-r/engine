#define BLEND 1
#define DEPTH_TEST 0
#define CUBEMAPS 1
layout (constant_id = 0) const uint TEXTURES = 512;
layout (constant_id = 1) const uint CASCADES = 16;

#define MAX_TEXTURES textures.length()
#include "../common/macros.h"
#include "../common/structs.h"

layout (std140, binding = 0) readonly buffer Materials {
	Material materials[];
};
layout (std140, binding = 1) readonly buffer Textures {
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

layout (binding = 7) uniform sampler2D samplerTextures[TEXTURES];

layout (binding = 8, rg16ui) uniform volatile coherent uimage3D voxelId[CASCADES];
layout (binding = 9, rg16f) uniform volatile coherent image3D voxelNormal[CASCADES];
#if VXGI_HDR
	layout (binding = 10, rgba16f) uniform volatile coherent image3D voxelRadiance[CASCADES];
#else
	layout (binding = 10, rgba8) uniform volatile coherent image3D voxelRadiance[CASCADES];
#endif
#if DEPTH_TEST
	layout (binding = 11, r16f) uniform volatile coherent image3D voxelDepth[CASCADES];
#endif

#if 0
layout (location = 0) out uvec2 outId;
layout (location = 1) out vec2 outNormals;
#if DEFERRED_SAMPLING
	layout (location = 2) out vec2 outUvs;
#else
	layout (location = 2) out vec4 outAlbedo;
#endif
#endif

void main() {
	const uint CASCADE = inId.z;
	if ( CASCADES <= CASCADE ) discard;
	const vec3 P = inPosition.xzy * 0.5 + 0.5;
	if ( abs(P.x) > 1 || abs(P.y) > 1 || abs(P.z) > 1 ) discard;

	vec4 A = vec4(0, 0, 0, 0);
	const vec3 N = inNormal;
	const vec2 uv = wrap(inUv.xy);
	const float mip = 0; // mipLevel(inUv.xy);
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
		const float gamma = LIGHTMAP_GAMMA;
		const vec4 L = pow(textureLod( samplerTextures[nonuniformEXT(t.index)], inSt, mip ), vec4(1.0 / gamma));
		A *= L;
	#endif
	}
#endif

	const ivec3 uvw = ivec3(P * imageSize(voxelRadiance[CASCADE]));
	
#if DEPTH_TEST
	const float outDepth = length(inPosition.xzy);
	const float inDepth = imageLoad(voxelDepth[CASCADE], uvw).r;
	if ( inDepth != 0 && inDepth < outDepth ) discard;
	imageStore(voxelDepth[CASCADE], uvw, vec4(inDepth, 0, 0, 0));
#endif

	imageStore(voxelId[CASCADE], uvw, uvec4(uvec2(inId.w+1, inId.y+1), 0, 0));
	imageStore(voxelNormal[CASCADE], uvw, vec4(encodeNormals( normalize( N ) ), 0, 0));
#if BLEND
	// GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA
	const vec4 src = A * inColor;
	const vec4 dst = imageLoad(voxelRadiance[CASCADE], uvw);
	imageStore(voxelRadiance[CASCADE], uvw, blend( src, dst, src.a ) );
#else
	imageStore(voxelRadiance[CASCADE], uvw, A * inColor );
#endif
}