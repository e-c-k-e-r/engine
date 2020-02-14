#version 450
#extension GL_EXT_samplerless_texture_functions : require

#define UF_DEFERRED_SAMPLING 0
#define UF_CAN_DISCARD 1

layout (binding = 1) uniform sampler samp;
layout (binding = 2) uniform utexture2D textureId;
layout (binding = 3) uniform texture2D textureNormals;
#if UF_DEFERRED_SAMPLING
	layout (binding = 4) uniform texture2D textureUvs;
#else
	layout (binding = 4) uniform texture2D textureAlbedo;
#endif

struct Cursor {
	vec2 position;
	vec2 radius;
	vec4 color;
};

layout (location = 0) in vec2 inUv;
layout (location = 1) in float inAlpha;
layout (location = 2) in Cursor inCursor;

layout (location = 0) out vec4 outAlbedo;
void main() {
	if ( inCursor.radius.x <= 0 && inCursor.radius.y <= 0 ) {
		vec2 uv = gl_FragCoord.xy / textureSize(textureId, 0);

		outAlbedo = texture(sampler2D(textureAlbedo, samp), uv).rgba;
		return;
	}
	float dist = pow(inUv.x - inCursor.position.x, 2) / pow(inCursor.radius.x, 2) + pow(inUv.y - inCursor.position.y, 2) / pow(inCursor.radius.y, 2);

	outAlbedo = texture(sampler2D(textureAlbedo, samp), inUv);
	if ( dist <= 1 ) {
		float attenuation = dist;
		outAlbedo.rgb = mix( inCursor.color.rgb * inCursor.color.a, outAlbedo.rgb, attenuation );
	}
	if ( inAlpha < 1.0 ) outAlbedo.a = inAlpha;
}

/*
layout (location = 0) out uvec2 outId;
layout (location = 1) out vec2 outNormals;
#if UF_DEFERRED_SAMPLING
	layout (location = 2) out vec2 outUvs;
#else
	layout (location = 2) out vec4 outAlbedo;
#endif
void main() {
	if ( inCursor.radius.x <= 0 && inCursor.radius.y <= 0 ) {
		vec2 uv = gl_FragCoord.xy / textureSize(textureId, 0);

		outId = texture(usampler2D(textureId, samp), uv).xy;
		outNormals = texture(sampler2D(textureNormals, samp), uv).xy;
		#if UF_DEFERRED_SAMPLING
			outUvs = texture(sampler2D(textureUvs, samp), uv).xy;
		#else
			outAlbedo = texture(sampler2D(textureAlbedo, samp), uv).rgba;
		#endif
		return;
	}
	float dist = pow(inUv.x - inCursor.position.x, 2) / pow(inCursor.radius.x, 2) + pow(inUv.y - inCursor.position.y, 2) / pow(inCursor.radius.y, 2);
#if !UF_DEFERRED_SAMPLING
	outAlbedo = texture(sampler2D(textureAlbedo, samp), inUv);
	if ( dist <= 1 ) {
		float attenuation = dist;
		outAlbedo.rgb = mix( inCursor.color.rgb * inCursor.color.a, outAlbedo.rgb, attenuation );
	}
	if ( inAlpha < 1.0 ) outAlbedo.a = inAlpha;
#endif
}
*/