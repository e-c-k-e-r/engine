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

void main(){
	vec3 norm = abs(inNormal[0] + inNormal[1] + inNormal[2]);
 	uint axis = norm.y > norm.x ? 1 : 0;
 	axis = norm.z > norm[axis] ? 2 : axis;

	mat4 invOrtho = inverse( ubo.voxel );
 	vec3 extentMin = vec3( invOrtho * vec4(-1, -1, -1, 1 ) );
 	vec3 extentMid = vec3( invOrtho * vec4( 0,  0,  0, 1 ) );
 	vec3 extentMax = vec3( invOrtho * vec4( 1,  1,  1, 1 ) );
	vec3 extentSize = extentMax - extentMin;

	for( uint i = 0; i < 3; ++i ){
		outUv = inUv[i];
		outSt = inSt[i];
		outColor = inColor[i];
		outNormal = inNormal[i];
		outTBN = inTBN[i];
		outPosition = inPosition[i];
		outId = inId[i];

		outPosition = vec3( ubo.voxel * vec4( outPosition, 1 ) );
		
		gl_Position = vec4( outPosition.xyz, 1 );
		if ( axis == 0 ) gl_Position.xyz = gl_Position.zyx;
		else if ( axis == 1 ) gl_Position.xyz = gl_Position.xzy;
		
		// outPosition = (inPosition[i] - extentMin) / extentSize;
		// outPosition = fract(outPosition * 0.5 + 0.5);

		outPosition = outPosition * 0.5 + 0.5;
	//	gl_Position.xy = fract( gl_Position.xy * 0.5 + 0.5 ) * 2.0 - 1.0;
	//	gl_Position.xy = fract( gl_Position.xy * 0.5 + 0.5 ) * 2.0 - 1.0;
		gl_Position.z = 1;
	
		EmitVertex();
	}
    EndPrimitive();
}