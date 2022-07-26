#version 450
#pragma shader_stage(geometry)

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout (location = 0) in vec3 inPosition[];
layout (location = 1) flat in vec4 inPOS0[];
layout (location = 2) in vec4 inPOS1[];
layout (location = 3) in vec2 inUv[];
layout (location = 4) in vec2 inSt[];
layout (location = 5) in vec4 inColor[];
layout (location = 6) in vec3 inNormal[];
layout (location = 7) in vec3 inTangent[];
layout (location = 8) flat in uvec4 inId[];

layout (location = 0) out vec3 outPosition;
layout (location = 1) flat out vec4 outPOS0;
layout (location = 2) out vec4 outPOS1;
layout (location = 3) out vec2 outUv;
layout (location = 4) out vec2 outSt;
layout (location = 5) out vec4 outColor;
layout (location = 6) out vec3 outNormal;
layout (location = 7) out vec3 outTangent;
layout (location = 8) flat out uvec4 outId;

layout (binding = 4) uniform UBO {
	mat4 voxel;

	float cascadePower;
	float granularity;
	float voxelizeScale;
	float occlusionFalloff;

	uint shadows;
	uint padding1;
	uint padding2;
	uint padding3;
} ubo;

float cascadePower( uint x ) {
	return pow(1 + x, ubo.cascadePower);
//	return max( 1, x * ubo.cascadePower );
}

#define USE_CROSS 0
void main(){
 	const float HALF_PIXEL = ubo.voxelizeScale;
 	const vec3 C = ( inPosition[0] + inPosition[1] + inPosition[2] ) / 3.0;

#if USE_CROSS
	const vec3 N = abs(cross(inPosition[2] - inPosition[0], inPosition[1] - inPosition[0]));
#else
	const vec3 N = abs(inNormal[0] + inNormal[1] + inNormal[2]);
 	uint A = N.y > N.x ? 1 : 0;
 	A = N.z > N[A] ? 2 : A;
#endif
	
	const uint CASCADE = inId[0].w;
	vec3 P[3] = {
		vec3( ubo.voxel * vec4( inPosition[0], 1 ) ) / cascadePower(CASCADE),
		vec3( ubo.voxel * vec4( inPosition[1], 1 ) ) / cascadePower(CASCADE),
		vec3( ubo.voxel * vec4( inPosition[2], 1 ) ) / cascadePower(CASCADE),
	};

 	for( uint i = 0; i < 3; ++i ){
 		const vec3 D = normalize( inPosition[i] - C ) * HALF_PIXEL;

		outPosition = P[i] + D;
		outPOS0 = inPOS0[i];
		outPOS1 = inPOS1[i];
		outUv = inUv[i];
		outSt = inSt[i];
		outColor = inColor[i];
		outNormal = inNormal[i];
		outTangent = inTangent[i];
		outId = inId[i];

		const vec3 P = outPosition; // + D;
	#if USE_CROSS
		if ( N.z > N.x && N.z > N.y ) gl_Position = vec4(P.x, P.y, 0, 1);
		else if ( N.x > N.y && N.x > N.z ) gl_Position = vec4(P.y, P.z, 0, 1);
		else gl_Position = vec4(P.x, P.z, 0, 1);
	#else
		if ( A == 0 ) gl_Position = vec4(P.zy, 0, 1 );
		else if ( A == 1 ) gl_Position = vec4(P.xz, 0, 1 );
		else if ( A == 2 ) gl_Position = vec4(P.xy, 0, 1 );
	#endif
		EmitVertex();
	}
    EndPrimitive();
}