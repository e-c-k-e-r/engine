#version 460
#extension GL_EXT_ray_tracing : enable
#pragma shader_stage(miss)

layout(location = 0) rayPayloadInEXT vec4 hitValue;

void main() {
    hitValue = vec4(0.2, 0.2, 0.2, 1.0);
}