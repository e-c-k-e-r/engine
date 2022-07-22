#version 450
#pragma shader_stage(compute)

//#extension GL_EXT_nonuniform_qualifier : enable

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

layout (binding = 3) uniform sampler2D samplerID;
layout (binding = 4) uniform sampler2D samplerDepth;

bool frustumCull( uint id ) {
	if ( PushConstant.passes == 0 ) return true;
	
	const DrawCommand drawCommand = drawCommands[id];
	const Instance instance = instances[drawCommand.instanceID];

	bool visible = true;
	for ( uint pass = 0; pass < PushConstant.passes; ++pass ) {
		mat4 mat = camera.viewport[pass].projection * camera.viewport[pass].view * instance.model;
	}
	return visible;
}

void main() {
	const uint gID = gl_GlobalInvocationID.x;
	if ( !(0 <= gID && gID < drawCommands.length()) ) return;
	if ( drawCommands[gID].instances == 0 ) return;
//	drawCommands[gID].instances = frustumCull( gID ) ? 1 : 0;
}