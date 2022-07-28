#version 450
#pragma shader_stage(fragment)
//#extension GL_EXT_nonuniform_qualifier : enable

#define DEFERRED_SAMPLING 0
#define FRAGMENT 1
#define BLEND 1
#define DEPTH_TEST 0
#define CUBEMAPS 1
#define TEXTURE_WORKAROUND 1
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

layout (binding = 12, rg16ui) uniform volatile coherent uimage3D voxelId[CASCADES];
layout (binding = 13, rg16f) uniform volatile coherent image3D voxelNormal[CASCADES];
#if VXGI_HDR
	layout (binding = 14, rgba16f) uniform volatile coherent image3D voxelRadiance[CASCADES];
#else
	layout (binding = 14, rgba8) uniform volatile coherent image3D voxelRadiance[CASCADES];
#endif
#if DEPTH_TEST
	layout (binding = 15, r16f) uniform volatile coherent image3D voxelDepth[CASCADES];
#endif

layout (location = 0) flat in uvec4 inId;
layout (location = 1) in vec3 inPosition;
layout (location = 2) in vec2 inUv;
layout (location = 3) in vec4 inColor;
layout (location = 4) in vec2 inSt;
layout (location = 5) in vec3 inNormal;
layout (location = 6) in vec3 inTangent;

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

#if USE_LIGHTMAP && !DEFERRED_SAMPLING
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

	const ivec3 uvw = ivec3(P * imageSize(voxelRadiance[CASCADE]));
	
#if DEPTH_TEST
	const float outDepth = length(inPosition.xzy);
	const float inDepth = imageLoad(voxelDepth[CASCADE], uvw).r;
	if ( inDepth != 0 && inDepth < outDepth ) discard;
	imageStore(voxelDepth[CASCADE], uvw, vec4(inDepth, 0, 0, 0));
#endif

	imageStore(voxelId[CASCADE], uvw, uvec4(uvec2(drawID + 1, instanceID + 1), 0, 0));
	imageStore(voxelNormal[CASCADE], uvw, vec4(encodeNormals( normalize( N ) ), 0, 0));
#if BLEND
	// GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA
	const vec4 src = A * inColor;
	const vec4 dst = imageLoad(voxelRadiance[CASCADE], uvw);
	imageStore(voxelRadiance[CASCADE], uvw, blend( src, dst, src.a ) );
#else
	imageStore(voxelRadiance[CASCADE], uvw, A );
#endif
}
