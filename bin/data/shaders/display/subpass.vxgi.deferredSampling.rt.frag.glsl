#version 450
#pragma shader_stage(fragment)

#define VXGI 1
#define RAYTRACE 1
#define MULTISAMPLING 0
#define DEFERRED_SAMPLING 1
#include "./subpass.h"

void main() {
	populateSurface();
	indirectLighting();
	directLighting();
	postProcess();
}