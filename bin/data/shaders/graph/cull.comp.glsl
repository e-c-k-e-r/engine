#version 450
#pragma shader_stage(compute)

#extension GL_EXT_nonuniform_qualifier : enable

layout (constant_id = 0) const uint PASSES = 6;
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

#define COMPUTE 1

#include "../common/macros.h"
#include "../common/structs.h"

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

struct Frustum {
	vec4 planes[6];
};

float inside( vec3 v, vec3 min, vec3 max ) {
	vec3 s = step(min, v) - step(max, v);
	return s.x * s.y * s.z; 
}
/*
	T f = 1 / tan( fov / 2 );
	return pod::Matrix4t<T>({
		f / raidou, 	0, 	 	0, 		0,
		0, 				-f, 	0, 		0,
		0,       		0,    	0, 		1,
		0,       		0,   znear, 	0
	});
*/

vec4 normalizePlane( vec4 p ) {
	return p / length(p.xyz);
}

bool frustumCull( uint id ) {
	if ( PushConstant.passes == 0 ) return true;
	
	const DrawCommand drawCommand = drawCommands[id];
	const Instance instance = instances[drawCommand.instanceID];

	bool visible = false;
	for ( uint pass = 0; pass < PushConstant.passes; ++pass ) {
	// return if our camera position is inside the AABB
	vec3 camPos = vec3( inverse(camera.viewport[pass].view)[3] );
	if ( instance.bounds.min.x <= camPos.x && camPos.x <= instance.bounds.max.x && instance.bounds.min.y <= camPos.y && camPos.y <= instance.bounds.max.y && instance.bounds.min.z <= camPos.z && camPos.z <= instance.bounds.max.z ) return true;
	// sphere based one, source of this block uses reverse infinite Z, but would be nice if it used AABB
	#if 0
		vec3 min = vec3(camera.viewport[pass].view * instance.model * vec4(instance.bounds.min, 1));
		vec3 max = vec3(camera.viewport[pass].view * instance.model * vec4(instance.bounds.max, 1));
		
		vec3 center = (max + min) * 0.5;
		vec3 extent = (max - min) * 0.5;
		float radius = length(extent);

		mat4 projectionT = transpose(camera.viewport[pass].projection);
		vec4 frustumX = normalizePlane(projectionT[3] + projectionT[0]); // x + w < 0
		vec4 frustumY = normalizePlane(projectionT[3] + projectionT[1]); // y + w < 0
		vec4 frustum = vec4( frustumX.x, frustumX.z, frustumY.y, frustumY.z );

		visible = visible && center.z * frustum[1] - abs(center.x) * frustum[0] > -radius;
		visible = visible && center.z * frustum[3] - abs(center.y) * frustum[2] > -radius;
	// optimized version of the below blocks
	#elif 1
		mat4 mat = camera.viewport[pass].projection * camera.viewport[pass].view * instance.model;
		vec3 min = vec3(mat * vec4(instance.bounds.min, 1));
		vec3 max = vec3(mat * vec4(instance.bounds.max, 1));

		vec3 center = (max + min) * 0.5;
		vec3 extent = (max - min) * 0.5;
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
			if ( dot(center + extent * sign(planes[p].xyz), planes[p].xyz ) > -planes[p].w ) return true;
		}
	// transforms each corner into clip space
	// an AABB is not visible if a plane has all 8 corners outside of it
	#elif 0
		mat4 mat = camera.viewport[pass].projection * camera.viewport[pass].view * instance.model;
		vec4 corners[9] = {
			vec4( instance.bounds.min.x, instance.bounds.min.y, instance.bounds.min.z, 1.0 ),
			vec4( instance.bounds.max.x, instance.bounds.min.y, instance.bounds.min.z, 1.0 ),
			vec4( instance.bounds.max.x, instance.bounds.max.y, instance.bounds.min.z, 1.0 ),
			vec4( instance.bounds.min.x, instance.bounds.max.y, instance.bounds.min.z, 1.0 ),

			vec4( instance.bounds.min.x, instance.bounds.min.y, instance.bounds.max.z, 1.0 ),
			vec4( instance.bounds.max.x, instance.bounds.min.y, instance.bounds.max.z, 1.0 ),
			vec4( instance.bounds.max.x, instance.bounds.max.y, instance.bounds.max.z, 1.0 ),
			vec4( instance.bounds.min.x, instance.bounds.max.y, instance.bounds.max.z, 1.0 ),

			vec4( (instance.bounds.max + instance.bounds.min) * 0.5, 1.0 ),
		};
		for ( uint p = 0; p < 9; ++p ) {
			vec4 t = mat * corners[p];
			if ( -t.w <= t.x && t.x <= t.w && -t.w <= t.y && t.y <= t.w ) return true;
		}
	// "optimized" version of the next block, compares bounds to each plane
	// an AABB is not visible if a plane has all 8 corners outside of it
	#elif 0
		mat4 mat = camera.viewport[pass].projection * camera.viewport[pass].view * instance.model;
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
		mat4 mat = camera.viewport[pass].projection * camera.viewport[pass].view * instance.model;
		vec4 corners[9] = {
			vec4( instance.bounds.min.x, instance.bounds.min.y, instance.bounds.min.z, 1.0 ),
			vec4( instance.bounds.max.x, instance.bounds.min.y, instance.bounds.min.z, 1.0 ),
			vec4( instance.bounds.max.x, instance.bounds.max.y, instance.bounds.min.z, 1.0 ),
			vec4( instance.bounds.min.x, instance.bounds.max.y, instance.bounds.min.z, 1.0 ),

			vec4( instance.bounds.min.x, instance.bounds.min.y, instance.bounds.max.z, 1.0 ),
			vec4( instance.bounds.max.x, instance.bounds.min.y, instance.bounds.max.z, 1.0 ),
			vec4( instance.bounds.max.x, instance.bounds.max.y, instance.bounds.max.z, 1.0 ),
			vec4( instance.bounds.min.x, instance.bounds.max.y, instance.bounds.max.z, 1.0 ),

			vec4( (instance.bounds.max + instance.bounds.min) * 0.5, 1.0 ),
		};
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
		for ( uint p = 0; p < 9; ++p ) corners[p] = mat * corners[p];
		for ( uint p = 0; p < 6; ++p ) {
			for ( uint q = 0; q < 9; ++q ) {
				if ( dot( corners[q], planes[p] ) > 0 ) return true;
			}
			return false;
		}
#endif
	}
	return visible;
}

void main() {
	const uint gID = gl_GlobalInvocationID.x;
	if ( !(0 <= gID && gID < drawCommands.length()) ) return;
	drawCommands[gID].instances = frustumCull( gID ) ? 1 : 0;
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