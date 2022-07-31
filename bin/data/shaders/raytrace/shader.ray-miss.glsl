#version 460
#extension GL_EXT_ray_tracing : enable
#pragma shader_stage(miss)

#define RT 1
#define COMPUTE 1

#include "../common/macros.h"
#include "../common/structs.h"

layout(location = 0) rayPayloadInEXT RayTracePayload payload;

void main() {
    payload.hit = false;
}