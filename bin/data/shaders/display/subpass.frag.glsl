#version 450
#pragma shader_stage(fragment)

#define DEFERRED_SAMPLING 0
#define MULTISAMPLING 0
#include "./subpass.h"

void main() {
	populateSurface();
	directLighting();
	postProcess();
}