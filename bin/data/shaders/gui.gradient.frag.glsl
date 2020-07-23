#version 450

layout (binding = 1) uniform sampler2D samplerColor;

struct Gui {
	vec4 offset;
	vec4 color;
	int mode;
	float depth;
};

layout (location = 0) in vec2 inUv;
layout (location = 1) in flat Gui inGui;
layout (location = 0) out vec4 outFragColor;

void main() {
	if ( inUv.x < inGui.offset.x ) discard;
	if ( inUv.y < inGui.offset.y ) discard;
	if ( inUv.x > inGui.offset.z ) discard;
	if ( inUv.y > inGui.offset.w ) discard;

	outFragColor = texture(samplerColor, inUv);// vec4(inUv.s, inUv.t, 1.0, 1.0);
	if ( outFragColor.a < 0.001 ) discard;
	
	if ( inGui.mode == 1 ) {
		outFragColor = inGui.color;
	} else {
		outFragColor *= inGui.color;
	}
	
	float end = 0.25;
	if ( inUv.x < end ) {
		outFragColor.a = inUv.x / end;
	} else if ( inUv.x + end >= 1 ) {
		float offset = (1 - inUv.x);
		outFragColor.a = offset / end;
	}
}