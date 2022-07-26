#version 450
#pragma shader_stage(compute)

#define DEFERRED_SAMPLING 1
#define MULTISAMPLING 1
#include "./comp.h"

void main() {
	resolveSurfaceFragment();

	postProcess();
}