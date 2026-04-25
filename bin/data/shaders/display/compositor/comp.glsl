#version 450
#pragma shader_stage(compute)

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(binding = 0) uniform sampler2D samplerA;
layout(binding = 1) uniform sampler2D samplerB;
layout(binding = 2, rgba16f) uniform writeonly image2D imageOutput;

void main() {
    ivec2 texCoords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(imageOutput);

    if(texCoords.x >= size.x || texCoords.y >= size.y) return;

    vec2 uv = (vec2(texCoords) + vec2(0.5)) / vec2(size);

    vec4 colorA = texture(samplerA, uv);
    vec4 colorB = texture(samplerB, uv);

    vec4 finalColor = vec4(mix(colorA.rgb, colorB.rgb, colorB.a), 1.0);

    imageStore(imageOutput, texCoords, finalColor);
}