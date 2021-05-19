#version 450
#pragma shader_stage(fragment)
#extension GL_EXT_samplerless_texture_functions : require

#define TEXTURES 1
#define CUBEMAPS 1
#define DEFERRED_SAMPLING 0
#define MULTISAMPLING 0

#include "../common/macros.h"
#include "../common/structs.h"
#include "../common/functions.h"

#if !MULTISAMPLING
	layout (binding = 1) uniform usampler2D textureId;
	layout (binding = 2) uniform sampler2D textureNormals;
	#if DEFERRED_SAMPLING
		layout (binding = 3) uniform sampler2D textureUvs;
	#else
		layout (binding = 3) uniform sampler2D  textureAlbedo;
 	#endif
#else
	layout (binding = 1) uniform usampler2DMS textureId;
	layout (binding = 2) uniform sampler2DMS textureNormals;
	#if DEFERRED_SAMPLING
		layout (binding = 3) uniform sampler2DMS textureUvs;
	#else
		layout (binding = 3) uniform sampler2DMS  textureAlbedo;
 	#endif
#endif

layout (location = 0) in vec2 inUv;
layout (location = 1) in float inAlpha;
layout (location = 2) in Cursor inCursor;

layout (location = 0) out vec4 outAlbedo;

void main() {
#if !MULTISAMPLING
	outAlbedo = texture( textureAlbedo, inUv );
#else
	outAlbedo = resolve( textureAlbedo, ivec2(inUv * textureSize(textureId)) );
#endif
	if ( inCursor.radius.x <= 0 || inCursor.radius.y <= 0 ) return;
	float dist = pow(inUv.x - inCursor.position.x, 2) / pow(inCursor.radius.x, 2) + pow(inUv.y - inCursor.position.y, 2) / pow(inCursor.radius.y, 2);

	if ( dist <= 1 ) {
		float attenuation = dist;
		outAlbedo.rgb = mix( inCursor.color.rgb * inCursor.color.a, outAlbedo.rgb, attenuation );
	}
	if ( inAlpha < 1.0 ) outAlbedo.a = inAlpha;
}