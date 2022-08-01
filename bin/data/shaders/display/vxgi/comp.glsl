#version 450
#pragma shader_stage(compute)

#extension GL_EXT_samplerless_texture_functions : require
#extension GL_EXT_nonuniform_qualifier : enable

layout (local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

#define COMPUTE 1

#define VXGI 1
#define MAX_CUBEMAPS CUBEMAPS
#define GAMMA_CORRECT 1

layout (constant_id = 0) const uint TEXTURES = 512;
layout (constant_id = 1) const uint CUBEMAPS = 128;
layout (constant_id = 2) const uint CASCADES = 16;

#include "../../common/macros.h"
#include "../../common/structs.h"

layout (binding = 0) uniform UBO {
	EyeMatrices matrices[2];

	Settings settings;
} ubo;
layout (std140, binding = 1) readonly buffer DrawCommands {
	DrawCommand drawCommands[];
};
layout (std140, binding = 2) readonly buffer Instances {
	Instance instances[];
};
layout (std140, binding = 3) readonly buffer InstanceAddresseses {
	InstanceAddresses instanceAddresses[];
};
layout (std140, binding = 4) readonly buffer Materials {
	Material materials[];
};
layout (std140, binding = 5) readonly buffer Textures {
	Texture textures[];
};
layout (std140, binding = 6) readonly buffer Lights {
	Light lights[];
};

layout (binding = 7) uniform sampler2D samplerTextures[TEXTURES];
layout (binding = 8) uniform samplerCube samplerCubemaps[CUBEMAPS];
layout (binding = 9) uniform sampler3D samplerNoise;

layout (binding = 10, rg16ui) uniform volatile coherent uimage3D voxelId[CASCADES];
layout (binding = 11, rg16f) uniform volatile coherent image3D voxelNormal[CASCADES];
#if VXGI_HDR
	layout (binding = 12, rgba32f) uniform volatile coherent image3D voxelRadiance[CASCADES];
#else
	layout (binding = 12, rgba16f) uniform volatile coherent image3D voxelRadiance[CASCADES];
#endif

#include "../../common/functions.h"
#include "../../common/light.h"
#undef VXGI
#include "../../common/shadows.h"
#define PBR 1
void main() {
	const vec3 tUvw = gl_GlobalInvocationID.xzy;
	for ( uint CASCADE = 0; CASCADE < CASCADES; ++CASCADE ) {
		surface.normal.world = decodeNormals( vec2(imageLoad(voxelNormal[CASCADE], ivec3(tUvw) ).xy) );
		surface.normal.eye = vec3( ubo.settings.vxgi.matrix * vec4( surface.normal.world, 0.0f ) );
	
		surface.position.eye = (vec3(gl_GlobalInvocationID.xyz) / vec3(imageSize(voxelRadiance[CASCADE])) * 2.0f - 1.0f) * cascadePower(CASCADE);
		surface.position.world = vec3( inverse(ubo.settings.vxgi.matrix) * vec4( surface.position.eye, 1.0f ) );
		
		const uvec2 ID = uvec2(imageLoad(voxelId[CASCADE], ivec3(tUvw) ).xy);
		const uint drawID = ID.x - 1;
		const uint instanceID = ID.y - 1;
		if ( ID.x == 0 || ID.y == 0 ) {
			imageStore(voxelRadiance[CASCADE], ivec3(tUvw), vec4(0));
			continue;
		}
		const DrawCommand drawCommand = drawCommands[drawID];
		surface.instance = instances[instanceID];
		const Material material = materials[surface.instance.materialID];
		surface.material.albedo = material.colorBase;
		surface.fragment = material.colorEmissive;

		surface.material.albedo = imageLoad(voxelRadiance[CASCADE], ivec3(tUvw) );
		surface.material.metallic = material.factorMetallic;
		surface.material.roughness = material.factorRoughness;
		surface.material.occlusion = material.factorOcclusion;

		const vec3 ambient = ubo.settings.lighting.ambient.rgb * surface.material.occlusion;
		if ( validTextureIndex( surface.instance.lightmapID ) ) {
			surface.fragment.rgb += surface.material.albedo.rgb;
		} else {
			surface.fragment.rgb += surface.material.albedo.rgb * ambient;
			// corrections
			surface.material.roughness *= 4.0;
			const vec3 F0 = mix(vec3(0.04), surface.material.albedo.rgb, surface.material.metallic); 
			const vec3 Lo = normalize( surface.position.world );
			const float cosLo = max(0.0, dot(surface.normal.world, Lo));
			for ( uint i = 0; i < ubo.settings.lengths.lights; ++i ) {
				const Light light = lights[i];
				if ( light.power <= LIGHT_POWER_CUTOFF ) continue;
				if ( light.type >= 0 && validTextureIndex( surface.instance.lightmapID ) ) continue;
				const vec3 Lp = light.position;
				const vec3 Liu = light.position - surface.position.world;
				const vec3 Li = normalize(Liu);
				const float Ls = shadowFactor( light, 0.0 );
				const float La = 1.0 / (1 + (PI * pow(length(Liu), 2.0)));
				if ( light.power * La * Ls <= LIGHT_POWER_CUTOFF ) continue;

				const float cosLi = max(0.0, dot(surface.normal.world, Li));
				const vec3 Lr = light.color.rgb * light.power * La * Ls;
			#if LAMBERT
				const vec3 diffuse = surface.material.albedo.rgb;
				const vec3 specular = vec3(0);
			#elif PBR
				const vec3 Lh = normalize(Li + Lo);
				const float cosLh = max(0.0, dot(surface.normal.world, Lh));
				
				const vec3 F = fresnelSchlick( F0, max( 0.0, dot(Lh, Lo) ) );
				const float D = ndfGGX( cosLh, surface.material.roughness );
				const float G = gaSchlickGGX(cosLi, cosLo, surface.material.roughness);
				const vec3 diffuse = mix( vec3(1.0) - F, vec3(0.0), surface.material.metallic ) * surface.material.albedo.rgb;
				const vec3 specular = (F * D * G) / max(EPSILON, 4.0 * cosLi * cosLo);
			#endif
				// lightmapped, compute only specular
				surface.light.rgb += (diffuse + specular) * Lr * cosLi;
				surface.light.a += light.power * La * Ls;
			}
		}

		surface.fragment.rgb += surface.light.rgb;

	#if TONE_MAP
		toneMap(surface.fragment.rgb, ubo.settings.bloom.exposure);
	#endif
	#if GAMMA_CORRECT
		gammaCorrect(surface.fragment.rgb, 1.0 / ubo.settings.bloom.gamma);
	#endif

		imageStore(voxelRadiance[CASCADE], ivec3(tUvw), vec4(surface.fragment.rgb, surface.material.albedo.a));
	}
}