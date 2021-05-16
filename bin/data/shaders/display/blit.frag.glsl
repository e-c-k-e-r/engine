#version 450
#pragma shader_stage(fragment)
#extension GL_EXT_samplerless_texture_functions : require

layout (binding = 1) uniform sampler samp;
layout (binding = 2) uniform texture2D albedoLeftTexture;
layout (binding = 3) uniform texture2D albedoRightTexture;

layout (location = 0) in vec2 inUv;

layout (location = 0) out vec4 outAlbedoSpecular;

layout( push_constant ) uniform PushBlock {
  uint pass;
} PushConstant;

void main() {
	if ( PushConstant.pass == 0 ) {
		outAlbedoSpecular.rgb = texture(sampler2D(albedoLeftTexture, samp), inUv).rgb;
	} else {
		outAlbedoSpecular.rgb = texture(sampler2D(albedoRightTexture, samp), inUv).rgb;
	}
	outAlbedoSpecular.a = 1;
}