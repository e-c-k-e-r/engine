#version 450
#pragma shader_stage(compute)

#define RT 1
#define VXGI 0
#define MULTISAMPLING 0
#include "./comp.h"

void main() {
	populateSurface();
	directLighting();
	postProcess();
}