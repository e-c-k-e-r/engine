#version 450
#pragma shader_stage(fragment)

#define MULTISAMPLING 0
#include "./subpass.h"

void main() {
	populateSurface();
	directLighting();
	postProcess();
}