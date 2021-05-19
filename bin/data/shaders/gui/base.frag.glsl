#version 450
#pragma shader_stage(fragment)

#define TEXTURES 1
#define CUBEMAPS 1
#define DEFERRED_SAMPLING 0

#include "../common/macros.h"
#include "../common/structs.h"
#include "../common/functions.h"

layout (binding = 1) uniform sampler2D samplerTexture;

struct Gui {
	vec4 offset;
	vec4 color;
	int mode;
	float depth;
	vec2 padding;
};

layout (location = 0) in vec2 inUv;
layout (location = 1) in flat Gui inGui;

layout (location = 0) out uvec2 outId;
layout (location = 1) out vec2 outNormals;
#if DEFERRED_SAMPLING
	layout (location = 2) out vec2 outUvs;
#else
	layout (location = 2) out vec4 outAlbedo;
#endif

void main() {
	if ( inUv.x < inGui.offset.x ) discard;
	if ( inUv.y < inGui.offset.y ) discard;
	if ( inUv.x > inGui.offset.z ) discard;
	if ( inUv.y > inGui.offset.w ) discard;

	float mip = mipLevel(inUv.xy);
	vec2 uv = inUv.xy;
	vec4 C = vec4(1, 1, 1, 1);
	//vec3 N = inNormal;
#if DEFERRED_SAMPLING
	outUvs = wrap(inUv.xy);
	vec4 outAlbedo = vec4(0,0,0,0);
#endif
#if !DEFERRED_SAMPLING || CAN_DISCARD
	C = textureLod( samplerTexture, uv, mip );
#endif
#if !DEFERRED_SAMPLING
	if ( inGui.mode == 1 ) {
		C = inGui.color;
	} else {
		C *= inGui.color;
	}
	outAlbedo = C;
#endif
/*
	outNormals = encodeNormals( N );
	outId = ivec2(0, 0);
*/
}