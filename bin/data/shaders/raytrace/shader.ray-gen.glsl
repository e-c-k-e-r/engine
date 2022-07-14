#version 460

#extension GL_EXT_ray_tracing : enable
#extension GL_ARB_shader_clock : enable

#pragma shader_stage(raygen)
layout (constant_id = 0) const uint PASSES = 2;
layout (constant_id = 1) const uint TEXTURES = 512;
layout (constant_id = 2) const uint CUBEMAPS = 8;
layout (constant_id = 3) const uint CASCADES = 1;

#define COMPUTE 1
#define PBR 1
#define VXGI 0
#define RAYTRACE 1
#define BUFFER_REFERENCE 1
#define FOG 0
#define BLOOM 0
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

	Settings settings;
} ubo;

layout (std140, binding = 3) readonly buffer Instances {
	Instance instances[];
};
layout (std140, binding = 4) readonly buffer InstanceAddresseses {
	InstanceAddresses instanceAddresses[];
};
layout (std140, binding = 5) readonly buffer Materials {
	Material materials[];
};
layout (std140, binding = 6) readonly buffer Textures {
	Texture textures[];
};
layout (std140, binding = 7) readonly buffer Lights {
	Light lights[];
};

layout (binding = 8) uniform sampler2D samplerTextures[TEXTURES];
layout (binding = 9) uniform samplerCube samplerCubemaps[CUBEMAPS];
layout (binding = 10) uniform sampler3D samplerNoise;
#if VXGI
	layout (binding = 11) uniform usampler3D voxelId[CASCADES];
	layout (binding = 12) uniform sampler3D voxelNormal[CASCADES];
	layout (binding = 13) uniform sampler3D voxelRadiance[CASCADES];
#endif

layout (location = 0) rayPayloadEXT RayTracePayload payload;

layout(buffer_reference, scalar) buffer Vertices { Vertex v[]; };
layout(buffer_reference, scalar) buffer Indices { uvec3 i[]; };
layout(buffer_reference, scalar) buffer Indirects { DrawCommand dc[]; };

layout(buffer_reference, scalar) buffer VPos { vec3 v[]; };
layout(buffer_reference, scalar) buffer VUv { vec2 v[]; };
layout(buffer_reference, scalar) buffer VColor { uint v[]; };
layout(buffer_reference, scalar) buffer VSt { vec2 v[]; };
layout(buffer_reference, scalar) buffer VNormal { vec3 v[]; };
layout(buffer_reference, scalar) buffer VTangent { vec3 v[]; };
layout(buffer_reference, scalar) buffer VID { uint v[]; };

#include "../common/functions.h"
#include "../common/light.h"
#include "../common/fog.h"
#if VXGI
	#include "../common/vxgi.h"
#endif

Triangle parsePayload( RayTracePayload payload ) {
	Triangle triangle;
	triangle.instanceID = payload.instanceID;

	if ( !payload.hit ) return triangle;

	const vec3 bary = vec3(
		1.0 - payload.attributes.x - payload.attributes.y,
		payload.attributes.x,
		payload.attributes.y
	);
	const InstanceAddresses instanceAddresses = instanceAddresses[triangle.instanceID];

	if ( !(0 < instanceAddresses.index) ) return triangle;
	
	const DrawCommand drawCommand = Indirects(nonuniformEXT(instanceAddresses.indirect)).dc[instanceAddresses.drawID];
	const uint triangleID = payload.primitiveID + (drawCommand.indexID / 3);

	Vertex points[3];
	uvec3 indices = Indices(nonuniformEXT(instanceAddresses.index)).i[triangleID];
	for ( uint _ = 0; _ < 3; ++_ ) /*triangle.*/indices[_] += drawCommand.vertexID;

	if ( 0 < instanceAddresses.vertex ) {
		Vertices vertices = Vertices(nonuniformEXT(instanceAddresses.vertex));
		for ( uint _ = 0; _ < 3; ++_ ) /*triangle.*/points[_] = vertices.v[/*triangle.*/indices[_]];
	} else {
		if ( 0 < instanceAddresses.position ) {
			VPos buf = VPos(nonuniformEXT(instanceAddresses.position));
			for ( uint _ = 0; _ < 3; ++_ ) /*triangle.*/points[_].position = buf.v[/*triangle.*/indices[_]];
		}
		if ( 0 < instanceAddresses.uv ) {
			VUv buf = VUv(nonuniformEXT(instanceAddresses.uv));
			for ( uint _ = 0; _ < 3; ++_ ) /*triangle.*/points[_].uv = buf.v[/*triangle.*/indices[_]];
		}
		if ( 0 < instanceAddresses.st ) {
			VSt buf = VSt(nonuniformEXT(instanceAddresses.st));
			for ( uint _ = 0; _ < 3; ++_ ) /*triangle.*/points[_].st = buf.v[/*triangle.*/indices[_]];
		}
		if ( 0 < instanceAddresses.normal ) {
			VNormal buf = VNormal(nonuniformEXT(instanceAddresses.normal));
			for ( uint _ = 0; _ < 3; ++_ ) /*triangle.*/points[_].normal = buf.v[/*triangle.*/indices[_]];
		}
		if ( 0 < instanceAddresses.tangent ) {
			VTangent buf = VTangent(nonuniformEXT(instanceAddresses.tangent));
			for ( uint _ = 0; _ < 3; ++_ ) /*triangle.*/points[_].tangent = buf.v[/*triangle.*/indices[_]];
		}
	}

	triangle.point.position = /*triangle.*/points[0].position * bary[0] + /*triangle.*/points[1].position * bary[1] + /*triangle.*/points[2].position * bary[2];
	triangle.point.uv = /*triangle.*/points[0].uv * bary[0] + /*triangle.*/points[1].uv * bary[1] + /*triangle.*/points[2].uv * bary[2];
	triangle.point.st = /*triangle.*/points[0].st * bary[0] + /*triangle.*/points[1].st * bary[1] + /*triangle.*/points[2].st * bary[2];
	triangle.point.normal = /*triangle.*/points[0].normal * bary[0] + /*triangle.*/points[1].normal * bary[1] + /*triangle.*/points[2].normal * bary[2];
	triangle.point.tangent = /*triangle.*/points[0].tangent * bary[0] + /*triangle.*/points[1].tangent * bary[1] + /*triangle.*/points[2].tangent * bary[2];
	
	triangle.geomNormal = normalize(cross(points[1].position - points[0].position, points[2].position - points[0].position));

	return triangle;
}

void trace( Ray ray, float tMin, float tMax ) {
	uint rayFlags = gl_RayFlagsOpaqueEXT;
	uint cullMask = 0xFF;
	
	payload.hit = false;
	traceRayEXT(tlas, rayFlags, cullMask, 0, 0, 0, ray.origin, tMin, ray.direction, tMax, 0);
}
void trace( Ray ray, float tMin ) {
	uint rayFlags = gl_RayFlagsOpaqueEXT;
	uint cullMask = 0xFF;

	float tMax = ubo.settings.rt.defaultRayBounds.y;
	
	payload.hit = false;
	traceRayEXT(tlas, rayFlags, cullMask, 0, 0, 0, ray.origin, tMin, ray.direction, tMax, 0);
}

void trace( Ray ray ) {
	trace( ray, ubo.settings.rt.defaultRayBounds.x, ubo.settings.rt.defaultRayBounds.y );
}

float shadowFactor( const Light light, float def ) {
	Ray ray;
	ray.origin = surface.position.world;
	ray.direction = light.position - ray.origin;

	float tMin = ubo.settings.rt.defaultRayBounds.x;
	float tMax = length(ray.direction) - 0.0001;
	
	ray.direction = normalize(ray.direction);

	uint rayFlags = gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT;
	uint cullMask = 0xFF;

	payload.hit = true;
	traceRayEXT(tlas, rayFlags, cullMask, 0, 0, 0, ray.origin, tMin, ray.direction, tMax, 0);

	return payload.hit ? 0.0 : 1.0;
}

void setupSurface( RayTracePayload payload ) {
	const Triangle triangle = parsePayload( payload );
	const Instance instance = instances[triangle.instanceID];
	surface.instance = instance;
	surface.fragment = vec4(0);
	surface.light = vec4(0);

	// bind position
	{
		surface.position.world = vec3( instance.model * vec4(triangle.point.position, 1.0 ) );
		surface.position.eye = vec3( ubo.eyes[surface.pass].view * vec4(surface.position.world, 1.0) );
	}
	// bind normals
	{
		surface.normal.world = normalize(vec3( instance.model * vec4(triangle.point.normal, 0.0 ) ));
	//	surface.normal.world = faceforward( surface.normal.world, surface.ray.direction, surface.normal.world );
	//	surface.normal.eye = normalize(vec3( ubo.eyes[surface.pass].view * vec4(surface.normal.world, 0.0) ));

	//	surface.tbn[0] = normalize(vec3( instance.model * vec4(triangle.tbn[0], 0.0 ) ));
	//	surface.tbn[1] = normalize(vec3( instance.model * vec4(triangle.tbn[1], 0.0 ) ));
	//	surface.tbn[2] = surface.normal.world;

		vec3 tangent = normalize(vec3( instance.model * vec4(triangle.point.tangent, 0.0) ));
		vec3 bitangent = normalize(vec3( instance.model * vec4(cross( triangle.point.normal, triangle.point.tangent ), 0.0) ));
		if ( triangle.point.tangent != vec3(0) ) {
			surface.tbn = mat3(tangent, bitangent, triangle.point.normal);
		} else {
			surface.tbn = mat3(1);
		}
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
	if ( (surface.subID++ > 0 || bool(ubo.settings.lighting.useLightmaps)) && validTextureIndex( surface.instance.lightmapID ) ) {
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

	surface.light.rgb += surface.material.albedo.rgb * ubo.settings.lighting.ambient.rgb * surface.material.occlusion; // add ambient lighting
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
		} else if (  0 <= ubo.settings.lighting.indexSkybox && ubo.settings.lighting.indexSkybox < CUBEMAPS ) {
			outFrag = texture( samplerCubemaps[ubo.settings.lighting.indexSkybox], ray.direction );
		} else {
			outFrag = vec4(ubo.settings.lighting.ambient.rgb, 0);
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
		trace( transparency, ubo.settings.rt.alphaTestOffset );
		if ( payload.hit ) {
			setupSurface( payload );
			directLighting();
			transparencyColor *= surface.fragment;
		} else if (  0 <= ubo.settings.lighting.indexSkybox && ubo.settings.lighting.indexSkybox < CUBEMAPS ) {
			transparencyColor *= texture( samplerCubemaps[ubo.settings.lighting.indexSkybox], ray.direction );
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
			} else if (  0 <= ubo.settings.lighting.indexSkybox && ubo.settings.lighting.indexSkybox < CUBEMAPS ) {
				reflectionColor *= texture( samplerCubemaps[ubo.settings.lighting.indexSkybox], reflection.direction );
			}

			outFrag += reflectionColor;

			payload = surfacePayload;
			setupSurface( payload );
		}
	}

	return outFrag;
}

void main()  {
//	if ( ubo.settings.mode.frameNumber > 16 ) return;	
//	prngSeed = tea(gl_LaunchIDEXT.y * gl_LaunchSizeEXT.x + gl_LaunchIDEXT.x, ubo.settings.mode.frameNumber);
	prngSeed = tea(gl_LaunchIDEXT.y * gl_LaunchSizeEXT.x + gl_LaunchIDEXT.x, int(clockARB()));
	surface.pass = PushConstant.pass;
	surface.subID = 0;
	vec4 outFrag = vec4(0);

	const uint SAMPLES = min(ubo.settings.rt.samples, 4);
	const uint NUM_PATHS = min(ubo.settings.rt.paths, 8);
#if 1
	const uint FRAME_ACCUMULATION_VALUE = ubo.settings.rt.frameAccumulationMinimum > 0 ? min(ubo.settings.rt.frameAccumulationMinimum, ubo.settings.mode.frameNumber + 1) : ubo.settings.mode.frameNumber + 1;
#else
	const uint FRAME_ACCUMULATION_VALUE = min(32, ubo.settings.mode.frameNumber + 1);
#endif
	const float BLEND_FACTOR = 1.0f / float(FRAME_ACCUMULATION_VALUE);
	uint FRAME_NUMBER = ubo.settings.mode.frameNumber;

#if 0
	for ( uint samp = 0; samp < SAMPLES; ++samp, ++FRAME_NUMBER ) {
		{
			const vec2 center = ( FRAME_NUMBER > 0 ) ? vec2( rnd(), rnd() ) : vec2(0.5);
			const vec2 inUv = (vec2(gl_LaunchIDEXT.xy) + center) / vec2(gl_LaunchSizeEXT.xy);
		#if 1
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
			for ( uint path = 0; path < NUM_PATHS; ++path ) {
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
	}
#elif 0
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
		for ( uint path = 0; path < NUM_PATHS; ++path ) {
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
	{
		surface.fragment = outFrag;
	}
#else
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
		surface.fragment = traceStep( surface.ray );
	}
#endif
	{
	#if BLOOM
		float brightness = dot(surface.fragment.rgb, vec3(0.2126, 0.7152, 0.0722));
		outFragBright = brightness > ubo.brightnessThreshold ? vec4(surface.fragment.rgb, 1.0) : vec4(0, 0, 0, 1);
	#endif
	#if FOG
		fog( surface.ray, surface.fragment.rgb, surface.fragment.a );
	#endif
	#if TONE_MAP
		surface.fragment.rgb = vec3(1.0) - exp(-surface.fragment.rgb * ubo.settings.bloom.exposure);
	#endif
	#if GAMMA_CORRECT
		surface.fragment.rgb = pow(surface.fragment.rgb, vec3(1.0 / ubo.settings.bloom.gamma));
	#endif
	#if WHITENOISE
		if ( enabled(ubo.settings.mode.type, 1) ) whitenoise(surface.fragment.rgb, ubo.settings.mode.parameters);
	#endif
	}

	{
		outFrag = surface.fragment;
		outFrag.a = 1;
	}

	if ( ubo.settings.mode.frameNumber == 0 ) {
		imageStore(outImage, ivec2(gl_LaunchIDEXT.xy), outFrag);
	} else {
	//	if ( length(outFrag.rgb) < 0.01f ) return;
		vec4 blended = mix(imageLoad(outImage, ivec2(gl_LaunchIDEXT.xy)), outFrag, BLEND_FACTOR);

		imageStore(outImage, ivec2(gl_LaunchIDEXT.xy), blended);
	}
}