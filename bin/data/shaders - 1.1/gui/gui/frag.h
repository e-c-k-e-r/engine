#define TEXTURES 1
#define CUBEMAPS 1

#include "../../common/macros.h"
#include "../../common/structs.h"
#include "../../common/functions.h"
#include "./gui.h"

layout (binding = 1) uniform sampler2D samplerTexture;

layout (location = 0) in vec2 inUv;
layout (location = 1) in flat Gui inGui;
layout (location = 7) in flat Glyph inGlyph;

layout (location = 0) out vec4 outAlbedo;

void main() {
	if ( inUv.x < inGui.offset.x || inUv.y < inGui.offset.y || inUv.x > inGui.offset.z || inUv.y > inGui.offset.w ) discard;

	const vec2 uv = inUv.xy;
	const float mip = mipLevel(dFdx(inUv), dFdy(inUv));
	vec4 C = inGui.color;
#if GLYPH
	if ( enabled(inGui.mode, 1) ) {
		outAlbedo = inGui.color;
		return;
	}
	const float sampled = texture(samplerTexture, inUv).r;
	if ( enabled(inGui.mode, 2) ) {
		const float smoothing = ( inGlyph.spread > 0 && inGlyph.scale > 0 ) ? 0.25 / (inGlyph.spread * inGlyph.scale) : 0.25 / (4 * 1.5);
		const float outlining = smoothstep(0.5 - smoothing, 0.5 + smoothing, sampled);
		const float alpha = smoothstep(inGlyph.weight - smoothing, inGlyph.weight + smoothing, sampled);
		if ( alpha < 0.001 || alpha > 1 ) discard;
		C = mix(inGlyph.stroke, inGui.color, outlining);
		C.a = inGui.color.a * alpha;
	} else {
		if ( sampled < 0.001 || sampled > 1 ) discard;
		C *= sampled;
	}
#else
	if ( enabled(inGui.mode, 0) ) C = inGui.color;
	else C *= textureLod( samplerTexture, uv, mip );
#endif
	outAlbedo = C;
}