#version 450
#pragma shader_stage(compute)

#define COMPUTE 1

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout (constant_id = 0) const uint MIPS = 6;

layout (binding = 0, rgba16f) uniform image2D imageColor;
layout (binding = 1) uniform sampler2D samplerBloom;

layout (binding = 2) uniform UBO {
	float threshold;
	float smoothness;
	uint size;
	float padding1;

	float weights[32];
} ubo;

// 9-tap bilinear tent filter
vec3 tentFilter(sampler2D tex, vec2 uv, float lod) {
	vec2 texSize = vec2(textureSize(tex, int(lod)));
	vec4 d = (1.0 / texSize.xyxy) * vec4(1.0, 1.0, -1.0, 0.0);

	vec3 s = textureLod(tex, uv - d.xy, lod).rgb;
	s += textureLod(tex, uv - d.wy, lod).rgb * 2.0;
	s += textureLod(tex, uv - d.zy, lod).rgb;
	s += textureLod(tex, uv + d.zw, lod).rgb * 2.0;
	s += textureLod(tex, uv,		lod).rgb * 4.0;
	s += textureLod(tex, uv + d.xw, lod).rgb * 2.0;
	s += textureLod(tex, uv + d.zy, lod).rgb;
	s += textureLod(tex, uv + d.wy, lod).rgb * 2.0;
	s += textureLod(tex, uv + d.xy, lod).rgb;

	return s * (1.0 / 16.0);
}

void main() {
	ivec2 texel = ivec2(gl_GlobalInvocationID.xy);
	ivec2 size = imageSize(imageColor);
	if ( texel.x >= size.x || texel.y >= size.y ) return;

	vec2 uv = (vec2(texel) + 0.5) / vec2(size);
	vec3 bloomAcc = vec3(0.0);
	float weightSum = 0.0;

	for ( uint i = 0; i < min(MIPS, ubo.size); ++i ) {
		float w = ubo.weights[i];
		bloomAcc += textureLod(samplerBloom, uv, float(i)).rgb * w;
		//bloomAcc += tentFilter(samplerBloom, uv, float(i)) * w;
		weightSum += w;
	}

	if ( weightSum > 0.0 ) bloomAcc /= weightSum;

	vec3 base = imageLoad( imageColor, texel ).rgb;
	imageStore( imageColor, texel, vec4(base + bloomAcc, 1.0) );
}