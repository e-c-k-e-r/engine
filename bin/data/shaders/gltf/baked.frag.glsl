#version 450

#define UF_DEFERRED_SAMPLING 0
#define UF_CAN_DISCARD 1

#define PI 3.1415926536f

layout (constant_id = 0) const uint TEXTURES = 1;
layout (binding = 0) uniform sampler2D samplerTextures[TEXTURES];

struct Material {
	vec4 colorBase;
	vec4 colorEmissive;

	float factorMetallic;
	float factorRoughness;
	float factorOcclusion;
	float factorAlphaCutoff;

	int indexAlbedo;
	int indexNormal;
	int indexEmissive;
	int indexOcclusion;
	
	int indexMetallicRoughness;
	int modeAlpha;
	int padding1;
	int padding2;
};
struct Texture {
	int index;
	int samp;
	int remap;
	float blend;

	vec4 lerp;
};
layout (std140, binding = 1) readonly buffer Materials {
	Material materials[];
};
layout (std140, binding = 2) readonly buffer Textures {
	Texture textures[];
};

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec2 inSt;
layout (location = 2) in vec4 inColor;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in mat3 inTBN;
layout (location = 7) in vec3 inPosition;
layout (location = 8) flat in ivec4 inId;

layout (location = 0) out uvec2 outId;
layout (location = 1) out vec2 outNormals;
#if UF_DEFERRED_SAMPLING
	layout (location = 2) out vec2 outUvs;
#else
	layout (location = 2) out vec4 outAlbedo;
#endif

vec2 encodeNormals( vec3 n ) {
//	return n.xy / sqrt(n.z*8+8) + 0.5;
	return (vec2(atan(n.y,n.x)/PI, n.z)+1.0)*0.5;
}

float wrap( float i ) {
	return fract(i);
}
vec2 wrap( vec2 uv ) {
	return vec2( wrap( uv.x ), wrap( uv.y ) );
}
float mipLevel( in vec2 uv ) {
	const vec2 dx_vtc = dFdx(uv);
	const vec2 dy_vtc = dFdy(uv);
	return 0.5 * log2(max(dot(dx_vtc, dx_vtc), dot(dy_vtc, dy_vtc)));
}
bool validTextureIndex( int textureIndex ) {
	return 0 <= textureIndex && textureIndex < TEXTURES;
}
void main() {
	const float mip = mipLevel(inUv.xy);
	const vec2 uv = wrap(inUv.xy);
	vec4 A = vec4(0, 0, 0, 0);
	const vec3 P = inPosition;
	const vec3 N = inNormal;
#if UF_DEFERRED_SAMPLING
	outUvs = wrap(inUv.xy);
	const vec4 outAlbedo = vec4(0,0,0,0);
#endif
#if !UF_DEFERRED_SAMPLING
	A = textureLod( samplerTextures[0], inSt, mip );
#endif
	outNormals = encodeNormals( N );
	outId = ivec2(inId.w+1, inId.y+1);
}