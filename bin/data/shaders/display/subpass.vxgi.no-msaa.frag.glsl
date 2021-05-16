#version 450
#pragma shader_stage(fragment)

#define VXGI 1
#define MULTISAMPLING 0
#include "./subpass.h"

void main() {
	populateSurface();
	indirectLighting();

	const vec3 ambient = ubo.ambient.rgb * surface.material.occlusion + surface.material.indirect.rgb;
	surface.fragment.rgb += (0 <= surface.material.indexLightmap) ? (surface.material.albedo.rgb + ambient) : (surface.material.albedo.rgb * ambient);

	pbr();
	postProcess();
}