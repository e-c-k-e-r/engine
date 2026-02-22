#version 450
#pragma shader_stage(compute)

#define COMPUTE 1
#define TEXTURES 0
#define CUBEMAPS 0
#define BLOOM 1

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout( push_constant ) uniform PushBlock {
  uint eye;
  uint mode;
} PushConstant;

layout (binding = 0) uniform UBO {
	float threshold;
	float smoothness;
	uint size;
	float padding1;

	float weights[32];
} ubo;

layout (binding = 1, rgba16f) uniform image2D imageColor;
layout (binding = 2, rgba16f) uniform image2D imageBloom;
layout (binding = 3, rgba16f) uniform image2D imagePingPong;

#include "../../common/macros.h"
#include "../../common/structs.h"
#include "../../common/functions.h"

void main() {
	const uint mode = PushConstant.mode;
	const ivec2 texel = ivec2(gl_GlobalInvocationID.xy);
	const ivec2 size = imageSize( imageColor );
	if ( texel.x >= size.x || texel.y >= size.y ) return;
	
	if ( mode == 0 ) { // fill bloom
		vec3 result = imageLoad( imageColor, texel ).rgb;
		float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
		if( brightness < ubo.threshold ) result = vec3(0.0);
		imageStore(imageBloom, texel, vec4(result, 1.0));
	} else if ( mode == 1 ) { // bloom horizontal
		vec3 result = imageLoad( imageBloom, texel ).rgb * ubo.weights[0];
    for ( int i = 1; i < int(ubo.size); ++i ) {
			vec3 c1 = imageLoad( imageBloom, texel + ivec2(i, 0) ).rgb;
			vec3 c2 = imageLoad( imageBloom, texel - ivec2(i, 0) ).rgb;
			result += (c1 + c2) * ubo.weights[i];
    }
    imageStore( imagePingPong, texel, vec4(result, 1.0) );
	} else if ( mode == 2 ) { // bloom vertical
		vec3 result = imageLoad( imagePingPong, texel ).rgb * ubo.weights[0];
		for( int i = 1; i < int(ubo.size); ++i ) {
			vec3 c1 = imageLoad( imagePingPong, texel + ivec2(0, i) ).rgb;
			vec3 c2 = imageLoad( imagePingPong, texel - ivec2(0, i) ).rgb;
			result += (c1 + c2) * ubo.weights[i];
		}
		imageStore(imageBloom, texel, vec4(result, 1.0));
	} else if ( mode == 3 ) { // combine
		vec3 base = imageLoad( imageColor, texel ).rgb;
		vec3 bloom = imageLoad( imageBloom, texel ).rgb;
		imageStore( imageColor, texel, vec4(base + bloom, 1.0) );
	}
}