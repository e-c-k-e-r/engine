#version 450
#pragma shader_stage(compute)

#define RT 1
#define VXGI 1
#define MULTISAMPLING 1
#include "./comp.h"

void main() {
	populateSurface();
	directLighting();
	postProcess();
}