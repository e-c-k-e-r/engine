#version 450
#pragma shader_stage(fragment)

#define VXGI 1
#define DEFERRED_SAMPLING 1
#define MULTISAMPLING 1
#include "./subpass.h"

void main() {
	resolveSurfaceFragment();
	postProcess();
}