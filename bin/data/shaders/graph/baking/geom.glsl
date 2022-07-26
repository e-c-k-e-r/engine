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
layout (location = 9) flat in uint inLayer[];

layout (location = 0) out vec3 outPosition;
layout (location = 1) flat out vec4 outPOS0;
layout (location = 2) out vec4 outPOS1;
layout (location = 3) out vec2 outUv;
layout (location = 4) out vec2 outSt;
layout (location = 5) out vec4 outColor;
layout (location = 6) out vec3 outNormal;
layout (location = 7) out vec3 outTangent;
layout (location = 8) flat out uvec4 outId;
layout (location = 9) flat out uint outLayer;


void main(){
 	for( uint i = 0; i < 3; ++i ){
		outPosition = inPosition[i];
		outPOS0 = inPOS0[i];
		outPOS1 = inPOS1[i];
		outUv = inUv[i];
		outSt = inSt[i];
		outColor = inColor[i];
		outNormal = inNormal[i];
		outTangent = inTangent[i];
		outId = inId[i];
		outLayer = inLayer[i];
		
		gl_Position = vec4(inSt[i] * 2.0 - 1.0, 0.0, 1.0);
		gl_Layer = int(inLayer[i]);

		EmitVertex();
	}
    EndPrimitive();
}