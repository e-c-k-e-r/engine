#include "../common/macros.h"

#define TEXTURES 1
#define CUBEMAPS 1

#if !MULTISAMPLING
	layout (binding = 1) uniform usampler2D samplerId;
	layout (binding = 2) uniform sampler2D samplerNormals;
	#if DEFERRED_SAMPLING
		layout (binding = 3) uniform sampler2D samplerUvs;
	#else
		layout (binding = 4) uniform sampler2D  samplerAlbedo;
 	#endif
#else
	layout (binding = 1) uniform usampler2DMS samplerId;
	layout (binding = 2) uniform sampler2DMS samplerNormals;
	#if DEFERRED_SAMPLING
		layout (binding = 3) uniform sampler2DMS samplerUvs;
	#else
		layout (binding = 4) uniform sampler2DMS samplerAlbedo;
 	#endif
#endif

#include "../common/structs.h"
#include "../common/functions.h"

layout (location = 0) in vec2 inUv;
layout (location = 1) in float inAlpha;
layout (location = 2) in Cursor inCursor;

layout (location = 0) out vec4 outAlbedo;

void main() {
#if DEFERRED_SAMPLING
#else
	#if !MULTISAMPLING
		outAlbedo = texture( samplerAlbedo, inUv );
	#else
		outAlbedo = resolve( samplerAlbedo, ivec2(inUv * textureSize(samplerId)) );
	#endif
#endif
	if ( inCursor.radius.x <= 0 || inCursor.radius.y <= 0 ) return;
	float dist = pow(inUv.x - inCursor.position.x, 2) / pow(inCursor.radius.x, 2) + pow(inUv.y - inCursor.position.y, 2) / pow(inCursor.radius.y, 2);

	if ( dist <= 1 ) {
		float attenuation = dist;
		outAlbedo.rgb = mix( inCursor.color.rgb * inCursor.color.a, outAlbedo.rgb, attenuation );
	}
	if ( inAlpha < 1.0 ) outAlbedo.a = inAlpha;
}