#version 450
#pragma shader_stage(compute)

#define VXGI 0
#define DEFERRED_SAMPLING 1
#define MULTISAMPLING 1
#define RT 1
#include "./comp.h"

void main() {
	populateSurface();
	directLighting();
	postProcess();
}