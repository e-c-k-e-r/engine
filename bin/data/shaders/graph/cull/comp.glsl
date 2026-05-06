#version 450
#pragma shader_stage(compute)

//#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_samplerless_texture_functions : enable

layout (constant_id = 0) const uint PASSES = 6;
layout (local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

#define COMPUTE 1
#define QUERY_MIPMAPS 1
#define DEPTH_BIAS 0.00005
#define FRUSTUM_CULLING 1
#define OCCLUSION_CULLING 0 // currently whack
#define LODS 1
#define MAX_LODS 4

#include "../../common/macros.h"
#include "../../common/structs.h"

float mipLevels( vec2 size ) {
	return ceil(log2(max(size.x, size.y)));
}
float mipLevels( ivec2 size ) {
	return ceil(log2(max(size.x, size.y)));
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

layout (std430, binding = 1) buffer DrawCommands {
	DrawCommand drawCommands[];
};

layout (std430, binding = 2) buffer Instances {
	Instance instances[];
};

layout (std430, binding = 3) buffer LODs {
	LODMetadata lodMetadata[];
};

layout (std430, binding = 4) buffer Objects {
	Object objects[];
};

layout (binding = 5) uniform sampler2D samplerDepth;

shared vec4 sharedPlanes[PASSES][6];

vec4 normalizePlane( vec4 p ) {
	return p / length(p.xyz);
}

void main() {
	const uint gID = gl_GlobalInvocationID.x;
	const uint lID = gl_LocalInvocationIndex;

	if ( lID == 0 ) {
		for (uint pass = 0; pass < PushConstant.passes; ++pass) {
			mat4 mat = camera.viewport[pass].projection * camera.viewport[pass].view;
			for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 2; ++j) {
				sharedPlanes[pass][i*2+j].x = mat[0][3] + (j == 0 ? mat[0][i] : -mat[0][i]);
				sharedPlanes[pass][i*2+j].y = mat[1][3] + (j == 0 ? mat[1][i] : -mat[1][i]);
				sharedPlanes[pass][i*2+j].z = mat[2][3] + (j == 0 ? mat[2][i] : -mat[2][i]);
				sharedPlanes[pass][i*2+j].w = mat[3][3] + (j == 0 ? mat[3][i] : -mat[3][i]);
				sharedPlanes[pass][i*2+j] = normalizePlane( sharedPlanes[pass][i*2+j] );
			}
		}
	}
	barrier();

	if ( gID >= drawCommands.length() ) return;

	const DrawCommand drawCommand = drawCommands[gID];
	if ( drawCommand.indices == 0 || drawCommand.vertices == 0 ) return;

	const Instance instance = instances[drawCommand.instanceID];
	const Object object = objects[instance.objectID];

	vec4 sphere = aabbToSphere( instance.bounds );
	vec3 worldCenter = (object.model * vec4(sphere.xyz, 1.0)).xyz;

	float scaleX = length(object.model[0].xyz);
	float scaleY = length(object.model[1].xyz);
	float scaleZ = length(object.model[2].xyz);
	float maxScale = max(max(scaleX, scaleY), scaleZ);
	float worldRadius = sphere.w * maxScale;

	bool isVisible = false;
#if FRUSTUM_CULLING
	for ( uint pass = 0; pass < PushConstant.passes; ++pass ) {
		bool insideFrustum = true;
		for ( int p = 0; p < 6; ++p ) {
			if ( dot(sharedPlanes[pass][p].xyz, worldCenter) + sharedPlanes[pass][p].w < -worldRadius ) {
				insideFrustum = false;
				break;
			}
		}
		if ( insideFrustum ) {
			isVisible = true;
			break;
		}
	}
#else
	isVisible = true;
#endif
#if OCCLUSION_CULLING
	if ( isVisible ) {
		isVisible = false;
		for ( uint pass = 0; pass < PushConstant.passes; ++pass ) {
			vec4 aabb;
			vec3 viewCenter = ( camera.viewport[pass].view * vec4(worldCenter, 1.0) ).xyz;

			mat4 proj = camera.viewport[pass].projection;
			float znear = proj[3][2];
			float P00 = proj[0][0];
			float P11 = proj[1][1];

			if ( projectSphere(viewCenter, worldRadius, znear, P00, P11, aabb) ) {
				vec2 pyramidSize = vec2(textureSize( samplerDepth, 0 ));
				float width = (aabb.z - aabb.x) * pyramidSize.x;
				float height = (aabb.w - aabb.y) * pyramidSize.y;

				float level = mipLevels(vec2(width, height));
				level = max(0.0, level);

				float d1 = textureLod(samplerDepth, vec2(aabb.x, aabb.y), level).x;
				float d2 = textureLod(samplerDepth, vec2(aabb.z, aabb.y), level).x;
				float d3 = textureLod(samplerDepth, vec2(aabb.x, aabb.w), level).x;
				float d4 = textureLod(samplerDepth, vec2(aabb.z, aabb.w), level).x;

				float depth = min(min(d1, d2), min(d3, d4));
				float depthSphere = znear / (viewCenter.z - worldRadius);

				if ( depthSphere >= depth - DEPTH_BIAS ) {
					isVisible = true;
					break;
				}
			} else {
				isVisible = true;
				break;
			}
		}
	}
#endif
#if LODS
	if ( isVisible ) {
		vec3 viewCenter = (camera.viewport[0].view * vec4(worldCenter, 1.0)).xyz;
		float dist = length(viewCenter);
		float P11 = abs(camera.viewport[0].projection[1][1]);
		float projectedSize = (worldRadius * P11) / max(dist, 0.001);

		uint lodLevel = 0;
		if ( projectedSize < 0.20 ) lodLevel = 1;
		if ( projectedSize < 0.08 ) lodLevel = 2;
		if ( projectedSize < 0.02 ) lodLevel = 3;
		lodLevel = min(lodLevel, MAX_LODS - 1);
		while ( lodLevel > 0 && lodMetadata[drawCommand.instanceID].levels[lodLevel].indices == 0 ) {
			lodLevel--;
		}

		LOD lod = lodMetadata[drawCommand.instanceID].levels[lodLevel];

		if ( lod.indices > 0 ) {
			drawCommands[gID].indices = lod.indices;
			drawCommands[gID].indexID = lod.indexID;
			drawCommands[gID].vertexID = lod.vertexID;
			drawCommands[gID].vertices = lod.vertices;
		}
	}
#endif
	drawCommands[gID].instances = isVisible ? 1 : 0;
}