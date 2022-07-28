#version 450
#pragma shader_stage(compute)

#define RT 0
#define VXGI 1
#define MULTISAMPLING 1
#include "./comp.h"

void main() {
	resolveSurfaceFragment();
	postProcess();
}