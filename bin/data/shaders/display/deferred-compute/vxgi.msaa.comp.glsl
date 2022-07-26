#version 450
#pragma shader_stage(compute)

#define VXGI 1
#define DEFERRED_SAMPLING 1
#define MULTISAMPLING 1
#define RT 0
#include "./comp.h"

void main() {
	resolveSurfaceFragment();
	postProcess();
}