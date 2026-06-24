#version 450
#pragma shader_stage(fragment)
#extension GL_EXT_samplerless_texture_functions : require

layout (location = 0) in vec2 inUv;
layout (location = 1) flat in uint inPass;

layout (location = 0) out vec4 outLeft;
layout (location = 1) out vec4 outRight;

layout (binding = 1) uniform sampler2D  samplerColor;

layout(constant_id = 0) const int FORCE_OPAQUE = 0;

#define TEXTURES 1

#include "../../common/macros.h"
#include "../../common/structs.h"
#include "../../common/functions.h"

void main() {
	outLeft = vec4(0.0);
	outRight = vec4(0.0);

	vec4 outColor = texture( samplerColor, inUv );
	if ( FORCE_OPAQUE == 1 ) outColor.a = 1;

	if ( inPass == 0 ) outLeft = outColor;
	else if ( inPass == 1 ) outRight = outColor;
}