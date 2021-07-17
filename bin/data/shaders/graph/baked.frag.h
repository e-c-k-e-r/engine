#define CUBEMAPS 1
#define MAX_TEXTURES TEXTURES
layout (constant_id = 0) const uint TEXTURES = 1;

#include "../common/macros.h"
#include "../common/structs.h"
#include "../common/functions.h"

layout (binding = 0) uniform sampler2D samplerTextures[TEXTURES];

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
#if DEFERRED_SAMPLING
	layout (location = 2) out vec2 outUvs;
#else
	layout (location = 2) out vec4 outAlbedo;
#endif

void main() {
	const float mip = mipLevel(inUv.xy);
	const vec2 uv = wrap(inUv.xy);
	vec4 A = vec4(0, 0, 0, 0);
	const vec3 P = inPosition;
	const vec3 N = inNormal;
#if DEFERRED_SAMPLING
	outUvs = wrap(inUv.xy);
	const vec4 outAlbedo = vec4(0,0,0,0);
#endif
#if !DEFERRED_SAMPLING
	A = textureLod( samplerTextures[0], inSt, mip );
#endif
	outNormals = encodeNormals( N );
	outId = ivec2(inId.x+1, inId.y+1);
}