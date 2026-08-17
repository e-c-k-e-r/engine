#version 450
#pragma shader_stage(geometry)
#extension GL_EXT_multiview : require

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout (location = 0) flat in uvec4 inId[];
layout (location = 1) flat in vec4 inPOS0[];
layout (location = 2) in vec4 inPOS1[];
layout (location = 3) in vec3 inPosition[];
layout (location = 4) in vec2 inUv[];
layout (location = 5) in vec4 inColor[];
layout (location = 6) in vec2 inSt[];
layout (location = 7) in vec3 inNormal[];
layout (location = 8) in vec3 inTangent[];

layout (location = 0) flat out uvec4 outId;
layout (location = 1) flat out vec4 outPOS0;
layout (location = 2) out vec4 outPOS1;
layout (location = 3) out vec3 outPosition;
layout (location = 4) out vec2 outUv;
layout (location = 5) out vec4 outColor;
layout (location = 6) out vec2 outSt;
layout (location = 7) out vec3 outNormal;
layout (location = 8) out vec3 outTangent;

#include "../../common/macros.h"
#include "../../common/structs.h"

layout (binding = 5) uniform UBO {
	float granularity;
	float occlusionFalloff;
	uint shadows;
	uint padding1;
} ubo;

layout (std140, binding = 6) readonly buffer RegionsBuffer {
	Region regions[];
};

#define USE_CROSS 0
void main(){
	const vec3 triMin = min(inPosition[0], min(inPosition[1], inPosition[2]));
	const vec3 triMax = max(inPosition[0], max(inPosition[1], inPosition[2]));

	uint emittedTriangles = 0;
	for (uint r = 0; r < regions.length(); ++r) {
		Region region = regions[r];
		if ( region.resolution == 0 ) continue;

		const float margin = (region.maxBounds.x - region.minBounds.x) / float(region.resolution) * 1.0;

		bool intersects = (triMin.x - margin <= region.maxBounds.x && triMax.x + margin >= region.minBounds.x) &&
						  (triMin.y - margin <= region.maxBounds.y && triMax.y + margin >= region.minBounds.y) &&
						  (triMin.z - margin <= region.maxBounds.z && triMax.z + margin >= region.minBounds.z);

		if ( !intersects ) continue;
		const float HALF_PIXEL = (region.maxBounds.x - region.minBounds.x) / float(region.resolution) * 0.5;
		const vec3 C = ( inPosition[0] + inPosition[1] + inPosition[2] ) / 3.0;

#if USE_CROSS
		const vec3 N = abs(cross(inPosition[2] - inPosition[0], inPosition[1] - inPosition[0]));
#else
		const vec3 N = abs(inNormal[0] + inNormal[1] + inNormal[2]);
		 uint A = N.y > N.x ? 1 : 0;
		 A = N.z > N[A] ? 2 : A;
#endif

		vec3 P[3];
		#pragma unroll 3
		for( uint i = 0; i < 3; ++i ){
			vec3 localPos = (inPosition[i] - region.minBounds) / (region.maxBounds - region.minBounds);
			P[i] = localPos * 2.0 - 1.0;
		}


		#pragma unroll 3
		for( uint i = 0; i < 3; ++i ){
			const vec3 D = normalize( inPosition[i] - C ) * HALF_PIXEL;
			vec3 projectedD = D / (region.maxBounds - region.minBounds) * 2.0;

			outPosition = inPosition[i] + D;
			outPOS0 = inPOS0[i];
			outPOS1 = inPOS1[i];
			outUv = inUv[i];
			outSt = inSt[i];
			outColor = inColor[i];
			outNormal = inNormal[i];
			outTangent = inTangent[i];
			outId = inId[i];
			outId.w = r;

			const vec3 finalP = P[i] + projectedD;
		#if USE_CROSS
			if ( N.z > N.x && N.z > N.y ) gl_Position = vec4(finalP.x, finalP.y, 0, 1);
			else if ( N.x > N.y && N.x > N.z ) gl_Position = vec4(finalP.y, finalP.z, 0, 1);
			else gl_Position = vec4(finalP.x, finalP.z, 0, 1);
		#else
			if ( A == 0 ) gl_Position = vec4(finalP.zy, 0, 1 );
			else if ( A == 1 ) gl_Position = vec4(finalP.xz, 0, 1 );
			else if ( A == 2 ) gl_Position = vec4(finalP.xy, 0, 1 );
		#endif
			EmitVertex();
		}
		EndPrimitive();

		emittedTriangles++;
		if ( emittedTriangles >= 4 ) break;
	}
}