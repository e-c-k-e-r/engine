#version 450
#pragma shader_stage(fragment)

#define FRAGMENT 1
#define CUBEMAPS 1
// #define MAX_TEXTURES TEXTURES
#define MAX_TEXTURES textures.length()
layout (constant_id = 0) const uint TEXTURES = 1;

#include "../common/macros.h"
#include "../common/structs.h"

layout (binding = 5) uniform sampler2D samplerTextures[TEXTURES];
layout (std140, binding = 6) readonly buffer Instances {
	Instance instances[];
};
layout (std140, binding = 7) readonly buffer Materials {
	Material materials[];
};
layout (std140, binding = 8) readonly buffer Textures {
	Texture textures[];
};

#include "../common/functions.h"

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec2 inSt;
layout (location = 2) in vec4 inColor;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in mat3 inTBN;
layout (location = 7) in vec3 inPosition;
layout (location = 8) flat in uvec4 inId;

void main() {
	const uint drawID = uint(inId.x);
	const uint instanceID = uint(inId.y);
	const uint materialID = uint(inId.z);
	const vec2 uv = wrap(inUv.xy);
	
	const Material material = materials[materialID];
	vec4 A = material.colorBase;
	surface.uv.xy = uv;
	surface.uv.z = mipLevel(dFdx(inUv), dFdy(inUv));

	// sample albedo
//	if ( !validTextureIndex( material.indexAlbedo ) ) discard; {
	if ( validTextureIndex( material.indexAlbedo ) ) {
	//	const Texture t = textures[material.indexAlbedo];
	//	A = textureLod( samplerTextures[nonuniformEXT(t.index)], mix( t.lerp.xy, t.lerp.zw, uv ), mip );
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
	//	if ( A.a < 1 - 0.00001 ) discard;
	}
}