#version 450
#pragma shader_stage(fragment)
#extension GL_EXT_samplerless_texture_functions : require

layout (location = 0) in vec2 inUv;
layout (location = 1) flat in uint inPass;

layout (location = 0) out vec4 outAlbedo;

layout (binding = 0) uniform sampler2D  samplerAlbedo;

void main() {
	outAlbedo = texture( samplerAlbedo, inUv );
}