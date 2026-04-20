#version 450
#pragma shader_stage(compute)

//#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_samplerless_texture_functions : enable

layout (constant_id = 0) const uint PASSES = 6;
layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

#define COMPUTE 1
#define QUERY_MIPMAPS 1
#define DEPTH_BIAS 0.00005

#include "../../common/macros.h"
#include "../../common/structs.h"

float mipLevels( vec2 size ) {
	return floor(log2(max(size.x, size.y)));
}
float mipLevels( ivec2 size ) {
	return floor(log2(max(size.x, size.y)));
}

vec4 aabbToSphere( Bounds bounds ) {
	vec4 sphere;
	sphere.xyz = (bounds.max + bounds.min) * 0.5;
	sphere.w = length((bounds.max - bounds.min) * 0.5);
	return sphere;
}

// 2D Polyhedral Bounds of a Clipped, Perspective-Projected 3D Sphere. Michael Mara, Morgan McGuire. 2013
bool projectSphere(vec3 C, float r, float znear, float P00, float P11, out vec4 aabb)
{
	if (C.z < r + znear)
		return false;

	vec2 cx = -C.xz;
	vec2 vx = vec2(sqrt(dot(cx, cx) - r * r), r);
	vec2 minx = mat2(vx.x, vx.y, -vx.y, vx.x) * cx;
	vec2 maxx = mat2(vx.x, -vx.y, vx.y, vx.x) * cx;

	vec2 cy = -C.yz;
	vec2 vy = vec2(sqrt(dot(cy, cy) - r * r), r);
	vec2 miny = mat2(vy.x, vy.y, -vy.y, vy.x) * cy;
	vec2 maxy = mat2(vy.x, -vy.y, vy.y, vy.x) * cy;

	aabb = vec4(minx.x / minx.y * P00, miny.x / miny.y * P11, maxx.x / maxx.y * P00, maxy.x / maxy.y * P11);
	aabb = aabb.xwzy * vec4(0.5f, -0.5f, 0.5f, -0.5f) + vec4(0.5f); // clip space -> uv space

	return true;
}

layout( push_constant ) uniform PushBlock {
  uint pass;
  uint passes;
} PushConstant;

layout (binding = 0) uniform Camera {
	Viewport viewport[PASSES];
} camera;

layout (std140, binding = 1) buffer DrawCommands {
	DrawCommand drawCommands[];
};

layout (std140, binding = 2) buffer Instances {
	Instance instances[];
};

layout (std140, binding = 3) buffer Objects {
	Object objects[];
};

layout (binding = 4) uniform sampler2D samplerDepth;

struct Frustum {
	vec4 planes[6];
};

vec4 normalizePlane( vec4 p ) {
	return p / length(p.xyz);
}

bool frustumCull( uint id ) {
	if ( PushConstant.passes == 0 ) return true;
	
	const DrawCommand drawCommand = drawCommands[id];
	const Instance instance = instances[drawCommand.instanceID];
	const Object object = objects[instance.objectID];

	if ( drawCommand.indices == 0 || drawCommand.vertices == 0 ) return false;

	bool visible = true;
	for ( uint pass = 0; pass < PushConstant.passes; ++pass ) {
		mat4 mat = camera.viewport[pass].projection * camera.viewport[pass].view * object.model;
		vec4 planes[6]; {
			for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 2; ++j) {
				planes[i*2+j].x = mat[0][3] + (j == 0 ? mat[0][i] : -mat[0][i]);
				planes[i*2+j].y = mat[1][3] + (j == 0 ? mat[1][i] : -mat[1][i]);
				planes[i*2+j].z = mat[2][3] + (j == 0 ? mat[2][i] : -mat[2][i]);
				planes[i*2+j].w = mat[3][3] + (j == 0 ? mat[3][i] : -mat[3][i]);
				planes[i*2+j] = normalizePlane( planes[i*2+j] );
			}
		}
		bool insideFrustum = true;
		for ( uint p = 0; p < 6; ++p ) {
			float d = max(instance.bounds.min.x * planes[p].x, instance.bounds.max.x * planes[p].x)
					+ max(instance.bounds.min.y * planes[p].y, instance.bounds.max.y * planes[p].y)
					+ max(instance.bounds.min.z * planes[p].z, instance.bounds.max.z * planes[p].z);
			
			if (d < -planes[p].w) {
        visible = false;
        break;
      }
		}
		if ( !visible ) break;
	}
	return visible;
}

bool occlusionCull( uint id ) {
	if ( PushConstant.passes == 0 ) return true;
	
	const DrawCommand drawCommand = drawCommands[id];
	const Instance instance = instances[drawCommand.instanceID];
	const Object object = objects[instance.objectID];

	bool visible = true;
	for ( uint pass = 0; pass < PushConstant.passes; ++pass ) {
		vec4 aabb;
		vec4 sphere = aabbToSphere( instance.bounds );
		vec3 center = (camera.viewport[pass].view * object.model * vec4(sphere.xyz, 1)).xyz;
		float radius = (object.model * vec4(sphere.w, 0, 0, 0)).x;

		mat4 proj = camera.viewport[pass].projection;
		float znear = proj[3][2];
		float P00 = proj[0][0];
		float P11 = proj[1][1];
		if (projectSphere(center, radius, znear, P00, P11, aabb)) {
			ivec2 pyramidSize = textureSize( samplerDepth, 0 );
			float mips = mipLevels( pyramidSize );

			float width = (aabb.z - aabb.x) * pyramidSize.x;
			float height = (aabb.w - aabb.y) * pyramidSize.y;

			//find the mipmap level that will match the screen size of the sphere
			float level = floor(log2(max(width, height)));
		//	if ( level == mips )
				--level;
			level = clamp( level, 0, mips );

			//sample the depth pyramid at that specific level
			float depth = textureLod(samplerDepth, (aabb.xy + aabb.zw) * 0.5, level).x;

			float depthSphere = znear / (center.z - radius);

			instances[drawCommand.instanceID].bounds.padding1 = depth;
			instances[drawCommand.instanceID].bounds.padding2 = proj[3][2];

			//if the depth of the sphere is in front of the depth pyramid value, then the object is visible
			visible = visible && depthSphere >= depth - DEPTH_BIAS;
		}
	}
	return visible;
}

void main() {
	const uint gID = gl_GlobalInvocationID.x;
	if ( !(0 <= gID && gID < drawCommands.length()) ) return;

	bool visible = frustumCull( gID );
//	if ( visible ) visible = occlusionCull( gID );
	drawCommands[gID].instances = visible ? 1 : 0;
}