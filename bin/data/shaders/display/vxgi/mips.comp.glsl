#version 450
#pragma shader_stage(compute)

#extension GL_KHR_shader_subgroup_quad : require
#extension GL_KHR_shader_subgroup_arithmetic : require
#extension GL_EXT_samplerless_texture_functions : enable

#define COMPUTE 1
#define SPD 1

#include "../../common/macros.h"
#include "../../common/structs.h"
#include "../../common/functions.h"

layout (local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout (constant_id = 0) const uint CASCADES = 8;
layout (constant_id = 1) const uint MIPS = 9; // 256^3 texture = 9 mips

layout(push_constant) uniform PushBlock {
	uint mips;
	uint cascade;
	uint numWorkGroups;
	uint workGroupOffset;
} PushConstant_;

layout (binding = 0) uniform sampler3D voxelRadiance[CASCADES];
layout (binding = 1, rgba8) coherent uniform image3D voxelMips[CASCADES * (MIPS - 1)];

layout (binding = 2, std430) buffer AtomicCounter {
	uint counter;
} spdCounter;


// 8^3 = 512 threads
shared vec4 s_colorAlpha[512];
shared uint s_isLastWG;

vec4 reduce8(vec4 v[8]) {
	vec3 color = vec3(0.0);
	float alpha = 0.0;

	for( int i = 0; i < 8; ++i ) {
		float a = float(uint(v[i].a * 255.0 + 0.5) & 0xF) / 15.0;
		color += v[i].rgb * a;
		alpha += a;
	}

	if ( alpha > 0.001 ) color /= alpha;
	alpha /= 8.0;

	uint lum4 = uint(clamp(luma(color), 0.0, 1.0) * 15.0) & 0xF;
	uint alpha4 = uint(clamp(alpha, 0.0, 1.0) * 15.0) & 0xF;
	return vec4(color, float((lum4 << 4) | alpha4) / 255.0);
}

ivec3 index3D(uint idx, uint sizeX, uint sizeY) {
	return ivec3( idx % sizeX, (idx / sizeX) % sizeY, idx / (sizeX * sizeY) );
}

vec4 reduceFromShared( uint lid, uint dst ) {
	ivec3 pos = index3D(lid, dst, dst) * 2;
	vec4 v[8];
	uint src = dst * 2;
	for ( int z = 0; z < 2; ++z ) {
		for ( int y = 0; y < 2; ++y ) {
			for ( int x = 0; x < 2; ++x ) {
				ivec3 p = pos + ivec3(x,y,z);
				uint flatIdx = p.x + p.y * src + p.z * src * src;
				v[x + y*2 + z*4] = s_colorAlpha[flatIdx];
			}
		}
	}
	return reduce8(v);
}

void main() {
	uint lid = gl_LocalInvocationIndex;
	ivec3 wgID = ivec3(gl_WorkGroupID);

	// mip 0 => 1
	if ( 1 < PushConstant_.mips ) {
		ivec3 gid = wgID * 8 + index3D(lid, 8, 8);
		ivec3 pos = gid * 2;

		vec4 v[8];
		v[0] = texelFetch(voxelRadiance[PushConstant_.cascade], pos + ivec3(0,0,0), 0);
		v[1] = texelFetch(voxelRadiance[PushConstant_.cascade], pos + ivec3(1,0,0), 0);
		v[2] = texelFetch(voxelRadiance[PushConstant_.cascade], pos + ivec3(0,1,0), 0);
		v[3] = texelFetch(voxelRadiance[PushConstant_.cascade], pos + ivec3(1,1,0), 0);
		v[4] = texelFetch(voxelRadiance[PushConstant_.cascade], pos + ivec3(0,0,1), 0);
		v[5] = texelFetch(voxelRadiance[PushConstant_.cascade], pos + ivec3(1,0,1), 0);
		v[6] = texelFetch(voxelRadiance[PushConstant_.cascade], pos + ivec3(0,1,1), 0);
		v[7] = texelFetch(voxelRadiance[PushConstant_.cascade], pos + ivec3(1,1,1), 0);

		vec4 color = reduce8(v);
		imageStore(voxelMips[PushConstant_.cascade + PushConstant_.mips * 0], gid, color);
		s_colorAlpha[lid] = color;
	}
	memoryBarrierShared(); barrier();

	// mip 1 => 2 => 3 => 4
	int threads[3] = { 64, 8, 1 };
	int edges[3]   = { 4, 2, 1 };
	
	{
		for ( int i = 0; i < 3; ++i ) {
			uint mipLevel = i + 2;
			if ( mipLevel < PushConstant_.mips ) {
				if ( lid < threads[i] ) {
					vec4 color = reduceFromShared(lid, edges[i]);
					ivec3 pos = wgID * edges[i] + index3D(lid, edges[i], edges[i]);
					imageStore(voxelMips[PushConstant_.cascade + PushConstant_.mips * (mipLevel - 1)], pos, color);
					s_colorAlpha[lid] = color;
				}
			}
			memoryBarrierShared(); barrier();
		}
	}
	if ( PushConstant_.mips <= 5 ) return; // bail if 16^3 or smaller
	// atomic sync
	
	{
		if ( lid == 0 ) {
			uint ticket = atomicAdd(spdCounter.counter, 1);
			s_isLastWG = (ticket == PushConstant_.numWorkGroups - 1) ? 1 : 0;
		}
		memoryBarrierShared(); barrier();
		if ( s_isLastWG == 0 ) return; // use last workgroup
		if ( lid == 0 ) spdCounter.counter = 0; // reset for next frame
	}
	// mip 4 => 5
	if ( 5 < PushConstant_.mips ) {
		ivec3 pos = index3D(lid, 8, 8) * 2;
		uint m4_idx = PushConstant_.cascade + PushConstant_.mips * 3;
		vec4 v[8];
		v[0] = imageLoad(voxelMips[m4_idx], pos + ivec3(0,0,0));
		v[1] = imageLoad(voxelMips[m4_idx], pos + ivec3(1,0,0));
		v[2] = imageLoad(voxelMips[m4_idx], pos + ivec3(0,1,0));
		v[3] = imageLoad(voxelMips[m4_idx], pos + ivec3(1,1,0));
		v[4] = imageLoad(voxelMips[m4_idx], pos + ivec3(0,0,1));
		v[5] = imageLoad(voxelMips[m4_idx], pos + ivec3(1,0,1));
		v[6] = imageLoad(voxelMips[m4_idx], pos + ivec3(0,1,1));
		v[7] = imageLoad(voxelMips[m4_idx], pos + ivec3(1,1,1));

		vec4 color = reduce8(v);
		imageStore(voxelMips[PushConstant_.cascade + PushConstant_.mips * 4], pos / 2, color);
		s_colorAlpha[lid] = color;
	}
	memoryBarrierShared(); barrier();

	// mip 5 => 6 => 7 => 8
	for ( int i = 0; i < 3; ++i ) {
		uint mipLevel = i + 6; // Mips 6, 7, 8
		if ( mipLevel < PushConstant_.mips ) {
			if ( lid < threads[i] ) {
				vec4 color = reduceFromShared(lid, edges[i]);
				ivec3 pos = index3D(lid, edges[i], edges[i]);
				imageStore(voxelMips[PushConstant_.cascade + PushConstant_.mips * (mipLevel - 1)], pos, color);
				s_colorAlpha[lid] = color;
			}
		}
		memoryBarrierShared(); barrier();
	}
}