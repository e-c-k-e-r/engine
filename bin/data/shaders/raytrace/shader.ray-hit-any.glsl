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

#define BUFFER_REFERENCE 1
#define COMPUTE 1
#define PBR 1
#define RAYTRACE 1
#define MAX_TEXTURES TEXTURES

#include "../common/macros.h"
#include "../common/structs.h"

/*
layout (std140, binding = 10) readonly buffer InstanceAddresseses {
	InstanceAddresses instanceAddresses[];
};

layout(buffer_reference, scalar) buffer Vertices { Vertex v[]; };
layout(buffer_reference, scalar) buffer Indices { uvec3 i[]; };
layout(buffer_reference, scalar) buffer Indirects { DrawCommand dc[]; };

layout(buffer_reference, scalar) buffer VPos { vec3 v[]; };
layout(buffer_reference, scalar) buffer VUv { vec2 v[]; };
layout(buffer_reference, scalar) buffer VColor { uint v[]; };
layout(buffer_reference, scalar) buffer VSt { vec2 v[]; };
layout(buffer_reference, scalar) buffer VNormal { vec3 v[]; };
layout(buffer_reference, scalar) buffer VTangent { vec3 v[]; };
layout(buffer_reference, scalar) buffer VID { uint v[]; };
*/

layout(location = 0) rayPayloadInEXT RayTracePayload payload;

hitAttributeEXT vec2 attribs;

void main() {
	payload.hit = true;
	payload.instanceID = gl_InstanceCustomIndexEXT;
	payload.primitiveID = gl_PrimitiveID;
	payload.attributes = attribs;

}