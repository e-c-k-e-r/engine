#version 460

#extension GL_EXT_ray_tracing : enable
#extension GL_ARB_shader_clock : enable

#pragma shader_stage(raygen)
layout (constant_id = 0) const uint PASSES = 2;
layout (constant_id = 1) const uint TEXTURES = 512;
layout (constant_id = 2) const uint CUBEMAPS = 128;
layout (constant_id = 3) const uint CASCADES = 4;

#define COMPUTE 1
#define PBR 1
#define VXGI 0
#define RAYTRACE 1
#define FOG 0
#define WHITENOISE 0
#define MAX_TEXTURES TEXTURES
#define TONE_MAP 1
#define GAMMA_CORRECT 1

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
	uint frameNumber;
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

void trace( Ray ray, float tMin, float tMax ) {
	uint rayFlags = gl_RayFlagsOpaqueEXT;
	uint cullMask = 0xFF;
	
	payload.hit = false;
	traceRayEXT(tlas, rayFlags, cullMask, 0, 0, 0, ray.origin, tMin, ray.direction, tMax, 0);
}
void trace( Ray ray, float tMin ) {
	uint rayFlags = gl_RayFlagsOpaqueEXT;
	uint cullMask = 0xFF;

	float tMax = 4096.0;
	
	payload.hit = false;
	traceRayEXT(tlas, rayFlags, cullMask, 0, 0, 0, ray.origin, tMin, ray.direction, tMax, 0);
}

void trace( Ray ray ) {
	trace( ray, 0.001, 4096.0 );
}

float shadowFactor( const Light light, float def ) {
	Ray ray;
	ray.origin = surface.position.world;
	ray.direction = light.position - ray.origin;

	float tMin = 0.001;
	float tMax = length(ray.direction);
	
	ray.direction = normalize(ray.direction);

	uint rayFlags = gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT;
	uint cullMask = 0xFF;

	payload.hit = true;
	traceRayEXT(tlas, rayFlags, cullMask, 0, 0, 0, ray.origin, tMin, ray.direction, tMax, 0);

	return payload.hit ? 0.0 : 1.0;
}

void setupSurface( RayTracePayload payload ) {
	const Instance instance = instances[payload.triangle.instanceID];
	surface.instance = instance;
	surface.fragment = vec4(0);
	surface.light = vec4(0);

	// bind position
	{
		surface.position.world = vec3( instance.model * vec4(payload.triangle.point.position, 1.0 ) );
		surface.position.eye = vec3( ubo.eyes[surface.pass].view * vec4(surface.position.world, 1.0) );
	}
	// bind normals
	{
		surface.normal.world = normalize(vec3( instance.model * vec4(payload.triangle.point.normal, 0.0 ) ));
	//	surface.normal.world = faceforward( surface.normal.world, surface.ray.direction, surface.normal.world );
	//	surface.normal.eye = normalize(vec3( ubo.eyes[surface.pass].view * vec4(surface.normal.world, 0.0) ));

	//	surface.tbn[0] = normalize(vec3( instance.model * vec4(payload.triangle.tbn[0], 0.0 ) ));
	//	surface.tbn[1] = normalize(vec3( instance.model * vec4(payload.triangle.tbn[1], 0.0 ) ));
	//	surface.tbn[2] = surface.normal.world;

		vec3 tangent = normalize(vec3( instance.model * vec4(payload.triangle.point.tangent, 0.0) ));
		vec3 bitangent = normalize(vec3( instance.model * vec4(cross( payload.triangle.point.normal, payload.triangle.point.tangent ), 0.0) ));
		if ( payload.triangle.point.tangent != vec3(0) ) {
			surface.tbn = mat3(tangent, bitangent, payload.triangle.point.normal);
		} else {
			surface.tbn = mat3(1);
		}
	}
	// bind UVs
	{
		surface.uv.xy = payload.triangle.point.uv;
		surface.st.xy = payload.triangle.point.st;
	}

	const Material material = materials[surface.instance.materialID];
	surface.material.albedo = material.colorBase;
	surface.material.metallic = material.factorMetallic;
	surface.material.roughness = material.factorRoughness;
	surface.material.occlusion = material.factorOcclusion;
	surface.light = material.colorEmissive;

	if ( validTextureIndex( material.indexAlbedo ) ) {
		surface.material.albedo *= sampleTexture( material.indexAlbedo );
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
		vec4 light = sampleTexture( surface.instance.lightmapID, surface.st );
		surface.material.lightmapped = light.a > 0.001;
		if ( surface.material.lightmapped )	surface.light += surface.material.albedo * light;
	} else {
		surface.material.lightmapped = false;
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
	// Normals
	if ( validTextureIndex( material.indexNormal ) && surface.tbn != mat3(1) ) {
		surface.normal.world = surface.tbn * normalize( sampleTexture( material.indexNormal ).xyz * 2.0 - vec3(1.0));
	}
	{
		surface.normal.eye = normalize(vec3( ubo.eyes[surface.pass].view * vec4(surface.normal.world, 0.0) ));
	}
}

void directLighting() {
#if VXGI
	indirectLighting();
#endif

	surface.light.rgb += surface.material.albedo.rgb * ubo.ambient.rgb * surface.material.occlusion; // add ambient lighting
	surface.light.rgb += surface.material.indirect.rgb; // add indirect lighting
#if PBR
	pbr();
#elif LAMBERT
	lambert();
#elif PHONG
	phong();
#endif
	surface.fragment.rgb += surface.light.rgb;
	surface.fragment.a = surface.material.albedo.a;
}

vec4 traceStep( Ray ray ) {
	vec4 outFrag = vec4(0);

	// initial condition
	{
		trace( ray );

		if ( payload.hit ) {
			setupSurface( payload );
			directLighting();
			outFrag = surface.fragment;
		} else if (  0 <= ubo.indexSkybox && ubo.indexSkybox < CUBEMAPS ) {
			outFrag = texture( samplerCubemaps[ubo.indexSkybox], ray.direction );
		}
	}

	// "transparency"
	if ( payload.hit && surface.material.albedo.a < 0.999 ) {
		if ( surface.material.albedo.a < 0.001 ) outFrag = vec4(0);

		RayTracePayload surfacePayload = payload;
		Ray transparency;
		transparency.direction = ray.direction;
		transparency.origin = surface.position.world;

		vec4 transparencyColor = vec4(1.0 - surface.material.albedo.a);
		trace( transparency, 1.75 );
		if ( payload.hit ) {
			setupSurface( payload );
			directLighting();
			transparencyColor *= surface.fragment;
		} else if (  0 <= ubo.indexSkybox && ubo.indexSkybox < CUBEMAPS ) {
			transparencyColor *= texture( samplerCubemaps[ubo.indexSkybox], ray.direction );
		}

		outFrag += transparencyColor;

		payload = surfacePayload;
		setupSurface( payload );
	}

	// reflection
	if ( payload.hit ) {
		const float REFLECTIVITY = 1.0 - surface.material.roughness;
		const vec4 REFLECTED_ALBEDO = surface.material.albedo * REFLECTIVITY;

		if ( REFLECTIVITY > 0.001 ) {
			RayTracePayload surfacePayload = payload;

			Ray reflection;
			reflection.origin = surface.position.world;
			reflection.direction = reflect( ray.direction, surface.normal.world );

			trace( reflection );

			vec4 reflectionColor = REFLECTED_ALBEDO;

			if ( payload.hit ) {
				setupSurface( payload );
				directLighting();
				reflectionColor *= surface.fragment;
			} else if (  0 <= ubo.indexSkybox && ubo.indexSkybox < CUBEMAPS ) {
				reflectionColor *= texture( samplerCubemaps[ubo.indexSkybox], reflection.direction );
			}

			outFrag += reflectionColor;

			payload = surfacePayload;
			setupSurface( payload );
		}
	}

	return outFrag;
}

void main()  {
//	if ( ubo.frameNumber > 16 ) return;	
//	prngSeed = tea(gl_LaunchIDEXT.y * gl_LaunchSizeEXT.x + gl_LaunchIDEXT.x, ubo.frameNumber);
	prngSeed = tea(gl_LaunchIDEXT.y * gl_LaunchSizeEXT.x + gl_LaunchIDEXT.x, int(clockARB()));
	surface.pass = PushConstant.pass;
	vec4 outFrag = vec4(0);

	const uint SAMPLES = 1;
	const uint NUM_PATHS = 2;
#if 0
	const uint FRAME_ACCUMULATION_VALUE = ubo.frameNumber + 1;
#else
	const uint FRAME_ACCUMULATION_VALUE = min(32, ubo.frameNumber + 1);
#endif
	const float BLEND_FACTOR = 1.0f / float(FRAME_ACCUMULATION_VALUE);
	uint FRAME_NUMBER = ubo.frameNumber;

	for ( uint samp = 0; samp < SAMPLES; ++samp, ++FRAME_NUMBER ) {
		{
			const vec2 center = ( FRAME_NUMBER > 0 ) ? vec2( rnd(), rnd() ) : vec2(0.5);
			const vec2 inUv = (vec2(gl_LaunchIDEXT.xy) + center) / vec2(gl_LaunchSizeEXT.xy);
		#if 0
			vec4 target	= ubo.eyes[surface.pass].iProjection * vec4(inUv.x * 2.0f - 1.0f, inUv.y * 2.0f - 1.0f, 1, 1);
			vec4 direction = ubo.eyes[surface.pass].iView * vec4(normalize(target.xyz), 0);

			surface.ray.direction = vec3(direction);
			surface.ray.origin = ubo.eyes[surface.pass].eyePos.xyz;
		#else
			const mat4 iProjectionView = inverse( ubo.eyes[surface.pass].projection * mat4(mat3(ubo.eyes[surface.pass].view)) );
			const vec4 near4 = iProjectionView * (vec4(2.0 * inUv - 1.0, -1.0, 1.0));
			const vec4 far4 = iProjectionView * (vec4(2.0 * inUv - 1.0, 1.0, 1.0));
			const vec3 near3 = near4.xyz / near4.w;
			const vec3 far3 = far4.xyz / far4.w;

			surface.ray.direction = normalize( far3 - near3 );
			surface.ray.origin = ubo.eyes[surface.pass].eyePos.xyz;
		#endif
		}

		{
			vec4 curValue = vec4(0);
			vec4 curWeight = vec4(1);
			for ( uint path = 0; path <= NUM_PATHS; ++path ) {
				vec4 stepValue = traceStep( surface.ray );
				curValue += stepValue * curWeight;

				if ( !payload.hit ) break;

				surface.ray.origin = surface.position.world;
				surface.ray.direction = samplingHemisphere( prngSeed, surface.normal.world );
				curWeight *= surface.material.albedo * dot( surface.ray.direction, surface.normal.world );
				
				if ( length(curWeight) < 0.01 ) break;
			}
			outFrag += curValue;
		}
	}

	{
		outFrag /= SAMPLES;
		outFrag.a = 1;

	#if BLOOM
		float brightness = dot(surface.fragment.rgb, vec3(0.2126, 0.7152, 0.0722));
		outFragBright = brightness > ubo.brightnessThreshold ? vec4(surface.fragment.rgb, 1.0) : vec4(0, 0, 0, 1);
	#endif
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
	}

	if ( ubo.frameNumber == 0 ) {
		imageStore(outImage, ivec2(gl_LaunchIDEXT.xy), outFrag);
	} else {
	//	if ( length(outFrag.rgb) < 0.01f ) return;
		vec4 blended = mix(imageLoad(outImage, ivec2(gl_LaunchIDEXT.xy)), outFrag, BLEND_FACTOR);

		imageStore(outImage, ivec2(gl_LaunchIDEXT.xy), blended);
	}
}

	/*
		{
			vec4 curValue = vec4(0);
			vec4 curWeight = vec4(1);
			for ( uint path = 0; path < NUM_PATHS; ++path ) {
				trace( surface.ray );

				if ( !payload.hit ) {
					if ( path == 0 && 0 <= ubo.indexSkybox && ubo.indexSkybox < CUBEMAPS ) {
						curValue = texture( samplerCubemaps[ubo.indexSkybox], surface.ray.direction );
					}
					break;
				}

				setupSurface( payload );
				directLighting();
				curValue += surface.fragment * curWeight;

				surface.ray.origin = surface.position.world;
				if ( !false ) {
					surface.ray.direction = reflect( surface.ray.direction, surface.normal.world );
					curWeight *= (1.0 - surface.material.roughness) * surface.material.albedo * dot( surface.ray.direction, surface.normal.world );
				} else {
					surface.ray.direction = samplingHemisphere( prngSeed, surface.normal.world );
					curWeight *= surface.material.albedo * dot( surface.ray.direction, surface.normal.world );
				}
				

				if ( length(curWeight) < 0.01 ) break;
			}
			outFrag += curValue;
		}
	*/