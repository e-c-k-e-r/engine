#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_buffer_reference2 : enable
#extension GL_EXT_scalar_block_layout : enable
#pragma shader_stage(anyhit)
layout (constant_id = 0) const uint PASSES = 2;
layout (constant_id = 1) const uint TEXTURES = 512;
layout (constant_id = 2) const uint CUBEMAPS = 128;

#define RT 1
#define COMPUTE 1

#include "../common/macros.h"
#include "../common/structs.h"

layout(location = 0) rayPayloadInEXT RayTracePayload payload;

hitAttributeEXT vec2 attribs;

void main() {
	payload.hit = true;
	payload.instanceID = gl_InstanceCustomIndexEXT;
	payload.primitiveID = gl_PrimitiveID;
	payload.attributes = attribs;

}