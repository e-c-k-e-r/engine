#version 450

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout (location = 0) in vec2 inUv[];
layout (location = 1) in vec2 inSt[];
layout (location = 2) in vec4 inColor[];
layout (location = 3) in vec3 inNormal[];
layout (location = 4) in mat3 inTBN[];
layout (location = 7) in vec3 inPosition[];
layout (location = 8) flat in ivec4 inId[];

layout (location = 0) out vec2 outUv;
layout (location = 1) out vec2 outSt;
layout (location = 2) out vec4 outColor;
layout (location = 3) out vec3 outNormal;
layout (location = 4) out mat3 outTBN;
layout (location = 7) out vec3 outPosition;
layout (location = 8) flat out ivec4 outId;

layout (binding = 6) uniform UBO {
	mat4 voxel;
} ubo;

#define CASCADE_POWER 2
uint SCALE_CASCADE( uint x ) {
	return uint(pow(1 + x, CASCADE_POWER));
}

void main(){
	const float RENDER_RESOLUTION = 256.0;
	const float PIXEL_SCALE = 2.0;
 	const float HALF_PIXEL = 1.0 / (RENDER_RESOLUTION * PIXEL_SCALE);
 	const vec3 C = ( inPosition[0] + inPosition[1] + inPosition[2] ) / 3.0;

#if USE_CROSS
	const vec3 N = abs(cross(inPosition[2] - inPosition[0], inPosition[1] - inPosition[0]));
#else
	const vec3 N = abs(inNormal[0] + inNormal[1] + inNormal[2]);
 	uint A = N.y > N.x ? 1 : 0;
 	A = N.z > N[A] ? 2 : A;
#endif
	
	const uint CASCADE = inId[0].z;
	vec3 P[3] = {
		vec3( ubo.voxel * vec4( inPosition[0], 1 ) ) / SCALE_CASCADE(CASCADE),
		vec3( ubo.voxel * vec4( inPosition[1], 1 ) ) / SCALE_CASCADE(CASCADE),
		vec3( ubo.voxel * vec4( inPosition[2], 1 ) ) / SCALE_CASCADE(CASCADE),
	};


 	for( uint i = 0; i < 3; ++i ){
 		const vec3 D = normalize( inPosition[i] - C ) * HALF_PIXEL;

		outUv = inUv[i];
		outSt = inSt[i];
		outColor = inColor[i];
		outNormal = inNormal[i];
		outTBN = inTBN[i];
		outPosition = P[i]; // + D;
		outId = inId[i];

		const vec3 P = outPosition + D;
	#if USE_CROSS
		if ( N.z > N.x && N.z > N.y ) gl_Position = vec4(P.x, P.y, 0, 1);
		else if ( N.x > N.y && N.x > N.z ) gl_Position = vec4(P.y, P.z, 0, 1);
		else gl_Position = vec4(P.x, P.z, 0, 1);
	#else
		if ( A == 0 ) gl_Position = vec4(P.zy, 1, 1 );
		else if ( A == 1 ) gl_Position = vec4(P.xz, 1, 1 );
		else if ( A == 2 ) gl_Position = vec4(P.xy, 1, 1 );
	#endif
		EmitVertex();
	}
    EndPrimitive();
}