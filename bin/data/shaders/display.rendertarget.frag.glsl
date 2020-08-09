#version 450

layout (binding = 1) uniform sampler2D samplerTexture;

struct Cursor {
	vec2 position;
	vec2 radius;
	vec4 color;
};

layout (location = 0) in vec2 inUv;
layout (location = 1) in float inAlpha;
layout (location = 2) in Cursor inCursor;

layout (location = 0) out vec4 outFragColor;

void main() {
	outFragColor = texture(samplerTexture, inUv);

	float dist = pow(inUv.x - inCursor.position.x, 2) / pow(inCursor.radius.x, 2) + pow(inUv.y - inCursor.position.y, 2) / pow(inCursor.radius.y, 2);
	if ( dist <= 1 ) {
		float attenuation = dist;
		outFragColor.rgb = mix( inCursor.color.rgb * inCursor.color.a, outFragColor.rgb, attenuation );
	}
}