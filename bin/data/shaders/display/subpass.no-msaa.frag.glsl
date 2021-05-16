#version 450
#pragma shader_stage(fragment)

#define MULTISAMPLING 0
#include "./subpass.h"

void main() {
	populateSurface();

	const vec3 ambient = ubo.ambient.rgb * surface.material.occlusion;
	surface.fragment.rgb += (0 <= surface.material.indexLightmap) ? (surface.material.albedo.rgb + ambient) : (surface.material.albedo.rgb * ambient);

	pbr();
	postProcess();
}