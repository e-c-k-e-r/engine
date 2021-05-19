#version 450
#pragma shader_stage(fragment)

#include "./subpass.h"

void main() {
	populateSurface();
	directLighting();
	postProcess();
}