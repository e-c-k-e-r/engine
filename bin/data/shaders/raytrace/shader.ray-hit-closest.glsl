#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_buffer_reference2 : enable
#extension GL_EXT_scalar_block_layout : enable
#pragma shader_stage(closest)
layout (constant_id = 0) const uint PASSES = 2;
layout (constant_id = 1) const uint TEXTURES = 512;
layout (constant_id = 2) const uint CUBEMAPS = 128;

#define ADDRESS_ENABLED 1
#define COMPUTE 1
#define PBR 1
#define RAYTRACE 1
#define MAX_TEXTURES TEXTURES

#include "../common/macros.h"
#include "../common/structs.h"

layout (std140, binding = 9) readonly buffer InstanceAddresseses {
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

layout(location = 0) rayPayloadInEXT RayTracePayload payload;

hitAttributeEXT vec2 attribs;

void main() {
	const vec3 bary = vec3(
		1.0 - attribs.x - attribs.y,
		attribs.x,
		attribs.y
	);

	const uint instanceID = gl_InstanceCustomIndexEXT;

	const InstanceAddresses instanceAddresses = instanceAddresses[instanceID];

	if ( !(0 < instanceAddresses.index) ) return;
	
	const DrawCommand drawCommand = Indirects(nonuniformEXT(instanceAddresses.indirect)).dc[instanceAddresses.drawID];
	const uint triangleID = gl_PrimitiveID + (drawCommand.indexID / 3);

	Triangle triangle;
	triangle.indices = Indices(nonuniformEXT(instanceAddresses.index)).i[triangleID];
	for ( uint _ = 0; _ < 3; ++_ ) triangle.indices[_] += drawCommand.vertexID;

	if ( 0 < instanceAddresses.vertex ) {
		Vertices vertices = Vertices(nonuniformEXT(instanceAddresses.vertex));
		for ( uint _ = 0; _ < 3; ++_ ) triangle.points[_] = vertices.v[triangle.indices[_]];
	} else {
		if ( 0 < instanceAddresses.position ) {
			VPos buf = VPos(nonuniformEXT(instanceAddresses.position));
			for ( uint _ = 0; _ < 3; ++_ ) triangle.points[_].position = buf.v[triangle.indices[_]];
		}
		if ( 0 < instanceAddresses.uv ) {
			VUv buf = VUv(nonuniformEXT(instanceAddresses.uv));
			for ( uint _ = 0; _ < 3; ++_ ) triangle.points[_].uv = buf.v[triangle.indices[_]];
		}
		if ( 0 < instanceAddresses.st ) {
			VSt buf = VSt(nonuniformEXT(instanceAddresses.st));
			for ( uint _ = 0; _ < 3; ++_ ) triangle.points[_].st = buf.v[triangle.indices[_]];
		}
		if ( 0 < instanceAddresses.normal ) {
			VNormal buf = VNormal(nonuniformEXT(instanceAddresses.normal));
			for ( uint _ = 0; _ < 3; ++_ ) triangle.points[_].normal = buf.v[triangle.indices[_]];
		}
	}

	triangle.point.position = triangle.points[0].position * bary[0] + triangle.points[1].position * bary[1] + triangle.points[2].position * bary[2];
	triangle.point.uv = triangle.points[0].uv * bary[0] + triangle.points[1].uv * bary[1] + triangle.points[2].uv * bary[2];
	triangle.point.st = triangle.points[0].st * bary[0] + triangle.points[1].st * bary[1] + triangle.points[2].st * bary[2];
	triangle.point.normal = triangle.points[0].normal * bary[0] + triangle.points[1].normal * bary[1] + triangle.points[2].normal * bary[2];

	payload.hit = true;
	payload.instanceID = instanceID;
	payload.triangle = triangle;
}