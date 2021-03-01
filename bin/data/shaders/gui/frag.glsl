#version 450

#define UF_DEFERRED_SAMPLING 0
#define UF_CAN_DISCARD 1

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
#if UF_DEFERRED_SAMPLING
	layout (location = 2) out vec2 outUvs;
#else
	layout (location = 2) out vec4 outAlbedo;
#endif

vec2 encodeNormals( vec3 n ) {
	float p = sqrt(n.z*8+8);
	return n.xy/p + 0.5;
}
float mipLevel( in vec2 uv ) {
	vec2 dx_vtc = dFdx(uv);
	vec2 dy_vtc = dFdy(uv);
	return 0.5 * log2(max(dot(dx_vtc, dx_vtc), dot(dy_vtc, dy_vtc)));
}

void main() {
	if ( inUv.x < inGui.offset.x ) discard;
	if ( inUv.y < inGui.offset.y ) discard;
	if ( inUv.x > inGui.offset.z ) discard;
	if ( inUv.y > inGui.offset.w ) discard;

	float mip = mipLevel(inUv.xy);
	vec2 uv = inUv.xy;
	vec4 C = vec4(1, 1, 1, 1);
	//vec3 N = inNormal;
#if UF_DEFERRED_SAMPLING
	outUvs = wrap(inUv.xy);
	vec4 outAlbedo = vec4(0,0,0,0);
#endif
#if !UF_DEFERRED_SAMPLING || UF_CAN_DISCARD
	C = textureLod( samplerTexture, uv, mip );
#endif
#if !UF_DEFERRED_SAMPLING
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