#version 450
#pragma shader_stage(geometry)

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout (location = 0) in vec2 inUv[];
layout (location = 1) in vec2 inSt[];
layout (location = 2) in vec4 inColor[];
layout (location = 3) in vec3 inNormal[];
layout (location = 4) in mat3 inTBN[];
layout (location = 7) in vec3 inPosition[];
layout (location = 8) flat in uvec4 inId[];
layout (location = 9) flat in uint inLayer[];

layout (location = 0) out vec2 outUv;
layout (location = 1) out vec2 outSt;
layout (location = 2) out vec4 outColor;
layout (location = 3) out vec3 outNormal;
layout (location = 4) out mat3 outTBN;
layout (location = 7) out vec3 outPosition;
layout (location = 8) flat out uvec4 outId;
layout (location = 9) flat out uint outLayer;

void main(){
 	for( uint i = 0; i < 3; ++i ){
		outUv = inUv[i];
		outSt = inSt[i];
		outColor = inColor[i];
		outNormal = inNormal[i];
		outTBN = inTBN[i];
		outPosition = inPosition[i];
		outId = inId[i];
		outLayer = inLayer[i];
		
		gl_Position = vec4(inSt[i] * 2.0 - 1.0, 0.0, 1.0);
		gl_Layer = int(inLayer[i]);

		EmitVertex();
	}
    EndPrimitive();
}