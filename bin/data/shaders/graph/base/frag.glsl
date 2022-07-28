#version 450
#pragma shader_stage(fragment)

//#extension GL_EXT_nonuniform_qualifier : enable

#define STABILIZE_BARY 1
#define QUERY_MIPMAP 0
#define EXTRA_ATTRIBUTES 1
#define CUBEMAPS 1
#define FRAGMENT 1
#define CAN_DISCARD 1
#define BARYCENTRIC 0
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
#if EXTRA_ATTRIBUTES
	layout (location = 1) in vec3 inPosition;
	layout (location = 2) in vec2 inUv;
	layout (location = 3) in vec4 inColor;
	layout (location = 4) in vec2 inSt;
	layout (location = 5) in vec3 inNormal;
	layout (location = 6) in vec3 inTangent;
#endif

layout (location = 0) out uvec2 outId;

void main() {
	const uint triangleID = gl_PrimitiveID; // uint(inId.x);
	const uint drawID = uint(inId.y);
	const uint instanceID = uint(inId.z);

#if EXTRA_ATTRIBUTES && CAN_DISCARD
	const DrawCommand drawCommand = drawCommands[drawID];
	const Instance instance = instances[instanceID];
	const Material material = materials[instance.materialID];

	surface.uv.xy = wrap(inUv.xy);
	surface.uv.z = mipLevel(dFdx(inUv), dFdy(inUv));

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
#endif

//	outId = uvec2(triangleID + 1, instanceID + 1);
	outId = uvec2(triangleID + 1, instanceID + 1);
}