#version 450
#pragma shader_stage(fragment)

//#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_fragment_shader_barycentric : enable
#extension GL_AMD_shader_explicit_vertex_parameter : enable

#define STABILIZE_BARY 1
#define DEFERRED_SAMPLING 1
#define QUERY_MIPMAP 0
#define EXTRA_ATTRIBUTES 1
#define CUBEMAPS 1
#define FRAGMENT 1
#define CAN_DISCARD 1
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

layout (location = 0) flat in uvec4 inId;
layout (location = 1) flat in vec4 inPOS0;
layout (location = 2) __explicitInterpAMD in vec4 inPOS1;
#if EXTRA_ATTRIBUTES
	layout (location = 3) in vec3 inPosition;
	layout (location = 4) in vec2 inUv;
	layout (location = 5) in vec4 inColor;
	layout (location = 6) in vec2 inSt;
	layout (location = 7) in vec3 inNormal;
	layout (location = 8) in vec3 inTangent;
#endif

layout (location = 0) out uvec2 outId;
layout (location = 1) out vec2 outBary;
#if 0 && EXTRA_ATTRIBUTES
	layout (location = 2) out vec2 outMips;
	layout (location = 3) out vec2 outNormals;
	layout (location = 4) out vec4 outUvs;
#endif


void main() {
	const uint drawID = uint(inId.x);
	const uint triangleID = uint(inId.y);
	const uint instanceID = uint(inId.z);

	const DrawCommand drawCommand = drawCommands[drawID];
	const Instance instance = instances[instanceID];
	const Material material = materials[instance.materialID];

#if EXTRA_ATTRIBUTES
	#if CAN_DISCARD
	{
		surface.uv.xy = wrap(inUv.xy);
		surface.uv.z = mipLevel(dFdx(inUv), dFdy(inUv));

		vec4 A = inColor * material.colorBase;	
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
			if ( A.a < 1 ) discard;
		}
	}
	#endif
	#if 0
	{
		vec3 T = inTangent;
		vec3 N = inNormal;
		T = normalize(T - dot(T, N) * N);
		vec3 B = cross(T, N);
		mat3 TBN = mat3(T, B, N);
	//	mat3 TBN = mat3(N, B, T);
		// sample normal
		if ( T != vec3(0) && validTextureIndex( material.indexNormal ) ) {
			N = TBN * normalize( sampleTexture( material.indexNormal ).xyz * 2.0 - 1.0 );
		}
		outUvs.xy = surface.uv.xy;
		outUvs.zw = surface.st.xy;
		outMips.x = surface.uv.z;
		outMips.y = surface.st.z;
		outNormals = encodeNormals( normalize( N ) );
	}
	#endif
#endif

	outId = uvec2(drawID + 1, triangleID + 1);

#if STABILIZE_BARY
	vec4 v0 = interpolateAtVertexAMD(inPOS1, 0);
	vec4 v1 = interpolateAtVertexAMD(inPOS1, 1);
	vec4 v2 = interpolateAtVertexAMD(inPOS1, 2);
#if 0
	vec3 bary;
	if (v0 == inPOS0) {
		bary.y = gl_BaryCoordSmoothAMD.x;
		bary.z = gl_BaryCoordSmoothAMD.y;
		bary.x = 1 - bary.y - bary.z;
	}
	else if (v1 == inPOS0) {
		bary.x = gl_BaryCoordSmoothAMD.x;
		bary.y = gl_BaryCoordSmoothAMD.y;
		bary.z = 1 - bary.y - bary.x;
	}
	else if (v2 == inPOS0) {
		bary.z = gl_BaryCoordSmoothAMD.x;
		bary.x = gl_BaryCoordSmoothAMD.y;
		bary.y = 1 - bary.z - bary.x;
	}
	outBary = bary.yz;
#else
	if (v0 == inPOS0) 
		outBary = (gl_BaryCoordEXT.yzx).yz;
	else if (v1 == inPOS0) 
		outBary = (gl_BaryCoordEXT.zxy).yz;
	else if (v2 == inPOS0) 
		outBary = (gl_BaryCoordEXT.xyz).yz;
#endif
#else
	outBary = gl_BaryCoordEXT.yz;
#endif
}