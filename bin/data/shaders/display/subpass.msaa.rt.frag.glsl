#version 450
#pragma shader_stage(fragment)

#define DEFERRED_SAMPLING 0
#define MULTISAMPLING 1
#define RAYTRACE 1
#include "./subpass.h"

void main() {
	resolveSurfaceFragment();

	postProcess();
}