#version 450
#pragma shader_stage(fragment)

#extension GL_EXT_nonuniform_qualifier : require

#define FRAGMENT 1
#define DEFERRED_SAMPLING 0
#define CUBEMAPS 1

#define BLEND 0
#define USE_LIGHTMAP 1
layout (constant_id = 0) const uint TEXTURES = 512;
layout (constant_id = 1) const uint REGIONS = 16;

#define MAX_TEXTURES textures.length()
#include "../../common/macros.h"
#include "../../common/structs.h"

layout (binding = 7) uniform sampler2D samplerTextures[TEXTURES];
layout (std140, binding = 8) readonly buffer DrawCommands {
	DrawCommand drawCommands[];
};
layout (std140, binding = 9) readonly buffer Instances {
	Instance instances[];
};
layout (std140, binding = 10) readonly buffer InstanceAddresseses {
	InstanceAddresses addresses[];
};
layout (std140, binding = 11) readonly buffer Materials {
	Material materials[];
};
layout (std140, binding = 12) readonly buffer Textures {
	Texture textures[];
};
layout (std140, binding = 13) readonly buffer Lights {
	Light lights[];
};
layout (std140, binding = 14) readonly buffer RegionsBuffer {
	Region regions[];
};

layout (binding = 15, r32ui) uniform volatile uimage3D voxelId[REGIONS];
layout (binding = 16, r32ui) uniform volatile uimage3D voxelNormal[REGIONS];
layout (binding = 17, r32ui) uniform volatile uimage3D voxelRadiance[REGIONS];
layout (binding = 18, rgba8) uniform writeonly image3D voxelOutput[REGIONS];

layout (location = 0) flat in uvec4 inId;
layout (location = 1) flat in vec4 inPOS0;
layout (location = 2) in vec4 inPOS1;
layout (location = 3) in vec3 inPosition;
layout (location = 4) in vec2 inUv;
layout (location = 5) in vec4 inColor;
layout (location = 6) in vec2 inSt;
layout (location = 7) in vec3 inNormal;
layout (location = 8) in vec3 inTangent;

#include "../../common/functions.h"

void main() {
	const uint REGION_INDEX = inId.w;
	if ( REGIONS <= REGION_INDEX ) discard;

	Region region = regions[REGION_INDEX];

	const vec3 P = (inPosition.xyz - region.minBounds) / (region.maxBounds - region.minBounds);
	const float epsilon = 0.001;
	if ( any(lessThan(P, vec3(-epsilon))) || any(greaterThan(P, vec3(1.0 + epsilon))) ) discard;

	const uint triangleID = uint(inId.x);
	const uint drawID = uint(inId.y);
	const uint instanceID = uint(inId.z);

	surface.uv.xy = wrap(inUv.xy);
	surface.uv.z = mipLevel(dFdx(inUv), dFdy(inUv));
	surface.st.xy = inSt;
	surface.st.z = mipLevel(dFdx(inSt), dFdy(inSt));

	const DrawCommand drawCommand = drawCommands[drawID];
	const Instance instance = instances[instanceID];
	const Material material = materials[instance.materialID];
	surface.instance = instance;

	vec4 A = material.colorBase;

	if ( validTextureIndex( material.indexAlbedo ) ) {
		A = sampleTexture( material.indexAlbedo );
	}
	if ( material.modeAlpha == 0 ) {
		A.a = 1;
	} else if ( material.modeAlpha == 2 ) {
		if ( A.a < abs(material.factorAlphaCutoff) ) discard;
		A.a = 1;
	}
	if ( A.a == 0 ) discard;

#if USE_LIGHTMAP
	if ( validTextureIndex( instance.lightmapID ) ) {
		A.rgb *= sampleTexture( instance.lightmapID, inSt ).rgb;
	}
#endif

	vec3 N = inNormal;
	vec3 T = inTangent;
	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(T, N);
	mat3 TBN = mat3(T, B, N);
	if ( T != vec3(0) && validTextureIndex( material.indexNormal ) ) {
		N = TBN * normalize( sampleTexture( material.indexNormal ).xyz * 2.0 - 1.0 );
	}

	const ivec3 uvw = ivec3(P * imageSize(voxelOutput[nonuniformEXT(REGION_INDEX)]));

	{
		uint packedId = ( instanceID + 1 ) << 16 | ( drawID + 1 );
		imageAtomicMax(voxelId[nonuniformEXT(REGION_INDEX)], uvw, packedId);
	}
	{
		vec2 N_E = encodeNormals( normalize( N ) );
		uint packedNormal = packHalf2x16(N_E);
		imageAtomicMax(voxelNormal[nonuniformEXT(REGION_INDEX)], uvw, packedNormal);
	}
	{
		uint l = uint(clamp(luma(A.rgb), 0.0, 1.0) * 15.0) & 0xF;
		uint a = uint(clamp(		A.a, 0.0, 1.0) * 15.0) & 0xF;
		float packedLumaAlpha = float((l << 4) | a) / 255.0;
		uint packedRadiance = packUnorm4x8(vec4(A.rgb, packedLumaAlpha));
		imageAtomicMax(voxelRadiance[nonuniformEXT(REGION_INDEX)], uvw, packedRadiance);
	}
}
