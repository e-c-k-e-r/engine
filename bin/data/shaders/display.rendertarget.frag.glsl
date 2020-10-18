#version 450
#extension GL_EXT_samplerless_texture_functions : require

layout (binding = 1) uniform sampler samp;
layout (binding = 2) uniform texture2D albedoTexture;
layout (binding = 3) uniform texture2D normalTexture;
layout (binding = 4) uniform texture2D positionTexture;

struct Cursor {
	vec2 position;
	vec2 radius;
	vec4 color;
};

layout (location = 0) in vec2 inUv;
layout (location = 1) in float inAlpha;
layout (location = 2) in Cursor inCursor;

layout (location = 0) out vec4 outAlbedoSpecular;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outPosition;

void main() {
	if ( inCursor.radius.x <= 0 && inCursor.radius.y <= 0 ) {
		vec2 uv = gl_FragCoord.xy / textureSize(albedoTexture, 0);
		outAlbedoSpecular = texture(sampler2D(albedoTexture, samp), uv);
		outNormal = texture(sampler2D(normalTexture, samp), uv);
		outPosition = texture(sampler2D(positionTexture, samp), uv);
	//	if ( outAlbedoSpecular.a < 0.01f ) outAlbedoSpecular = vec4(0,0,0,1);
		return;
	}

	outAlbedoSpecular = texture(sampler2D(albedoTexture, samp), inUv);
	float dist = pow(inUv.x - inCursor.position.x, 2) / pow(inCursor.radius.x, 2) + pow(inUv.y - inCursor.position.y, 2) / pow(inCursor.radius.y, 2);
	if ( dist <= 1 ) {
		float attenuation = dist;
		outAlbedoSpecular.rgb = mix( inCursor.color.rgb * inCursor.color.a, outAlbedoSpecular.rgb, attenuation );
	}
	if ( inAlpha < 1.0 ) outAlbedoSpecular.a = inAlpha;
}