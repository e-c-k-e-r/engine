//#extension GL_EXT_nonuniform_qualifier : enable
// #extension GL_EXT_fragment_shader_barycentric : enable
#extension GL_AMD_shader_explicit_vertex_parameter : enable

#define CUBEMAPS 1
#define FRAGMENT 1
// #define MAX_TEXTURES TEXTURES
#define MAX_TEXTURES textures.length()
layout (constant_id = 0) const uint TEXTURES = 1;

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

#include "../../common/functions.h"

layout (location = 0) in vec3 inPosition;
layout (location = 1) flat in vec4 inPOS0;
layout (location = 2) __explicitInterpAMD in vec4 inPOS1;
layout (location = 3) in vec2 inUv;
layout (location = 4) in vec2 inSt;
layout (location = 5) in vec4 inColor;
layout (location = 6) in vec3 inNormal;
layout (location = 7) in vec3 inTangent;
layout (location = 8) flat in uvec4 inId;

layout (location = 0) out uvec4 outId;
layout (location = 1) out vec4 outBary;
layout (location = 2) out vec2 outMips;
layout (location = 3) out vec2 outNormals;
layout (location = 4) out vec4 outUvs;


void main() {
	const uint drawID = uint(inId.x);
	const uint triangleID = uint(inId.y);
	const uint instanceID = uint(inId.z);
	const vec2 uv = wrap(inUv.xy);
	const vec3 P = inPosition;

	surface.uv.xy = uv;
	surface.uv.z = mipLevel(dFdx(inUv), dFdy(inUv));
	surface.st.xy = inSt;
	surface.st.z = mipLevel(dFdx(inSt), dFdy(inSt));
	vec3 T = inTangent;
	vec3 N = inNormal;
	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(T, N);
//	mat3 TBN = mat3(T, B, N);
	mat3 TBN = mat3(N, B, T);
	vec4 A = vec4(0, 0, 0, 0);

#if CAN_DISCARD
	const DrawCommand drawCommand = drawCommands[drawID];
	const Instance instance = instances[instanceID];
	const Material material = materials[instance.materialID];

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
	if ( T != vec3(0) && validTextureIndex( material.indexNormal ) ) {
		N = TBN * normalize( sampleTexture( material.indexNormal ).xyz * 2.0 - 1.0 );
	}
	outUvs.xy = surface.uv.xy;
	outUvs.zw = surface.st.xy;

	outMips.x = surface.uv.z;
	outMips.y = surface.st.z;

	outNormals = encodeNormals( normalize( N ) );
	outId = uvec4(drawID + 1, triangleID + 1, instanceID + 1, 0);

#if 1
	vec4 v0 = interpolateAtVertexAMD(inPOS1, 0);
	vec4 v1 = interpolateAtVertexAMD(inPOS1, 1);
	vec4 v2 = interpolateAtVertexAMD(inPOS1, 2);
	if (v0 == inPOS0) {
		outBary.y = gl_BaryCoordSmoothAMD.x;
		outBary.z = gl_BaryCoordSmoothAMD.y;
		outBary.x = 1 - outBary.z - outBary.y;
	}
	else if (v1 == inPOS0) {
		outBary.x = gl_BaryCoordSmoothAMD.x;
		outBary.y = gl_BaryCoordSmoothAMD.y;
		outBary.z = 1 - outBary.x - outBary.y;
	}
	else if (v2 == inPOS0) {
		outBary.z = gl_BaryCoordSmoothAMD.x;
		outBary.x = gl_BaryCoordSmoothAMD.y;
		outBary.y = 1 - outBary.x - outBary.z;
	}
#else
//	outBary = gl_BaryCoordEXT.xyz;
	outBary.x = 1 - gl_BaryCoordSmoothAMD.x - gl_BaryCoordSmoothAMD.y;
	outBary.y = gl_BaryCoordSmoothAMD.x;
	outBary.z = gl_BaryCoordSmoothAMD.y;
#endif
}