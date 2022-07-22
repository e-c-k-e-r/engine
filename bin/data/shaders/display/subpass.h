#extension GL_EXT_samplerless_texture_functions : require
#extension GL_EXT_nonuniform_qualifier : enable

#if RAYTRACE
	#extension GL_EXT_ray_query : require
#endif

#define DEFERRED 1
#define MAX_TEXTURES TEXTURES
//#define TEXTURE_WORKAROUND 0

#include "../common/macros.h"

layout (constant_id = 0) const uint TEXTURES = 512;
layout (constant_id = 1) const uint CUBEMAPS = 128;
#if VXGI
	layout (constant_id = 2) const uint CASCADES = 16;
#endif

#if !MULTISAMPLING
	layout (input_attachment_index = 0, binding = 0) uniform usubpassInput samplerId;
	layout (input_attachment_index = 1, binding = 1) uniform usubpassInput samplerBary;
	layout (input_attachment_index = 2, binding = 2) uniform subpassInput samplerMips;
	layout (input_attachment_index = 3, binding = 3) uniform subpassInput samplerNormal;
	layout (input_attachment_index = 4, binding = 4) uniform subpassInput samplerUv;
	layout (input_attachment_index = 5, binding = 5) uniform subpassInput samplerDepth;
#else
	layout (input_attachment_index = 0, binding = 0) uniform usubpassInputMS samplerId;
	layout (input_attachment_index = 1, binding = 1) uniform usubpassInputMS samplerBary;
	layout (input_attachment_index = 2, binding = 2) uniform subpassInputMS samplerMips;
	layout (input_attachment_index = 3, binding = 3) uniform subpassInputMS samplerNormal;
	layout (input_attachment_index = 4, binding = 4) uniform subpassInputMS samplerUv;
	layout (input_attachment_index = 5, binding = 5) uniform subpassInputMS samplerDepth;
#endif

#include "../common/structs.h"

layout (binding = 6) uniform UBO {
	EyeMatrices eyes[2];

	Settings settings;
} ubo;
/*
*/
layout (std140, binding = 7) readonly buffer DrawCommands {
	DrawCommand drawCommands[];
};
layout (std140, binding = 8) readonly buffer Instances {
	Instance instances[];
};
layout (std140, binding = 9) readonly buffer InstanceAddresseses {
	InstanceAddresses instanceAddresses[];
};
layout (std140, binding = 10) readonly buffer Materials {
	Material materials[];
};
layout (std140, binding = 11) readonly buffer Textures {
	Texture textures[];
};
layout (std140, binding = 12) readonly buffer Lights {
	Light lights[];
};

layout (binding = 13) uniform sampler2D samplerTextures[TEXTURES];
layout (binding = 14) uniform samplerCube samplerCubemaps[CUBEMAPS];
layout (binding = 15) uniform sampler3D samplerNoise;
#if VXGI
	layout (binding = 16) uniform usampler3D voxelId[CASCADES];
	layout (binding = 17) uniform sampler3D voxelNormal[CASCADES];
	layout (binding = 18) uniform sampler3D voxelRadiance[CASCADES];
#endif
#if RAYTRACE
	layout (binding = 19) uniform accelerationStructureEXT tlas;
#endif

layout (location = 0) in vec2 inUv;
layout (location = 1) in flat uvec2 inPushConstantPass;

layout (location = 0) out vec4 outFragColor;
layout (location = 1) out vec4 outFragBright;

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
#include "../common/fog.h"
#include "../common/light.h"
#include "../common/shadows.h"
#if VXGI
	#include "../common/vxgi.h"
#endif

void postProcess() {
	float brightness = dot(surface.fragment.rgb, vec3(0.2126, 0.7152, 0.0722));
//	outFragBright = brightness > ubo.settings.bloom.brightnessThreshold ? vec4(surface.fragment.rgb, 1.0) : vec4(0, 0, 0, 1);

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
	
	outFragColor = vec4(surface.fragment.rgb, 1.0);
}

void populateSurface() {
	surface.fragment = vec4(0);
	surface.pass = inPushConstantPass.x;

	{
	#if !MULTISAMPLING
		const float depth = subpassLoad(samplerDepth).r;
	#else
		const float depth = subpassLoad(samplerDepth, msaa.currentID).r; // resolve(samplerDepth, ubo.settings.mode.msaa).r;
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
	const uvec3 ID = subpassLoad(samplerId).xyz;
#else
	surface.normal.world = decodeNormals( subpassLoad(samplerNormal, msaa.currentID).xy ); // decodeNormals( resolve(samplerNormal, ubo.settings.mode.msaa).xy );
	const uvec3 ID = msaa.IDs[msaa.currentID]; // subpassLoad(samplerId, msaa.currentID).xy; //resolve(samplerId, ubo.settings.mode.msaa).xy;
#endif
	surface.normal.eye = vec3( ubo.eyes[surface.pass].view * vec4(surface.normal.world, 0.0) );

	if ( ID.x == 0 || ID.y == 0 || ID.z == 0 ) {
		if ( 0 <= ubo.settings.lighting.indexSkybox && ubo.settings.lighting.indexSkybox < CUBEMAPS ) {
			surface.fragment.rgb = texture( samplerCubemaps[ubo.settings.lighting.indexSkybox], surface.ray.direction ).rgb;
		}
		surface.fragment.a = 0.0;
		postProcess();
		return;
	}
	const uint drawID = ID.x - 1;
	const uint triangleID = ID.y - 1;
	const uint instanceID = ID.z - 1;

	const DrawCommand drawCommand = drawCommands[drawID];
	const Instance instance = instances[instanceID];
	surface.instance = instance;

	const Material material = materials[surface.instance.materialID];
	surface.material.albedo = material.colorBase;
	surface.material.metallic = material.factorMetallic;
	surface.material.roughness = material.factorRoughness;
	surface.material.occlusion = material.factorOcclusion;
	surface.fragment = material.colorEmissive;

	{
	#if !MULTISAMPLING
		const vec4 uv = subpassLoad(samplerUv);
		const vec2 mips = subpassLoad(samplerMips).xy;
	#else
		const vec4 uv = subpassLoad(samplerUv, msaa.currentID); // resolve(samplerUv, ubo.settings.mode.msaa);
		const vec2 mips = subpassLoad(samplerMips, msaa.currentID).xy; // resolve(samplerUv, ubo.settings.mode.msaa);
	#endif
		surface.uv.xy = uv.xy;
		surface.uv.z = mips.x;
		surface.st.xy = uv.zw;
		surface.st.z = mips.y;
	}

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
	if ( bool(ubo.settings.lighting.useLightmaps) && validTextureIndex( surface.instance.lightmapID ) ) {
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
}

void directLighting() {
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
}

#if MULTISAMPLING
void resolveSurfaceFragment() {
	for ( int i = 0; i < ubo.settings.mode.msaa; ++i ) {
		msaa.currentID = i;
		msaa.IDs[i] = subpassLoad(samplerId, msaa.currentID).xyz;

		// check if ID is already used
		bool unique = true;
		for ( int j = msaa.currentID - 1; j >= 0; --j ) {
			if ( msaa.IDs[j] == msaa.IDs[i] ) {
				surface.fragment = msaa.fragments[j];
				unique = false;
				break;
			}
		}

		if ( unique ) {
			populateSurface();
		#if VXGI
			indirectLighting();
		#endif
			directLighting();
		}

		msaa.fragment += surface.fragment;
		msaa.fragments[msaa.currentID] = surface.fragment;
	}
	
	surface.fragment = msaa.fragment / ubo.settings.mode.msaa;
}
#endif