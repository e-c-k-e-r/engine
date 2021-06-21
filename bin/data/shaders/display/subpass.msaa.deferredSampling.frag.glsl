#version 450
#pragma shader_stage(fragment)

#define DEFERRED_SAMPLING 1
#include "./subpass.h"

void main() {
	populateSurface();
	directLighting();
	postProcess();
}