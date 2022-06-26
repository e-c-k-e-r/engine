#version 450
#pragma shader_stage(fragment)

//#extension GL_EXT_nonuniform_qualifier : enable

layout (constant_id = 0) const uint TEXTURES = 512;
layout (constant_id = 1) const uint CUBEMAPS = 128;
layout (constant_id = 2) const uint LAYERS = 32;

layout (binding = 5) uniform sampler2D samplerTextures[TEXTURES];
layout (binding = 6) uniform samplerCube samplerCubemaps[CUBEMAPS];

#define SHADOW_SAMPLES 16
#define FRAGMENT 1
#define BAKING 1
#define PBR 1
#define LAMBERT 0

#include "../../common/macros.h"
#include "../../common/structs.h"

layout (std140, binding = 7) readonly buffer Instances {
	Instance instances[];
};
layout (std140, binding = 8) readonly buffer Materials {
	Material materials[];
};
layout (std140, binding = 9) readonly buffer Textures {
	Texture textures[];
};
layout (std140, binding = 10) readonly buffer Lights {
	Light lights[];
};

layout (binding = 11, rgba8) uniform volatile coherent image3D outAlbedos;

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
layout (location = 9) flat in uint inLayer;

layout (location = 0) out vec4 outAlbedo;

void main() {
	const uint drawID = uint(inId.x);
	const uint instanceID = uint(inId.y);
	const uint materialID = uint(inId.z);

	surface.normal.world = normalize( inNormal );
	surface.uv.xy = wrap(inUv.xy);
	surface.uv.z = mipLevel(dFdx(inUv), dFdy(inUv));
	surface.position.world = inPosition;
	const Material material = materials[materialID];
	
	vec4 A = material.colorBase;
	surface.material.metallic = material.factorMetallic;
	surface.material.roughness = material.factorRoughness;
	surface.material.occlusion = 1.0f - material.factorOcclusion;

	surface.light = material.colorEmissive;
	surface.material.albedo = vec4(1);
#if 1
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

			surface.light.rgb += (diffuse + specular) * Lr * cosLi;
			surface.light.a += light.power * La * Ls;
		}
	}
#else
	{
		// corrections
		surface.material.roughness *= 4.0;
		const vec3 F0 = mix(vec3(0.04), surface.material.albedo.rgb, surface.material.metallic); 
		const vec3 Lo = normalize( surface.position.world );
		const float cosLo = max(0.0, dot(surface.normal.world, Lo));
		for ( uint i = 0; i < lights.length(); ++i ) {
			const Light light = lights[i];
			if ( light.power <= LIGHT_POWER_CUTOFF ) continue;
			if ( light.type >= 0 && validTextureIndex( surface.instance.lightmapID ) ) continue;
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

			surface.light.rgb += (diffuse + specular) * Lr * cosLi;
			surface.light.a += light.power * La * Ls;
		}
	}
#endif
#define EXPOSURE 0
#define GAMMA 0

//	surface.light.rgb = vec3(1.0) - exp(-surface.light.rgb * EXPOSURE);
//	surface.light.rgb = pow(surface.light.rgb, vec3(1.0 / GAMMA));

	outAlbedo = vec4(surface.light.rgb, 1);

	{
		const vec2 st = inSt.xy * imageSize(outAlbedos).xy;
		const ivec3 uvw = ivec3(int(st.x), int(st.y), int(inLayer));
		imageStore(outAlbedos, uvw, vec4(surface.light.rgb, 1) );
	}
}