#extension GL_EXT_samplerless_texture_functions : require
#extension GL_EXT_nonuniform_qualifier : enable

#define DEFERRED 1
#define MAX_TEXTURES TEXTURES
#include "../common/macros.h"

layout (constant_id = 0) const uint TEXTURES = 512;
layout (constant_id = 1) const uint CUBEMAPS = 128;
#if VXGI
	layout (constant_id = 2) const uint CASCADES = 16;
#endif

#if !MULTISAMPLING
	layout (input_attachment_index = 0, binding = 0) uniform usubpassInput samplerId;
	layout (input_attachment_index = 1, binding = 1) uniform subpassInput samplerNormal;
	#if DEFERRED_SAMPLING
		layout (input_attachment_index = 2, binding = 2) uniform subpassInput samplerUv;
	#else
		layout (input_attachment_index = 2, binding = 2) uniform subpassInput samplerAlbedo;
	#endif
	layout (input_attachment_index = 3, binding = 3) uniform subpassInput samplerDepth;
#else
	layout (input_attachment_index = 0, binding = 0) uniform usubpassInputMS samplerId;
	layout (input_attachment_index = 1, binding = 1) uniform subpassInputMS samplerNormal;
	#if DEFERRED_SAMPLING
		layout (input_attachment_index = 2, binding = 2) uniform subpassInputMS samplerUv;
	#else
		layout (input_attachment_index = 2, binding = 2) uniform subpassInputMS samplerAlbedo;
	#endif
	layout (input_attachment_index = 3, binding = 3) uniform subpassInputMS samplerDepth;
#endif

#include "../common/structs.h"

layout (binding = 4) uniform UBO {
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
#if VXGI
	layout (binding = 12) uniform usampler3D voxelId[CASCADES];
	layout (binding = 13) uniform sampler3D voxelNormal[CASCADES];
	layout (binding = 14) uniform sampler3D voxelRadiance[CASCADES];
#endif

layout (location = 0) in vec2 inUv;
layout (location = 1) in flat uvec2 inPushConstantPass;

layout (location = 0) out vec4 outFragColor;
layout (location = 1) out vec4 outFragBright;

#include "../common/functions.h"
#include "../common/fog.h"
#include "../common/pbr.h"
#include "../common/shadows.h"
#if VXGI
	#include "../common/vxgi.h"
#endif

void postProcess() {
	float brightness = dot(surface.fragment.rgb, vec3(0.2126, 0.7152, 0.0722));
	outFragBright = brightness > ubo.brightnessThreshold ? vec4(surface.fragment.rgb, 1.0) : vec4(0, 0, 0, 1);

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
	
	outFragColor = vec4(surface.fragment.rgb, 1.0);
}

void populateSurface() {
	surface.pass = inPushConstantPass.x;
	{
	#if !MULTISAMPLING
		const float depth = subpassLoad(samplerDepth).r;
	#else
		const float depth = resolve(samplerDepth, ubo.msaa).r;
	#endif

		vec4 positionEye = ubo.eyes[surface.pass].iProjection * vec4(inUv * 2.0 - 1.0, depth, 1.0);
		positionEye /= positionEye.w;
		surface.position.eye = positionEye.xyz;
		surface.position.world = vec3( ubo.eyes[surface.pass].iView * positionEye );
	}
#if 0
	{
		const vec4 near4 = ubo.eyes[surface.pass].iProjectionView * (vec4(2.0 * inUv - 1.0, -1.0, 1.0));
		const vec4 far4 = ubo.eyes[surface.pass].iProjectionView * (vec4(2.0 * inUv - 1.0, 1.0, 1.0));
		const vec3 near3 = near4.xyz / near4.w;
		const vec3 far3 = far4.xyz / far4.w;

		surface.ray.origin = near3;
		surface.ray.direction = normalize( far3 - near3 );
	}
	// separate our ray direction due to floating point precision problems
	{
		const mat4 iProjectionView = inverse( ubo.eyes[surface.pass].projection * mat4(mat3(ubo.eyes[surface.pass].view)) );
		const vec4 near4 = iProjectionView * (vec4(2.0 * inUv - 1.0, -1.0, 1.0));
		const vec4 far4 = iProjectionView * (vec4(2.0 * inUv - 1.0, 1.0, 1.0));
		const vec3 near3 = near4.xyz / near4.w;
		const vec3 far3 = far4.xyz / far4.w;

		surface.ray.direction = normalize( far3 - near3 );
	}
#else
	{
		const mat4 iProjectionView = inverse( ubo.eyes[surface.pass].projection * mat4(mat3(ubo.eyes[surface.pass].view)) );
		const vec4 near4 = iProjectionView * (vec4(2.0 * inUv - 1.0, -1.0, 1.0));
		const vec4 far4 = iProjectionView * (vec4(2.0 * inUv - 1.0, 1.0, 1.0));
		const vec3 near3 = near4.xyz / near4.w;
		const vec3 far3 = far4.xyz / far4.w;

		surface.ray.direction = normalize( far3 - near3 );
		surface.ray.origin = ubo.eyes[surface.pass].eyePos.xyz;
	}
#endif
#if !MULTISAMPLING
	surface.normal.world = decodeNormals( subpassLoad(samplerNormal).xy );
	const uvec2 ID = subpassLoad(samplerId).xy;
#else
	surface.normal.world = decodeNormals( resolve(samplerNormal, ubo.msaa).xy );
	const uvec2 ID = subpassLoad(samplerId, 0).xy; //resolve(samplerId, ubo.msaa).xy;
#endif
	surface.normal.eye = vec3( ubo.eyes[surface.pass].view * vec4(surface.normal.world, 0.0) );

	const uint drawID = ID.x - 1;
	const uint instanceID = ID.y - 1;
	if ( ID.x == 0 || ID.y == 0 ) {
		surface.fragment.rgb = texture( samplerCubemaps[ubo.indexSkybox], surface.ray.direction ).rgb;
		surface.fragment.a = 0.0;
		postProcess();
		return;
	}
//	const DrawCommand drawCommand = drawCommands[drawID];
	const Instance instance = instances[instanceID];
	surface.material.id = instance.materialID;
	const Material material = materials[surface.material.id];
	surface.material.albedo = material.colorBase;
	surface.fragment = material.colorEmissive;
#if DEFERRED_SAMPLING
#if !MULTISAMPLING
	surface.uv = subpassLoad(samplerUv).xy;
#else
	surface.uv = resolve(samplerUv, ubo.msaa).xy;
#endif
	const float mip = mipLevel(inUv.xy);
	if ( validTextureIndex( material.indexAlbedo ) ) {
		surface.material.albedo = sampleTexture( material.indexAlbedo, mip );
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
		surface.fragment += sampleTexture( material.indexEmissive, mip );
	}
#else
#if !MULTISAMPLING
	surface.material.albedo = subpassLoad(samplerAlbedo);
#else
	surface.material.albedo = resolve(samplerAlbedo, ubo.msaa);
#endif
#endif
	surface.material.metallic = material.factorMetallic;
	surface.material.roughness = material.factorRoughness;
	surface.material.occlusion = material.factorOcclusion;
	surface.material.indexLightmap = material.indexLightmap;
}

void directLighting() {
	const vec3 ambient = ubo.ambient.rgb * surface.material.occlusion + surface.material.indirect.rgb;
	surface.fragment.rgb += (0 <= surface.material.indexLightmap) ? (surface.material.albedo.rgb + ambient) : (surface.material.albedo.rgb * ambient);
	if ( ubo.lights == 0 ) { surface.fragment.rgb = surface.material.albedo.rgb; return; }
#if PBR
	pbr();
#elif LAMBERT
	lambert();
#elif PHONG
	phong();
#endif
}