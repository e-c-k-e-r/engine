#version 450
#pragma shader_stage(fragment)

//#extension GL_EXT_nonuniform_qualifier : enable

#define FRAGMENT 1
#define CAN_DISCARD 1

#define MAX_TEXTURES textures.length()

layout (constant_id = 0) const uint TEXTURES = 1;


#include "../../common/macros.h"
#include "../../common/structs.h"

#if BARYCENTRIC && !BARYCENTRIC_CALCULATE
	// AMD has a specific extension with a helpful intrinsic, but it's not necessary
	#if AMD
		#define BARYCENTRIC_STABILIZE 1
		#define BARY_COORD gl_BaryCoordSmoothAMD
		#extension GL_AMD_shader_explicit_vertex_parameter : enable
	#else
		#define BARYCENTRIC_STABILIZE 0
 		#define BARY_COORD gl_BaryCoordEXT
		#extension GL_EXT_fragment_shader_barycentric : enable
	#endif
#endif

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
#if BARYCENTRIC_STABILIZE
	layout (location = 2) __explicitInterpAMD in vec4 inPOS1;
#else
	layout (location = 2) in vec4 inPOS1;
#endif
layout (location = 3) in vec3 inPosition;
layout (location = 4) in vec2 inUv;
layout (location = 5) in vec4 inColor;
layout (location = 6) in vec2 inSt;
layout (location = 7) in vec3 inNormal;
layout (location = 8) in vec3 inTangent;

layout (location = 0) out uvec2 outId;
#if BARYCENTRIC
	layout (location = 1) out vec2 outBary;
#else
	layout (location = 1) out vec4 outUv;
	layout (location = 2) out vec4 outNormal;
#endif

void main() {
	const uint triangleID = gl_PrimitiveID; // uint(inId.x);
	const uint drawID = uint(inId.y);
	const uint instanceID = uint(inId.z);

#if CAN_DISCARD
	const DrawCommand drawCommand = drawCommands[drawID];
	const Instance instance = instances[instanceID];
	const Material material = materials[instance.materialID];

	surface.uv.xy = wrap(inUv.xy);
	surface.uv.z = mipLevel(dFdx(inUv), dFdy(inUv));
	
	surface.st.xy = wrap(inSt.xy);
	surface.st.z = mipLevel(dFdx(inSt), dFdy(inSt));

	vec4 A = inColor * material.colorBase;	
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
	if ( A.a < 0.0001 ) discard;

#if !BARYCENTRIC
	outUv = vec4(surface.uv.xy, surface.st.xy);
	outNormal = vec4( encodeNormals(inNormal), encodeNormals(inTangent) );
#endif

#endif

	outId = uvec2(triangleID + 1, instanceID + 1);

#if BARYCENTRIC && !BARYCENTRIC_CALCULATE
	vec3 bary = vec3(BARY_COORD.xy, 1 - BARY_COORD.x - BARY_COORD.y);
	// AMD's barycentric emits a different order for coords
	#if AMD
		bary = bary.yzx;
	#endif
	#if BARYCENTRIC_STABILIZE
		vec4 v0 = interpolateAtVertexAMD(inPOS1, 0);
		vec4 v1 = interpolateAtVertexAMD(inPOS1, 1);
		vec4 v2 = interpolateAtVertexAMD(inPOS1, 2);
		if (v0 == inPOS0) outBary = encodeBarycentrics(bary.yzx);
		else if (v1 == inPOS0) outBary = encodeBarycentrics(bary.zxy);
		else if (v2 == inPOS0) outBary = encodeBarycentrics(bary.xyz);
	#else
		outBary = encodeBarycentrics(bary.xyz);
	#endif
#endif
}