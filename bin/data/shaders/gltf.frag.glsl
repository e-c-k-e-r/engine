#version 450

layout (constant_id = 0) const uint TEXTURES = 1;
layout (binding = 1) uniform sampler2D samplerTextures[TEXTURES];

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec4 inColor;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inPosition;
layout (location = 4) flat in uint inId;

layout (location = 0) out vec4 outAlbedoSpecular;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outPosition;

void main() {
	uint offset = inId;
	outAlbedoSpecular = texture(samplerTextures[offset], inUv.xy);
	if ( outAlbedoSpecular.a < 0.001 ) discard;
	outAlbedoSpecular.rgb *= inColor.rgb;
	outAlbedoSpecular.a = 1;
	
	outNormal = vec4(inNormal,1);
	outPosition = vec4(inPosition,1);
}