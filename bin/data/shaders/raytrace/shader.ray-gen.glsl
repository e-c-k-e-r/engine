#version 460
#extension GL_EXT_ray_tracing : enable
#pragma shader_stage(raygen)
layout (constant_id = 0) const uint PASSES = 2;
layout (constant_id = 1) const uint TEXTURES = 512;
layout (constant_id = 2) const uint CUBEMAPS = 128;
layout (constant_id = 3) const uint CASCADES = 4;

#define ADDRESS_ENABLED 1
#define COMPUTE 1
#define PBR 1
#define VXGI 0
#define RAYTRACE 1
#define FOG 0
#define WHITENOISE 0
#define MAX_TEXTURES TEXTURES

#include "../common/macros.h"
#include "../common/structs.h"

layout( push_constant ) uniform PushBlock {
	uint pass;
	uint draw;
} PushConstant;

layout (binding = 0) uniform accelerationStructureEXT tlas;

layout (binding = 1, rgba8) uniform volatile coherent image2D outImage;

layout (binding = 2) uniform UBO {
	EyeMatrices eyes[2];

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

	 int indexSkybox;
	uint useLightmaps;
	uint padding2;
	uint padding3;
} ubo;

layout (std140, binding = 3) readonly buffer Instances {
	Instance instances[];
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

#if VXGI
	layout (binding = 14) uniform usampler3D voxelId[CASCADES];
	layout (binding = 15) uniform sampler3D voxelNormal[CASCADES];
	layout (binding = 16) uniform sampler3D voxelRadiance[CASCADES];
#endif

layout (location = 0) rayPayloadEXT RayTracePayload payload;

#include "../common/functions.h"
#include "../common/light.h"
#if VXGI
	#include "../common/vxgi.h"
#endif

float shadowFactor( const Light light, float def ) {

	Ray ray;
	ray.origin = surface.position.world;
	ray.direction = light.position - ray.origin;

	uint rayFlags = gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT;
	uint cullMask = 0xFF;
	float tMin = 0.125;
	float tMax = length(ray.direction) - 1;
	
	ray.direction = normalize(ray.direction);

	payload.hit = true;
	traceRayEXT(tlas, rayFlags, cullMask, 0, 0, 0, ray.origin.xyz, tMin, ray.direction.xyz, tMax, 0);

	return payload.hit ? 0.0 : 1.0;
}

void processHit( uint instanceID, Triangle triangle ) {
	const Instance instance = instances[instanceID];
	surface.instance = instance;

	surface.pass = 0;
	surface.fragment = vec4(0);
	surface.light = vec4(0);

	// bind position
	{
		surface.position.world = vec3( instance.model * vec4(triangle.point.position, 1.0 ) );
		surface.position.eye = vec3( ubo.eyes[surface.pass].view * vec4(surface.position.world, 1.0) );
	}
	// bind normals
	{
		surface.normal.world = vec3( instance.model * vec4(triangle.point.normal, 0.0 ) );
		surface.normal.eye = vec3( ubo.eyes[surface.pass].view * vec4(surface.normal.world, 0.0) );
	}
	// bind UVs
	{
		surface.uv.xy = triangle.point.uv;
		surface.st.xy = triangle.point.st;
	}

	const Material material = materials[surface.instance.materialID];
	surface.material.albedo = material.colorBase;
	surface.material.metallic = material.factorMetallic;
	surface.material.roughness = material.factorRoughness;
	surface.material.occlusion = material.factorOcclusion;
	surface.fragment = material.colorEmissive;


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
	// Lightmap
	if ( bool(ubo.useLightmaps) && validTextureIndex( surface.instance.lightmapID ) ) {
		surface.light += surface.material.albedo * sampleTexture( surface.instance.lightmapID, surface.st );
	}
	// Emissive textures
	if ( validTextureIndex( material.indexEmissive ) ) {
		surface.light += sampleTexture( material.indexEmissive );
	}
	// Occlusion map
	if ( validTextureIndex( material.indexOcclusion ) ) {
	 	surface.material.occlusion = sampleTexture( material.indexOcclusion ).r;
	}
	// Metallic/Roughness map
	if ( validTextureIndex( material.indexMetallicRoughness ) ) {
	 	vec4 samp = sampleTexture( material.indexMetallicRoughness );
	 	surface.material.metallic = samp.r;
		surface.material.roughness = samp.g;
	}
#if VXGI
	{
		indirectLighting();
	}
#endif

	{
		surface.light.rgb += surface.material.albedo.rgb * ubo.ambient.rgb * surface.material.occlusion; // add ambient lighting
		surface.light.rgb += surface.material.indirect.rgb; // add indirect lighting
	#if PBR
		pbr();
	#endif
		surface.fragment.rgb += surface.light.rgb;
		surface.fragment.a = surface.material.albedo.a;
	}
}

void trace( float tMin, float tMax ) {
	uint rayFlags = gl_RayFlagsOpaqueEXT;
	uint cullMask = 0xFF;

	payload.hit = false;
	traceRayEXT(tlas, rayFlags, cullMask, 0, 0, 0, surface.ray.origin.xyz, tMin, surface.ray.direction.xyz, tMax, 0);
}

void main()  {
	{
		surface.fragment = vec4(0);
		surface.pass = PushConstant.pass;

		const vec2 inUv = (vec2(gl_LaunchIDEXT.xy) + vec2(0.5)) / vec2(gl_LaunchSizeEXT.xy);

		const mat4 iProjectionView = inverse( ubo.eyes[surface.pass].projection * mat4(mat3(ubo.eyes[surface.pass].view)) );
		const vec4 near4 = iProjectionView * (vec4(2.0 * inUv - 1.0, -1.0, 1.0));
		const vec4 far4 = iProjectionView * (vec4(2.0 * inUv - 1.0, 1.0, 1.0));
		const vec3 near3 = near4.xyz / near4.w;
		const vec3 far3 = far4.xyz / far4.w;

		surface.ray.direction = normalize( far3 - near3 );
		surface.ray.origin = ubo.eyes[surface.pass].eyePos.xyz;
	}
	
	trace( 0.001, 4096.0 );

	if ( payload.hit ) {
		processHit( payload.instanceID, payload.triangle );
	#if 1

		uint tries = 8;
		while ( 0 < surface.material.metallic && --tries > 0 ) {
			vec4 back = surface.fragment;
			surface.fragment = vec4(0);

			surface.ray.direction = reflect( normalize(surface.position.world - surface.ray.origin), surface.normal.world );
			surface.ray.origin = surface.position.world;
			
			trace( 0.01, 4096.0 );

			if ( payload.hit ) {
				processHit( payload.instanceID, payload.triangle );
				vec4 front = surface.fragment;
				surface.fragment = mix( front, back, surface.material.metallic );
			//	surface.fragment.rgb = front.rgb * front.a + back.rgb * back.a;
			} else {
				surface.fragment = back;
			}
		}
	#endif
	#if 0
		if ( surface.fragment.a < 1.0 ) {
			vec4 back = surface.fragment;
			surface.fragment = vec4(0);

			surface.ray.origin = surface.position.world;
			
			trace( 0.01, 4096.0 );

			if ( payload.hit ) {
				processHit( payload.instanceID, payload.triangle );
				vec4 front = surface.fragment;

			//	surface.fragment = mix( front, back, back.a );
				surface.fragment.rgb = front.rgb * front.a + back.rgb * back.a;
			} else {
				surface.fragment = back;
			}
		}
	#endif
	} else if ( 0 <= ubo.indexSkybox && ubo.indexSkybox < CUBEMAPS ) {
		surface.fragment.rgb = texture( samplerCubemaps[ubo.indexSkybox], surface.ray.direction ).rgb;
	}

	{
		surface.fragment.a = 1;
	//	float brightness = dot(surface.fragment.rgb, vec3(0.2126, 0.7152, 0.0722));
	//	outFragBright = brightness > ubo.brightnessThreshold ? vec4(surface.fragment.rgb, 1.0) : vec4(0, 0, 0, 1);

	#if FOG
		fog( surface.ray, surface.fragment.rgb, surface.fragment.a );
	#endif
	#if TONE_MAP
		surface.fragment.rgb = vec3(1.0) - exp(-surface.fragment.rgb * ubo.exposure);
	#endif
	#if GAMMA_CORRECT
		surface.fragment.rgb = pow(surface.fragment.rgb, vec3(1.0 / ubo.gamma));
	#endif
	#if WHITENOISE
		if ( enabled(ubo.mode.type, 1) ) whitenoise(surface.fragment.rgb, ubo.mode.parameters);
	#endif
		
		imageStore(outImage, ivec2(gl_LaunchIDEXT.xy), surface.fragment);
	}
}