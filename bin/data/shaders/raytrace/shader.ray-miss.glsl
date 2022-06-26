#version 460
#extension GL_EXT_ray_tracing : enable
#pragma shader_stage(miss)

#define ADDRESS_ENABLED 1
#define COMPUTE 1
#define PBR 1
#define RAYTRACE 1
#define MAX_TEXTURES TEXTURES

#include "../common/macros.h"
#include "../common/structs.h"

layout(location = 0) rayPayloadInEXT RayTracePayload payload;

void main() {
    payload.hit = false;
}