#extension GL_EXT_samplerless_texture_functions : require
#extension GL_EXT_nonuniform_qualifier : enable

layout (local_size_x = 8, local_size_y = 8, local_size_z = 8) in;


#define LAMBERT 1
#define PBR 0
#define VXGI 1
#define COMPUTE 1

layout (constant_id = 0) const uint TEXTURES = 512;
layout (constant_id = 1) const uint CUBEMAPS = 128;
layout (constant_id = 2) const uint CASCADES = 16;

#include "../common/macros.h"
#include "../common/structs.h"

layout (binding = 4) uniform UBO {
	EyeMatrices matrices[2];

	Mode mode;
	Fog fog;
	Vxgi vxgi;

	uint lights;
	uint materials;
	uint textures;
	uint drawCommands;
	
	vec3 ambient;
	float gamma;

	float exposure;
	float brightnessThreshold;
	uint msaa;
	uint shadowSamples;

	uint indexSkybox;
	uint padding1;
	uint padding2;
	uint padding3;
} ubo;
/*
layout (std140, binding = 5) readonly buffer DrawCommands {
	DrawCommand drawCommands[];
};
*/
layout (std140, binding = 5) readonly buffer Instances {
	Instance instances[];
};
layout (std140, binding = 6) readonly buffer Materials {
	Material materials[];
};
layout (std140, binding = 7) readonly buffer Textures {
	Texture textures[];
};
layout (std140, binding = 8) readonly buffer Lights {
	Light lights[];
};

layout (binding = 9) uniform sampler2D samplerTextures[TEXTURES];
layout (binding = 10) uniform samplerCube samplerCubemaps[CUBEMAPS];
layout (binding = 11) uniform sampler3D samplerNoise;

layout (binding = 12, rg16ui) uniform volatile coherent uimage3D voxelId[CASCADES];
layout (binding = 13, rg16f) uniform volatile coherent image3D voxelNormal[CASCADES];
#if VXGI_HDR
	layout (binding = 14, rgba8) uniform volatile coherent image3D voxelRadiance[CASCADES];
#else
	layout (binding = 14, rgba16f) uniform volatile coherent image3D voxelRadiance[CASCADES];
#endif

#include "../common/functions.h"
#undef VXGI
#include "../common/shadows.h"

void main() {
	const vec3 tUvw = gl_GlobalInvocationID.xzy;
	for ( uint CASCADE = 0; CASCADE < CASCADES; ++CASCADE ) {
		surface.normal.world = decodeNormals( vec2(imageLoad(voxelNormal[CASCADE], ivec3(tUvw) ).xy) );
		surface.normal.eye = vec3( ubo.vxgi.matrix * vec4( surface.normal.world, 0.0f ) );
	
		surface.position.eye = (vec3(gl_GlobalInvocationID.xyz) / vec3(imageSize(voxelRadiance[CASCADE])) * 2.0f - 1.0f) * cascadePower(CASCADE);
		surface.position.world = vec3( inverse(ubo.vxgi.matrix) * vec4( surface.position.eye, 1.0f ) );
		
		const uvec2 ID = uvec2(imageLoad(voxelId[CASCADE], ivec3(tUvw) ).xy);
		const uint drawID = ID.x - 1;
		const uint instanceID = ID.y - 1;
		if ( ID.x == 0 || ID.y == 0 ) {
			imageStore(voxelRadiance[CASCADE], ivec3(tUvw), vec4(0));
			continue;
		}
	//	const DrawCommand drawCommand = drawCommands[drawID];
		const Instance instance = instances[instanceID];
		surface.material.id = instance.materialID;
		const Material material = materials[surface.material.id];
		surface.material.albedo = material.colorBase;
		surface.fragment = material.colorEmissive;

	#if DEFERRED_SAMPLING
		surface.uv = imageLoad(voxelUv[CASCADE], ivec3(tUvw) ).xy;
		if ( validTextureIndex( material.indexAlbedo ) ) {
			surface.material.albedo = sampleTexture( material.indexAlbedo );
		}

		// OPAQUE
		if ( material.modeAlpha == 0 ) {
			surface.material.albedo.a = 1;
		// BLEND
		} else if ( material.modeAlpha == 1 ) {

		// MASK
		} else if ( material.modeAlpha == 2 ) {

		}
		// Emissive textures
		if ( validTextureIndex( material.indexEmissive ) ) {
			surface.fragment += sampleTexture( material.indexEmissive );
		}
	#else
		surface.material.albedo = imageLoad(voxelRadiance[CASCADE], ivec3(tUvw) );
	#endif
		surface.material.metallic = material.factorMetallic;
		surface.material.roughness = material.factorRoughness;
		surface.material.occlusion = material.factorOcclusion;

		float litFactor = 1.0;
		if ( 0 <= material.indexLightmap ) {
			surface.fragment.rgb += surface.material.albedo.rgb + ubo.ambient.rgb * surface.material.occlusion;
		} else {
			surface.fragment.rgb += surface.material.albedo.rgb * ubo.ambient.rgb * surface.material.occlusion;
		}
		// corrections
		surface.material.roughness *= 4.0;
		{
			const vec3 F0 = mix(vec3(0.04), surface.material.albedo.rgb, surface.material.metallic); 
			const vec3 Lo = normalize( surface.position.world );
			const float cosLo = max(0.0, dot(surface.normal.world, Lo));
			for ( uint i = 0; i < ubo.lights; ++i ) {
				const Light light = lights[i];
				if ( light.power <= LIGHT_POWER_CUTOFF ) continue;
				if ( light.type >= 0 && 0 <= material.indexLightmap ) continue;
				const vec3 Lp = light.position;
				const vec3 Liu = light.position - surface.position.world;
				const vec3 Li = normalize(Liu);
				const float Ls = shadowFactor( light, 0.0 );
				const float La = 1.0 / (PI * pow(length(Liu), 2.0));
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
				if ( light.type >= 0 && 0 <= material.indexLightmap ) surface.fragment.rgb += (specular) * Lr * cosLi;
				// point light, compute only diffuse
				// else if ( abs(light.type) == 1 ) surface.fragment.rgb += (diffuse) * Lr * cosLi;
				else surface.fragment.rgb += (diffuse + specular) * Lr * cosLi;
				surface.fragment.a += light.power * La * Ls;
			}
		}

		imageStore(voxelRadiance[CASCADE], ivec3(tUvw), vec4(surface.fragment.rgb, surface.material.albedo.a));
	}
}