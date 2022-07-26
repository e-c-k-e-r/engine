#version 450
#pragma shader_stage(compute)

#define VXGI 1
#define DEFERRED_SAMPLING 1
#define MULTISAMPLING 0
#define RT 1
#include "./comp.h"

void main() {
	populateSurface();
	directLighting();
	postProcess();
}