#version 450
#extension GL_EXT_samplerless_texture_functions : require

#define MULTISAMPLING 1
#define DEFERRED_SAMPLING 0
#define CAN_DISCARD 1

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

struct Cursor {
	vec2 position;
	vec2 radius;
	vec4 color;
};

vec4 resolve( sampler2DMS t, ivec2 uv ) {
	vec4 resolved = vec4(0);
	int samples = textureSamples(t);
	for ( int i = 0; i < samples; ++i ) {
		resolved += texelFetch(t, uv, i);
	}
	resolved /= float(samples);
	return resolved;
}

layout (location = 0) in vec2 inUv;
layout (location = 1) in float inAlpha;
layout (location = 2) in Cursor inCursor;

layout (location = 0) out vec4 outAlbedo;

void main() {
#if !MULTISAMPLING
	ivec2 screenSize = textureSize(textureId, 0);
#else
	ivec2 screenSize = textureSize(textureId);
#endif
	ivec2 uv = ivec2(inUv * screenSize);
	if ( inCursor.radius.x <= 0 || inCursor.radius.y <= 0 ) {
	#if !MULTISAMPLING
		outAlbedo = texture( textureAlbedo, uv ).rgba;
	#else
		outAlbedo = resolve( textureAlbedo, uv );
	#endif
		return;
	}
	float dist = pow(inUv.x - inCursor.position.x, 2) / pow(inCursor.radius.x, 2) + pow(inUv.y - inCursor.position.y, 2) / pow(inCursor.radius.y, 2);

#if !MULTISAMPLING
	outAlbedo = texture( textureAlbedo, inUv );
#else
	outAlbedo = resolve( textureAlbedo, uv );
#endif

	if ( dist <= 1 ) {
		float attenuation = dist;
		outAlbedo.rgb = mix( inCursor.color.rgb * inCursor.color.a, outAlbedo.rgb, attenuation );
	}
	if ( inAlpha < 1.0 ) outAlbedo.a = inAlpha;
}