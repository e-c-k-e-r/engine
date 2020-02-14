#version 450

#define UF_DEFERRED_SAMPLING 0
#define UF_CAN_DISCARD 1

layout (binding = 1) uniform sampler2D samplerTexture;

struct Gui {
	vec4 offset;
	vec4 color;
	int mode;
	float depth;
	int sdf;
	int shadowbox;
	vec4 stroke;
	float weight;
	int spread;
	float scale;
	float padding;
};

layout (location = 0) in vec2 inUv;
layout (location = 1) in flat Gui inGui;

layout (location = 0) out uvec2 outId;
layout (location = 1) out vec2 outNormals;
#if UF_DEFERRED_SAMPLING
	layout (location = 2) out vec2 outUvs;
#else
	layout (location = 2) out vec4 outAlbedo;
#endif

void main() {
	if ( inUv.x < inGui.offset.x ) discard;
	if ( inUv.y < inGui.offset.y ) discard;
	if ( inUv.x > inGui.offset.z ) discard;
	if ( inUv.y > inGui.offset.w ) discard;

#if UF_DEFERRED_SAMPLING
	vec4 outAlbedo = vec4(0,0,0,0);
#endif

	if ( inGui.shadowbox == 1 ) {
		outAlbedo = inGui.color;
		return;
	}
	float dist = texture(samplerTexture, inUv).r;
	if ( inGui.sdf == 1 ) {
		float smoothing = ( inGui.spread > 0 && inGui.scale > 0 ) ? 0.25 / (inGui.spread * inGui.scale) : 0.25 / (4 * 1.5);
		float outlining = smoothstep(0.5 - smoothing, 0.5 + smoothing, dist);
		float alpha = smoothstep(inGui.weight - smoothing, inGui.weight + smoothing, dist);
		vec4 c = inGui.color;
		outAlbedo = mix(inGui.stroke, c, outlining);
		outAlbedo.a = inGui.color.a * alpha;
		if ( alpha < 0.001 ) discard;
		if ( alpha > 1 ) discard;
	} else {
		outAlbedo = vec4(inGui.color) * dist;
		if ( dist < 0.001 ) discard;
		if ( dist > 1 ) discard;
	}
}