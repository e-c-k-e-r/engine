//#extension GL_EXT_nonuniform_qualifier : enable
#define CUBEMAPS 1
#define FRAGMENT 1
// #define MAX_TEXTURES TEXTURES
#define MAX_TEXTURES textures.length()
layout (constant_id = 0) const uint TEXTURES = 1;

#include "../common/macros.h"
#include "../common/structs.h"

layout (binding = 5) uniform sampler2D samplerTextures[TEXTURES];
layout (std140, binding = 6) readonly buffer DrawCommands {
	DrawCommand drawCommands[];
};
layout (std140, binding = 7) readonly buffer Instances {
	Instance instances[];
};
layout (std140, binding = 8) readonly buffer InstanceAddresseses {
	InstanceAddresses instanceAddresses[];
};
layout (std140, binding = 9) readonly buffer Materials {
	Material materials[];
};
layout (std140, binding = 10) readonly buffer Textures {
	Texture textures[];
};
layout (std140, binding = 11) readonly buffer Lights {
	Light lights[];
};

#include "../common/functions.h"

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec2 inSt;
layout (location = 2) in vec4 inColor;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in mat3 inTBN;
layout (location = 7) in vec3 inPosition;
layout (location = 8) flat in uvec4 inId;

layout (location = 0) out uvec2 outId;
layout (location = 1) out vec2 outNormals;
#if DEFERRED_SAMPLING
	layout (location = 2) out vec4 outUvs;
	layout (location = 3) out vec2 outMips;
#else
	layout (location = 2) out vec4 outAlbedo;
#endif

void main() {
	const uint drawID = uint(inId.x);
	const uint instanceID = uint(inId.y);
	const uint materialID = uint(inId.z);
	const vec2 uv = wrap(inUv.xy);
	const vec3 P = inPosition;

	surface.uv.xy = uv;
	surface.uv.z = mipLevel(dFdx(inUv), dFdy(inUv));
	surface.st.xy = inSt;
	surface.st.z = mipLevel(dFdx(inSt), dFdy(inSt));
	vec3 N = inNormal;
	vec4 A = vec4(0, 0, 0, 0);

#if DEFERRED_SAMPLING
	vec4 outAlbedo = vec4(0,0,0,0);
#endif
#if !DEFERRED_SAMPLING || CAN_DISCARD
	const Instance instance = instances[instanceID];
	const Material material = materials[materialID];

	A = material.colorBase;
	float M = material.factorMetallic;
	float R = material.factorRoughness;
	float AO = material.factorOcclusion;
	
	// sample albedo
	if ( validTextureIndex( material.indexAlbedo ) ) {
		A = sampleTexture( material.indexAlbedo );
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
#if USE_LIGHTMAP && !DEFERRED_SAMPLING
	if ( validTextureIndex( instance.lightmapID ) ) {
		vec4 light = sampleTexture( instance.lightmapID, surface.st );
		if ( light.a > 0.001 ) A.rgb *= light.rgb;
	}
#endif
#endif
	// sample normal
	if ( validTextureIndex( material.indexNormal ) ) {
		N = inTBN * normalize( sampleTexture( material.indexNormal ).xyz * 2.0 - vec3(1.0));
	}
#if !DEFERRED_SAMPLING
	#if 0
		// sample metallic/roughness
		if ( validTextureIndex( material.indexMetallicRoughness ) ) {
			Texture t = textures[material.indexMetallicRoughness];
			const vec4 sampled = textureLod( samplerTextures[nonuniformEXT((useAtlas)?textureAtlas.index:t.index)], ( useAtlas ) ? mix( t.lerp.xy, t.lerp.zw, uv ) : uv, mipUv );
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
	outUvs.xy = surface.uv.xy;
	outUvs.zw = surface.st.xy;

	outMips.x = surface.uv.z;
	outMips.y = surface.st.z;
#endif
	outNormals = encodeNormals( N );
	outId = uvec2(drawID + 1, instanceID + 1);
}