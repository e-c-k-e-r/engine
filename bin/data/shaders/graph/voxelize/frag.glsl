#version 450
#pragma shader_stage(fragment)

// to-do: convert to use functions.h surface population functions

#define FRAGMENT 1
#define DEFERRED_SAMPLING 0
#define CUBEMAPS 1

#define BLEND 0
#define USE_LIGHTMAP 1
layout (constant_id = 0) const uint TEXTURES = 512;
layout (constant_id = 1) const uint CASCADES = 16;

#define MAX_TEXTURES textures.length()
#include "../../common/macros.h"
#include "../../common/structs.h"

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

layout (binding = 12, r32ui) uniform volatile coherent uimage3D voxelDrawId[CASCADES];
layout (binding = 13, r32ui) uniform volatile coherent uimage3D voxelInstanceId[CASCADES];
layout (binding = 14, r32ui) uniform volatile coherent uimage3D voxelNormalX[CASCADES];
layout (binding = 15, r32ui) uniform volatile coherent uimage3D voxelNormalY[CASCADES];
layout (binding = 16, r32ui) uniform volatile coherent uimage3D voxelRadianceR[CASCADES];
layout (binding = 17, r32ui) uniform volatile coherent uimage3D voxelRadianceG[CASCADES];
layout (binding = 18, r32ui) uniform volatile coherent uimage3D voxelRadianceB[CASCADES];
layout (binding = 19, r32ui) uniform volatile coherent uimage3D voxelRadianceA[CASCADES];
layout (binding = 20, r32ui) uniform volatile coherent uimage3D voxelCount[CASCADES];
layout (binding = 21, rgba16f) uniform volatile coherent image3D voxelOutput[CASCADES];

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
	const uint CASCADE = inId.w;
	if ( CASCADES <= CASCADE ) discard;
	const vec3 P = inPosition.xzy * 0.5 + 0.5;
	if ( abs(P.x) > 1 || abs(P.y) > 1 || abs(P.z) > 1 ) discard;

	const uint triangleID = uint(inId.x); // gl_PrimitiveID
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
	float M = material.factorMetallic;
	float R = material.factorRoughness;
	float AO = material.factorOcclusion;
	
	// sample albedo
	if ( validTextureIndex( material.indexAlbedo ) ) {
		A = sampleTexture( material.indexAlbedo );
	}
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

#if USE_LIGHTMAP
	if ( validTextureIndex( instance.lightmapID ) ) {
		A.rgb *= sampleTexture( instance.lightmapID, inSt ).rgb;
	}
#endif

	// sample normal
	vec3 N = inNormal;
	vec3 T = inTangent;
	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(T, N);
	mat3 TBN = mat3(T, B, N);
//	mat3 TBN = mat3(N, B, T);
	if ( T != vec3(0) && validTextureIndex( material.indexNormal ) ) {
		N = TBN * normalize( sampleTexture( material.indexNormal ).xyz * 2.0 - 1.0 );
	}

	const ivec3 uvw = ivec3(P * imageSize(voxelOutput[CASCADE]));

	imageAtomicMax(voxelDrawId[CASCADE], ivec3(uvw), uint( drawID + 1 ) );
	imageAtomicMax(voxelInstanceId[CASCADE], ivec3(uvw), uint( instanceID + 1 ) );

	vec2 N_E = encodeNormals( normalize( N ) );
	imageAtomicMin(voxelNormalX[CASCADE], ivec3(uvw), uint( floatBitsToUint( N_E.x ) ) );
	imageAtomicMin(voxelNormalY[CASCADE], ivec3(uvw), uint( floatBitsToUint( N_E.y ) ) );

	imageAtomicAdd(voxelRadianceR[CASCADE], ivec3(uvw), uint( A.r * 256 ) );
	imageAtomicAdd(voxelRadianceG[CASCADE], ivec3(uvw), uint( A.g * 256 ) );
	imageAtomicAdd(voxelRadianceB[CASCADE], ivec3(uvw), uint( A.b * 256 ) );
	imageAtomicAdd(voxelRadianceA[CASCADE], ivec3(uvw), uint( A.a * 256 ) );
	imageAtomicAdd(voxelCount[CASCADE], ivec3(uvw), uint( 1 ) );

	imageStore(voxelOutput[CASCADE], uvw, vec4(N.x, N.y, N.z, A.a) );
}