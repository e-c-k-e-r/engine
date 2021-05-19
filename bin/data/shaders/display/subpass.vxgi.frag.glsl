#version 450
#pragma shader_stage(fragment)

#define VXGI 1
#include "./subpass.h"

void main() {
	populateSurface();
	indirectLighting();
	directLighting();
	postProcess();
}