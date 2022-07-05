#version 450
#pragma shader_stage(fragment)

#define DEFERRED_SAMPLING 1
#define MULTISAMPLING 0
#define VXGI 0
#define RT 1

#include "./subpass.h"

void main() {
	populateSurface();
	directLighting();
	postProcess();
}