#version 450
#pragma shader_stage(fragment)

#define MULTISAMPLING 1
#define DEFERRED_SAMPLING 1

#include "./subpass.h"

void main() {
	populateSurface();
	directLighting();
	postProcess();
}