#version 450
#pragma shader_stage(fragment)

#define VXGI 0
#define DEFERRED_SAMPLING 1
#define MULTISAMPLING 0
#include "./subpass.h"

void main() {
	populateSurface();
	directLighting();
	postProcess();
}