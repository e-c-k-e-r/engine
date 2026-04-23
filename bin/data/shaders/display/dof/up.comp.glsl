#version 450
#pragma shader_stage(compute)

#define COMPUTE 1

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout (constant_id = 0) const uint MIPS = 6;

layout (binding = 0, rgba16f) uniform image2D imageColor;
layout (binding = 1) uniform sampler2D samplerDepth;
layout (binding = 2) uniform sampler2D samplerDof;

layout (binding = 3) uniform UBO {
	float focusDistance;
	float focusRange;
	float maxCoC;
	float nearPlane;
} ubo;


float linearizeDepth( float d ) {
	return ubo.nearPlane / max(d, 0.000001);
}

float calculateCoC( float depth ) {
	float dist = abs(linearizeDepth(depth) - ubo.focusDistance);
	return clamp(dist / ubo.focusRange, 0.0, 1.0) * ubo.maxCoC;
}

void main() {
ivec2 texel = ivec2(gl_GlobalInvocationID.xy);
	ivec2 size = imageSize(imageColor);
	if ( texel.x >= size.x || texel.y >= size.y ) return;
	vec2 uv = (vec2(texel) + 0.5) / vec2(size);

	vec3 base = imageLoad( imageColor, texel ).rgb;
	float depth = textureLod( samplerDepth, uv, 0.0 ).r;

	float pixelCoC = calculateCoC(depth);

	vec4 neighborhoodDof = textureLod( samplerDof, uv, 1.0 );
	float neighborhoodCoC = neighborhoodDof.a;

	float effectiveCoC = max(pixelCoC, neighborhoodCoC);

	if ( effectiveCoC > 0.001 ) {
		float targetMip = effectiveCoC * float(MIPS - 1);

		vec4 dofSample = textureLod( samplerDof, uv, targetMip );

		float blendFactor = smoothstep( 0.0, 0.1, effectiveCoC );

		base = mix( base, dofSample.rgb, blendFactor );
	}

	imageStore( imageColor, texel, vec4(base, 1.0) );
}