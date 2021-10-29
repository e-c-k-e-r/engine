#version 450
#pragma shader_stage(fragment)

//#extension GL_EXT_nonuniform_qualifier : enable

layout (constant_id = 0) const uint TEXTURES = 512;
layout (constant_id = 1) const uint CUBEMAPS = 128;

layout (binding = 4) uniform sampler2D samplerTextures[TEXTURES];
layout (binding = 5) uniform samplerCube samplerCubemaps[CUBEMAPS];

#define SHADOW_SAMPLES 16
#define FRAGMENT 1
#define BAKING 1
#define PBR 1
#define LAMBERT 0

#include "../../common/macros.h"
#include "../../common/structs.h"

layout (std140, binding = 6) readonly buffer Instances {
	Instance instances[];
};
layout (std140, binding = 7) readonly buffer Materials {
	Material materials[];
};
layout (std140, binding = 8) readonly buffer Textures {
	Texture textures[];
};
layout (std140, binding = 9) readonly buffer Lights {
	Light lights[];
};

#include "../../common/functions.h"
#include "../../common/shadows.h"
#if PBR
#include "../../common/pbr.h"
#endif

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec2 inSt;
layout (location = 2) in vec4 inColor;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in mat3 inTBN;
layout (location = 7) in vec3 inPosition;
layout (location = 8) flat in uvec4 inId;

layout (location = 0) out vec4 outAlbedo;

void main() {
	const float mip = mipLevel(inUv.xy);
	const uint drawID = uint(inId.x);
	const uint instanceID = uint(inId.y);
	const uint materialID = uint(inId.z);

	vec4 A = vec4(1, 1, 1, 1);
	surface.normal.world = normalize( inNormal );
	surface.uv = wrap(inUv.xy);
	surface.position.world = inPosition;
	surface.material.id = materialID;
	const Material material = materials[surface.material.id];
	
	surface.material.metallic = material.factorMetallic;
	surface.material.roughness = material.factorRoughness;
	surface.material.occlusion = 1.0f - material.factorOcclusion;

	surface.fragment = material.colorEmissive;
	surface.material.albedo = vec4(1);
	{
		const vec3 F0 = mix(vec3(0.04), surface.material.albedo.rgb, surface.material.metallic); 
		for ( uint i = 0; i < lights.length(); ++i ) {
			const Light light = lights[i];
			const mat4 mat = light.view; // inverse(light.view);
			const vec3 position = surface.position.world;
		//	const vec3 position = vec3( mat * vec4(surface.position.world, 1.0) );
			const vec3 normal = surface.normal.world;
		//	const vec3 normal = vec3( mat * vec4(surface.normal.world, 0.0) );

			if ( light.power <= LIGHT_POWER_CUTOFF ) continue;
			const vec3 Lp = light.position;
			const vec3 Liu = light.position - surface.position.world;
			const float La = 1.0 / (PI * pow(length(Liu), 2.0));
			const float Ls = shadowFactor( light, 0.0 );
			if ( light.power * La * Ls <= LIGHT_POWER_CUTOFF ) continue;

			const vec3 Lo = normalize( -position );
			const float cosLo = max(0.0, dot(normal, Lo));

			const vec3 Li = normalize(Liu);
			const vec3 Lr = light.color.rgb * light.power * La * Ls;
		//	const float cosLi = max(0.0, dot(normal, Li));
			const float cosLi = abs(dot(normal, Li));
		#if LAMBERT
			const vec3 diffuse = surface.material.albedo.rgb;
			const vec3 specular = vec3(0);
		#elif PBR
			const vec3 Lh = normalize(Li + Lo);
		//	const float cosLh = max(0.0, dot(normal, Lh));
			const float cosLh = abs(dot(normal, Lh));
			
			const vec3 F = fresnelSchlick( F0, max( 0.0, dot(Lh, Lo) ) );
			const float D = 1; // ndfGGX( cosLh, surface.material.roughness );
			const float G = gaSchlickGGX(cosLi, cosLo, surface.material.roughness);
			const vec3 diffuse = mix( vec3(1.0) - F, vec3(0.0), surface.material.metallic ) * surface.material.albedo.rgb;
			const vec3 specular = (F * D * G) / max(EPSILON, 4.0 * cosLi * cosLo);
		#endif
			surface.fragment.rgb += (diffuse + specular) * Lr * cosLi;
			surface.fragment.a += light.power * La * Ls;
		}
	}

#define EXPOSURE 1
#define GAMMA 1

//	surface.fragment.rgb = vec3(1.0) - exp(-surface.fragment.rgb * EXPOSURE);
//	surface.fragment.rgb = pow(surface.fragment.rgb, vec3(1.0 / GAMMA));

	outAlbedo = vec4(surface.fragment.rgb, 1);
}