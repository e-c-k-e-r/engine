#version 450
#pragma shader_stage(compute)

#extension GL_EXT_samplerless_texture_functions : require
#extension GL_EXT_nonuniform_qualifier : enable

layout (local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

#define COMPUTE 1

#define VXGI 1
#define MAX_CUBEMAPS CUBEMAPS
#define GAMMA_CORRECT 1
#define PBR 1
#define LAMBERT 0

layout (constant_id = 0) const uint TEXTURES = 512;
layout (constant_id = 1) const uint CUBEMAPS = 128;
layout (constant_id = 2) const uint CASCADES = 16;

#include "../../common/macros.h"
#include "../../common/structs.h"

layout (binding = 0) uniform UBO {
	EyeMatrices eyes[2];

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

layout (binding = 10, r32ui) uniform volatile coherent uimage3D voxelDrawId[CASCADES];
layout (binding = 11, r32ui) uniform volatile coherent uimage3D voxelInstanceId[CASCADES];
layout (binding = 12, r32ui) uniform volatile coherent uimage3D voxelNormalX[CASCADES];
layout (binding = 13, r32ui) uniform volatile coherent uimage3D voxelNormalY[CASCADES];
layout (binding = 14, r32ui) uniform volatile coherent uimage3D voxelRadianceR[CASCADES];
layout (binding = 15, r32ui) uniform volatile coherent uimage3D voxelRadianceG[CASCADES];
layout (binding = 16, r32ui) uniform volatile coherent uimage3D voxelRadianceB[CASCADES];
layout (binding = 17, r32ui) uniform volatile coherent uimage3D voxelRadianceA[CASCADES];
layout (binding = 18, r32ui) uniform volatile coherent uimage3D voxelCount[CASCADES];
layout (binding = 19, rgba8) uniform volatile coherent image3D voxelOutput[CASCADES];

#include "../../common/functions.h"
#include "../../common/light.h"
#undef VXGI
#include "../../common/shadows.h"

void main() {
	const vec3 tUvw = gl_GlobalInvocationID.xzy;
	for ( uint CASCADE = 0; CASCADE < CASCADES; ++CASCADE ) {
		vec2 N_E = vec2(
			uintBitsToFloat(imageLoad(voxelNormalX[CASCADE], ivec3(tUvw) ).x),
			uintBitsToFloat(imageLoad(voxelNormalY[CASCADE], ivec3(tUvw) ).x)
		);

		surface.normal.world = decodeNormals( N_E );
		surface.normal.eye = vec3( ubo.settings.vxgi.matrix * vec4( surface.normal.world, 0.0f ) );
	
		surface.position.eye = (vec3(gl_GlobalInvocationID.xyz) / vec3(imageSize(voxelOutput[CASCADE])) * 2.0f - 1.0f) * cascadePower(CASCADE);
		surface.position.world = vec3( inverse(ubo.settings.vxgi.matrix) * vec4( surface.position.eye, 1.0f ) );

		surface.pass = 0; // PushConstant.pass;
		surface.fragment = vec4(0);
		surface.light = vec4(0);
		surface.motion = vec2(0);
		surface.material.indirect = vec4(0);

#if 0
	#if 0
		vec4 A = imageLoad(voxelOutput[CASCADE], ivec3(tUvw) );
	#else
		vec4 A = vec4(0);
		A.r = imageLoad(voxelRadianceR[CASCADE], ivec3(tUvw) ).r;
		A.g = imageLoad(voxelRadianceG[CASCADE], ivec3(tUvw) ).r;
		A.b = imageLoad(voxelRadianceB[CASCADE], ivec3(tUvw) ).r;
		A.a = imageLoad(voxelRadianceA[CASCADE], ivec3(tUvw) ).r;
		A /= 256.0;

		uint count = imageLoad(voxelCount[CASCADE], ivec3(tUvw) ).r;
		if ( count > 0 ) A /= count;
	/*
		vec4 A = vec4(surface.normal.world, 1.0);
	*/
	#endif

		surface.material.albedo = A;
		surface.fragment.rgb = surface.material.albedo.rgb;
		
		const bool DISCARD_DUE_TO_DIVERGENCE = surface.material.albedo.a == 0;
#else
		const uvec2 ID = uvec2(
			imageLoad(voxelDrawId[CASCADE], ivec3(tUvw) ).x,
			imageLoad(voxelInstanceId[CASCADE], ivec3(tUvw) ).x
		);
		const bool DISCARD_DUE_TO_DIVERGENCE = ID.x == 0 || ID.y == 0;

		const uint drawID = ID.x == 0 ? 0 : ID.x - 1;
		const uint instanceID = ID.y == 0 ? 0 : ID.y - 1;
	
	//	if ( ID.x == 0 || ID.y == 0 ) {
	#if 1
		if ( DISCARD_DUE_TO_DIVERGENCE ) {
			imageStore(voxelOutput[CASCADE], ivec3(tUvw), vec4(0));
			continue;
		}
	#endif
	
		const DrawCommand drawCommand = drawCommands[drawID];
		surface.instance = instances[instanceID];
		const Material material = materials[surface.instance.materialID];
		surface.material.albedo = material.colorBase;
		surface.fragment = material.colorEmissive;

	#if 0
		vec4 A = imageLoad(voxelOutput[CASCADE], ivec3(tUvw) );
	#else
		vec4 A = vec4(0);
		A.r = imageLoad(voxelRadianceR[CASCADE], ivec3(tUvw) ).r;
		A.g = imageLoad(voxelRadianceG[CASCADE], ivec3(tUvw) ).r;
		A.b = imageLoad(voxelRadianceB[CASCADE], ivec3(tUvw) ).r;
		A.a = imageLoad(voxelRadianceA[CASCADE], ivec3(tUvw) ).r;
		A /= 256.0;

		uint count = imageLoad(voxelCount[CASCADE], ivec3(tUvw) ).r;
		if ( count > 0 ) A /= count;
	#endif

		surface.material.albedo = A;
		surface.material.metallic = material.factorMetallic;
		surface.material.roughness = material.factorRoughness;
		surface.material.occlusion = material.factorOcclusion;

		const vec3 ambient = ubo.settings.lighting.ambient.rgb * surface.material.occlusion;
		if ( validTextureIndex( surface.instance.lightmapID ) ) {
			surface.fragment.rgb += surface.material.albedo.rgb;
		} else {
			surface.fragment.rgb += surface.material.albedo.rgb * ambient;
			// corrections
			surface.position.eye = vec3( ubo.eyes[surface.pass].view * vec4( surface.position.world, 1 ) );
			surface.normal.eye = vec3( ubo.eyes[surface.pass].view * vec4(surface.normal.world, 0) );
		#if 0
			pbr();
		#else
		/*
			surface.material.roughness *= 4.0;
			const vec3 F0 = mix(vec3(0.04), surface.material.albedo.rgb, surface.material.metallic); 
			const vec3 Lo = normalize( surface.position.world );
			const float cosLo = max(0.0, dot(surface.normal.world, Lo));
			for ( uint i = 0; i < ubo.settings.lengths.lights; ++i ) {
				const Light light = lights[i];
			
			//	if ( light.power <= LIGHT_POWER_CUTOFF ) continue;
			//	if ( light.type >= 0 && validTextureIndex( surface.instance.lightmapID ) ) continue;

				const vec3 Lp = light.position;
				const vec3 Liu = light.position - surface.position.world;
				const vec3 Li = normalize(Liu);
				const float Ls = shadowFactor( light, 0.0 );
				const float La = 1.0 / (1 + (PI * pow(length(Liu), 2.0)));
			
			//	if ( light.power * La * Ls <= LIGHT_POWER_CUTOFF ) continue;

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
		*/
		#endif
		}
		surface.fragment.rgb += surface.light.rgb;
	#endif

	#if TONE_MAP
		toneMap(surface.fragment.rgb, ubo.settings.bloom.exposure);
	#endif
	#if GAMMA_CORRECT
		gammaCorrect(surface.fragment.rgb, 1.0 / ubo.settings.bloom.gamma);
	#endif

	#if 0
		imageStore(voxelOutput[CASCADE], ivec3(tUvw), vec4(surface.fragment.rgb, surface.material.albedo.a));
	#else
		if ( DISCARD_DUE_TO_DIVERGENCE ) {
			imageStore(voxelOutput[CASCADE], ivec3(tUvw), vec4(0));
		} else {
			imageStore(voxelOutput[CASCADE], ivec3(tUvw), vec4(surface.fragment.rgb, surface.material.albedo.a));
		}
	#endif
	}
}