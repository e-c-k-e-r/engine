#version 450
#pragma shader_stage(fragment)

#extension GL_EXT_nonuniform_qualifier : enable

layout (constant_id = 0) const uint TEXTURES = 1;
layout (binding = 0) uniform sampler2D samplerTextures[TEXTURES];

#define SHADOW_SAMPLES 16

#define PBR 0
#define LAMBERT 1

#include "../../common/macros.h"
#include "../../common/structs.h"
#include "../../common/functions.h"
#include "../../common/shadows.h"

layout (std140, binding = 3) readonly buffer Materials {
	Material materials[];
};
layout (std140, binding = 4) readonly buffer Textures {
	Texture textures[];
};
layout (std140, binding = 5) readonly buffer Lights {
	Light lights[];
};

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec4 inColor;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in mat3 inTBN;
layout (location = 6) in vec3 inPosition;
layout (location = 7) flat in ivec4 inId;

layout (location = 0) out vec4 outAlbedo;

void main() {
	vec4 A = vec4(1, 1, 1, 1);
	surface.normal.world = normalize( inNormal );
	const float mip = mipLevel(inUv.xy);
	surface.uv = wrap(inUv.xy);
	surface.position.world = inPosition;
	surface.material.id = int(inId.y);
	const Material material = materials[surface.material.id];
	
	surface.material.metallic = material.factorMetallic;
	surface.material.roughness = material.factorRoughness;
	surface.material.occlusion = 1.0f - material.factorOcclusion;

	surface.fragment = material.colorEmissive;
#if 0
	// sample albedo
	const bool useAtlas = validTextureIndex( material.indexAtlas );
	Texture textureAtlas;
	if ( useAtlas ) textureAtlas = textures[material.indexAtlas];
	if ( !validTextureIndex( material.indexAlbedo ) ) discard;
	{
		const Texture t = textures[material.indexAlbedo];
		surface.material.albedo = textureLod( samplerTextures[nonuniformEXT((useAtlas) ? textureAtlas.index : t.index)], (useAtlas) ? mix( t.lerp.xy, t.lerp.zw, uv ) : uv, mip );
		// alpha mode OPAQUE
		if ( material.modeAlpha == 0 ) {
			surface.material.albedo.a = 1;
		// alpha mode BLEND
		} else if ( material.modeAlpha == 1 ) {

		// alpha mode MASK
		} else if ( material.modeAlpha == 2 ) {
			if ( surface.material.albedo.a < abs(material.factorAlphaCutoff) ) discard;
			surface.material.albedo.a = 1;
		}
		if ( surface.material.albedo.a == 0 ) discard;
	}

	// sample normal
	if ( validTextureIndex( material.indexNormal ) ) {
		const Texture t = textures[material.indexNormal];
		surfacem.normal.world = inTBN * normalize( textureLod( samplerTextures[nonuniformEXT((useAtlas)?textureAtlas.index:t.index)], ( useAtlas ) ? mix( t.lerp.xy, t.lerp.zw, uv ) : uv, mip ).xyz * 2.0 - vec3(1.0));
	}

	// sample emissive
	if ( validTextureIndex( material.indexEmissive ) ) {
		const Texture t = textures[material.indexEmissive];
		surface.fragment = textureLod( samplerTextures[nonuniformEXT((useAtlas) ? textureAtlas.index : t.index)], (useAtlas) ? mix( t.lerp.xy, t.lerp.zw, uv ) : uv, mip );
	}
#else
	surface.material.albedo = vec4(1);
#endif

	{
		const vec3 F0 = mix(vec3(0.04), surface.material.albedo.rgb, surface.material.metallic); 
		const vec3 Lo = normalize( -surface.position.world );
		const float cosLo = max(0.0, dot(surface.normal.world, Lo));
		for ( uint i = 0; i < lights.length(); ++i ) {
			const Light light = lights[i];
			if ( light.power <= LIGHT_POWER_CUTOFF ) continue;
			const vec3 Lp = light.position;
			const vec3 Liu = light.position - surface.position.world;
			const float La = 1.0 / (PI * pow(length(Liu), 2.0));
			const float Ls = shadowFactor( light, 0.0 );
			if ( light.power * La * Ls <= LIGHT_POWER_CUTOFF ) continue;

			const vec3 Li = normalize(Liu);
			const vec3 Lr = light.color.rgb * light.power * La * Ls;
			const float cosLi = abs(dot(surface.normal.world, Li));// max(0.0, dot(N, Li));
		#if LAMBERT
			const vec3 diffuse = surface.material.albedo.rgb;
			const vec3 specular = vec3(0);
		#elif PBR
			const vec3 Lh = normalize(Li + Lo);
			const float cosLh = max(0.0, dot(N, Lh));
			
			const vec3 F = fresnelSchlick( F0, max( 0.0, dot(Lh, Lo) ) );
			const float D = ndfGGX( cosLh, surface.material.roughness );
			const float G = gaSchlickGGX(cosLi, cosLo, surface.material.roughness);
			const vec3 diffuse = mix( vec3(1.0) - F, vec3(0.0), surface.material.metallic ) * surface.material.albedo.rgb;
			const vec3 specular = (F * D * G) / max(EPSILON, 4.0 * cosLi * cosLo);
		#endif
			surface.fragment.rgb += (diffuse + specular) * Lr * cosLi;
			surface.fragment.a += light.power * La * Ls;
		}
	}

	outAlbedo = vec4(surface.fragment.rgb, 1);
}