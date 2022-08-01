#version 460
#pragma shader_stage(compute)

#define RT 1
#define VXGI 1
#define MULTISAMPLING 0
#include "./comp.h"

void main() {
	populateSurface();
	indirectLighting();
	directLighting();
	postProcess();
}