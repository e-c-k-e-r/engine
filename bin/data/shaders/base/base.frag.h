#define TEXTURES 1
#define CUBEMAPS 1
#define MAX_TEXTURES TEXTURES
#include "../common/macros.h"
#include "../common/structs.h"
#include "../common/functions.h"

layout (binding = 2) uniform sampler2D samplerTexture;

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec4 inColor;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inPosition;

layout (location = 0) out uvec2 outId;
layout (location = 1) out vec2 outNormals;
#if DEFERRED_SAMPLING
	layout (location = 2) out vec2 outUvs;
#else
	layout (location = 2) out vec4 outAlbedo;
#endif

void main() {
	float mip = mipLevel(inUv.xy);
	vec2 uv = inUv.xy;
	vec4 C = vec4(1, 1, 1, 1);
	vec3 P = inPosition;
	vec3 N = inNormal;
#if DEFERRED_SAMPLING
	outUvs = wrap(inUv.xy);
	vec4 outAlbedo = vec4(0,0,0,0);
#endif
#if !DEFERRED_SAMPLING || CAN_DISCARD
	C = textureLod( samplerTexture, uv, mip );
#endif
#if !DEFERRED_SAMPLING
	outAlbedo = C * inColor;
#endif
	outNormals = encodeNormals( N );
	//outId = ivec2(0, 0);
}