#version 450
#pragma shader_stage(compute)

//#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_samplerless_texture_functions : enable

layout (constant_id = 0) const uint PASSES = 6;
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

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

layout (binding = 3) uniform sampler2D samplerDepth;

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

	bool visible = false;
	for ( uint pass = 0; pass < PushConstant.passes; ++pass ) {
#if 0
		vec4 sphere = aabbToSphere( instance.bounds );
		vec3 center = vec3( camera.viewport[pass].view * instance.model * vec4(  ) );
#else
		mat4 mat = camera.viewport[pass].projection * camera.viewport[pass].view * instance.model;
	#if 1
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
		for ( uint p = 0; p < 6; ++p ) {
			float d = max(instance.bounds.min.x * planes[p].x, instance.bounds.max.x * planes[p].x)
					+ max(instance.bounds.min.y * planes[p].y, instance.bounds.max.y * planes[p].y)
					+ max(instance.bounds.min.z * planes[p].z, instance.bounds.max.z * planes[p].z);
			if ( d > -planes[p].w ) return true;
		}
	#else
		vec4 corners[8] = {
			vec4( instance.bounds.min.x, instance.bounds.min.y, instance.bounds.min.z, 1.0 ),
			vec4( instance.bounds.max.x, instance.bounds.min.y, instance.bounds.min.z, 1.0 ),
			vec4( instance.bounds.max.x, instance.bounds.max.y, instance.bounds.min.z, 1.0 ),
			vec4( instance.bounds.min.x, instance.bounds.max.y, instance.bounds.min.z, 1.0 ),

			vec4( instance.bounds.min.x, instance.bounds.min.y, instance.bounds.max.z, 1.0 ),
			vec4( instance.bounds.max.x, instance.bounds.min.y, instance.bounds.max.z, 1.0 ),
			vec4( instance.bounds.max.x, instance.bounds.max.y, instance.bounds.max.z, 1.0 ),
			vec4( instance.bounds.min.x, instance.bounds.max.y, instance.bounds.max.z, 1.0 ),
		};
		vec4 planes[6]; {
			#pragma unroll 3
			for (int i = 0; i < 3; ++i)
			#pragma unroll 2
			for (int j = 0; j < 2; ++j) {
				planes[i*2+j].x = mat[0][3] + (j == 0 ? mat[0][i] : -mat[0][i]);
				planes[i*2+j].y = mat[1][3] + (j == 0 ? mat[1][i] : -mat[1][i]);
				planes[i*2+j].z = mat[2][3] + (j == 0 ? mat[2][i] : -mat[2][i]);
				planes[i*2+j].w = mat[3][3] + (j == 0 ? mat[3][i] : -mat[3][i]);
				planes[i*2+j] = normalizePlane( planes[i*2+j] );
			}
		}
		#pragma unroll 8
		for ( uint p = 0; p < 8; ++p ) corners[p] = mat * corners[p];
		#pragma unroll 6
		for ( uint p = 0; p < 6; ++p ) {
			#pragma unroll 8
			for ( uint q = 0; q < 8; ++q ) {
				if ( dot( corners[q], planes[p] ) > 0 ) return true;
			}
			return false;
		}
	#endif
#endif
	}
	return visible;
}

bool occlusionCull( uint id ) {
	if ( PushConstant.passes == 0 ) return true;
	
	const DrawCommand drawCommand = drawCommands[id];
	const Instance instance = instances[drawCommand.instanceID];

	bool visible = true;
	for ( uint pass = 0; pass < PushConstant.passes; ++pass ) {
#if 1
		vec4 aabb;
		vec4 sphere = aabbToSphere( instance.bounds );
		vec3 center = (camera.viewport[pass].view * instance.model * vec4(sphere.xyz, 1)).xyz;
		float radius = (instance.model * vec4(sphere.w, 0, 0, 0)).x;
	//	center.y *= -1;
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

#else
		mat4 mat = camera.viewport[pass].projection * camera.viewport[pass].view * instance.model;
		vec3 boundsSize = instance.bounds.max - instance.bounds.min;
		vec3 points[8] = {
			instance.bounds.min.xyz,
			instance.bounds.min.xyz + vec3(boundsSize.x,0,0),
			instance.bounds.min.xyz + vec3(0, boundsSize.y,0),
			instance.bounds.min.xyz + vec3(0, 0, boundsSize.z),
			instance.bounds.min.xyz + vec3(boundsSize.xy,0),
			instance.bounds.min.xyz + vec3(0, boundsSize.yz),
			instance.bounds.min.xyz + vec3(boundsSize.x, 0, boundsSize.z),
			instance.bounds.min.xyz + boundsSize.xyz,
		};
		vec2 minXY = vec2(1);
		vec2 maxXY = vec2(0);
		
		float minZ = 1;
		float maxZ = 0;

		#pragma unroll 8
		for ( uint i = 0; i < 8; ++i ) {
			vec4 clip = mat * vec4( points[i], 1 );
			clip.xyz /= clip.w;
			clip.xy = clip.xy * 0.5 + 0.5;

			minXY.x = min(minXY.x, clip.x);
			minXY.y = min(minXY.y, clip.y);
			
			maxXY.x = max(maxXY.x, clip.x);
			maxXY.y = max(maxXY.y, clip.y);

		#if INVERSE
			clip.z = 1.0 - clip.z;
			maxZ = max(maxZ, clip.z);
		#else
			minZ = min(minZ, clip.z);
		#endif
		}

		if ( maxXY.x <= 0 || maxXY.y <= 0 ) return false;
		if ( minXY.x >= 1 || minXY.y >= 1 ) return false;
		
		ivec2 depthSize = textureSize( samplerDepth, 0 );
		float mips = mipLevels( depthSize );

		vec4 uv = vec4(minXY, maxXY);

		ivec2 clipSize = ivec2(maxXY - minXY) * depthSize;
		float mip = mipLevels( clipSize );
		mip = clamp( mip, 0, mips );
		if ( mip == 0 ) {
			mip = 1;
		} else {
			float lower = max(mip - 1, 0);
			float scale = exp2(-lower);
			vec2 a = floor(uv.xy * scale);
			vec2 b = ceil(uv.zw * scale);
			vec2 dims = b - a;

			// Use the lower level if we only touch <= 2 texels in both dimensions
			if (dims.x <= 2 && dims.y <= 2) mip = lower;
		}

		float depths[4] = {
			textureLod( samplerDepth, uv.xy, mip ).r,
			textureLod( samplerDepth, uv.zy, mip ).r,
			textureLod( samplerDepth, uv.xw, mip ).r,
			textureLod( samplerDepth, uv.zw, mip ).r,
		};
	#if INVERSE
		float minDepth = 1.0 - min(min(min(depths[0], depths[1]), depths[2]), depths[3]);
	#else
		float maxDepth = max(max(max(depths[0], depths[1]), depths[2]), depths[3]);
	#endif

		instances[drawCommand.instanceID].bounds.padding1 = minZ;
		instances[drawCommand.instanceID].bounds.padding2 = maxDepth;

		return minZ <= maxDepth;
#endif
	}
	return visible;
}

void main() {
	const uint gID = gl_GlobalInvocationID.x;
	if ( !(0 <= gID && gID < drawCommands.length()) ) return;

	bool visible = frustumCull( gID );
	if ( visible ) visible = occlusionCull( gID );
//	bool visible = occlusionCull( gID );
	drawCommands[gID].instances = visible ? 1 : 0;
}


/*
		Frustum frustum;
		for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 2; ++j) {
			frustum.planes[i*2+j].x = mat[0][3] + (j == 0 ? mat[0][i] : -mat[0][i]);
			frustum.planes[i*2+j].y = mat[1][3] + (j == 0 ? mat[1][i] : -mat[1][i]);
			frustum.planes[i*2+j].z = mat[2][3] + (j == 0 ? mat[2][i] : -mat[2][i]);
			frustum.planes[i*2+j].w = mat[3][3] + (j == 0 ? mat[3][i] : -mat[3][i]);
			frustum.planes[i*2+j]*= length(frustum.planes[i*2+j].xyz);
		}
		for ( uint i = 0; i < 6; ++i ) {
			vec4 plane = frustum.planes[i];
			float d = dot(instance.bounds.center, plane.xyz);
			float r = dot(instance.bounds.extent, abs(plane.xyz));
			bool inside = d + r > -plane.w;
			if ( !inside ) return 0;
		}
		return true;
*/
/*
		vec4 plane;
		vec4 center = vec4( (max + min) * 0.5, 1 );
		vec4 extent = vec4( (max - min) * 0.5, 1 );
		center = mat * center;
		extent = mat * extent;
		center.xyz /= center.w;
		extent.xyz /= extent.w;
		for (int i = 0; i < 4; ++i ) plane[i] = mat[i][3] + mat[i][0]; // left
		visible = dot(center.xyz + extent.xyz * sign(plane.xyz), plane.xyz ) > -plane.w;
		if ( visible ) return true;
		
		for (int i = 0; i < 4; ++i ) plane[i] = mat[i][3] - mat[i][0]; // right
		visible = dot(center.xyz + extent.xyz * sign(plane.xyz), plane.xyz ) > -plane.w;
		if ( visible ) return true;
		
		for (int i = 0; i < 4; ++i ) plane[i] = mat[i][3] + mat[i][1]; // bottom
		visible = dot(center.xyz + extent.xyz * sign(plane.xyz), plane.xyz ) > -plane.w;
		if ( visible ) return true;
		
		for (int i = 0; i < 4; ++i ) plane[i] = mat[i][3] - mat[i][1]; // top
		visible = dot(center.xyz + extent.xyz * sign(plane.xyz), plane.xyz ) > -plane.w;
		if ( visible ) return true;
		
		for (int i = 0; i < 4; ++i ) plane[i] = mat[i][3] + mat[i][2]; // near
		visible = dot(center.xyz + extent.xyz * sign(plane.xyz), plane.xyz ) > -plane.w;
		if ( visible ) return true;

		for (int i = 0; i < 4; ++i ) plane[i] = mat[i][3] - mat[i][2]; // far
		visible = dot(center.xyz + extent.xyz * sign(plane.xyz), plane.xyz ) > -plane.w;
		if ( visible ) return true;

*/
/*
	for ( uint p = 0; p < 8; ++p ) {
		vec4 t = corners[p];
		float w = abs(t.w);
		visible = -w <= t.x && t.x <= w && -w <= t.y && t.y <= w && 0 <= t.z && t.z <= w; // && -w <= t.z && t.z <= w;
	}
*/
/*
mat4 convert( mat4 proj ) {
	float f = -proj[1][1];
	float raidou = f / proj[0][0];
	float zNear = proj[3][2];
	float zFar = 32;
	
	float range = zNear - zFar;

	float Sx = f * raidou;
	float Sy = f;
	float Sz = (-zNear - zFar) / range;
	float Pz = 2 * zFar * zNear / range;

	mat4 new = mat4(1.0);
	new[0][0] = Sx;
	new[1][1] = -Sy;
	new[2][2] = Sz;
	new[3][2] = Pz;
	new[2][3] = 1;
	return new;
}
*/