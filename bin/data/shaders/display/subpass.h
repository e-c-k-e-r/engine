#extension GL_EXT_samplerless_texture_functions : require
#extension GL_EXT_nonuniform_qualifier : enable

#define MAX_TEXTURES TEXTURES
#include "../common/macros.h"

layout (constant_id = 0) const uint TEXTURES = 512;
#if VXGI
	layout (constant_id = 1) const uint CASCADES = 16;
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
	Matrices matrices;

	Mode mode;
	Fog fog;

	uint lights;
	uint materials;
	uint textures;
	uint drawCalls;
	
	vec3 ambient;
	float gamma;

	float exposure;
	uint msaa;
	uint shadowSamples;
	float cascadePower;
} ubo;

layout (std140, binding = 5) readonly buffer Lights {
	Light lights[];
};
layout (std140, binding = 6) readonly buffer Materials {
	Material materials[];
};
layout (std140, binding = 7) readonly buffer Textures {
	Texture textures[];
};
layout (std140, binding = 8) readonly buffer DrawCalls {
	DrawCall drawCalls[];
};

#if VXGI
	layout (binding = 9) uniform usampler3D voxelId[CASCADES];
	layout (binding = 10) uniform sampler3D voxelUv[CASCADES];
	layout (binding = 11) uniform sampler3D voxelNormal[CASCADES];
	layout (binding = 12) uniform sampler3D voxelRadiance[CASCADES];

	layout (binding = 13) uniform sampler3D samplerNoise;
	layout (binding = 14) uniform samplerCube samplerSkybox;
	layout (binding = 15) uniform sampler2D samplerTextures[TEXTURES];
#else
	layout (binding = 9) uniform sampler3D samplerNoise;
	layout (binding = 10) uniform samplerCube samplerSkybox;
	layout (binding = 11) uniform sampler2D samplerTextures[TEXTURES];
#endif

layout (location = 0) in vec2 inUv;
layout (location = 1) in flat uint inPushConstantPass;

layout (location = 0) out vec4 outFragColor;

#include "../common/functions.h"
#include "../common/fog.h"
#include "../common/pbr.h"
#if VXGI
	#include "../common/vxgi.h"
#endif
#include "../common/shadows.h"

void postProcess() {
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
	if ( (ubo.mode.type & (0x1 << 1)) == (0x1 << 1) ) whitenoise(surface.fragment.rgb, ubo.mode.parameters);
#endif

	outFragColor = vec4(surface.fragment.rgb,1);
}

void populateSurface() {
	surface.pass = inPushConstantPass;
	{
	#if !MULTISAMPLING
		const float depth = subpassLoad(samplerDepth).r;
	#else
		const float depth = resolve(samplerDepth, ubo.msaa).r;
	#endif

		vec4 positionEye = ubo.matrices.iProjection[surface.pass] * vec4(inUv * 2.0 - 1.0, depth, 1.0);
		positionEye /= positionEye.w;
		surface.position.eye = positionEye.xyz;
		surface.position.world = vec3( ubo.matrices.iView[surface.pass] * positionEye );
	}
#if 0
	{
		const vec4 near4 = ubo.matrices.iProjectionView[surface.pass] * (vec4(2.0 * inUv - 1.0, -1.0, 1.0));
		const vec4 far4 = ubo.matrices.iProjectionView[surface.pass] * (vec4(2.0 * inUv - 1.0, 1.0, 1.0));
		const vec3 near3 = near4.xyz / near4.w;
		const vec3 far3 = far4.xyz / far4.w;

		surface.ray.origin = near3;
		surface.ray.direction = normalize( far3 - near3 );
	}
	// separate our ray direction due to floating point precision problems
	{
		const mat4 iProjectionView = inverse( ubo.matrices.projection[surface.pass] * mat4(mat3(ubo.matrices.view[surface.pass])) );
		const vec4 near4 = iProjectionView * (vec4(2.0 * inUv - 1.0, -1.0, 1.0));
		const vec4 far4 = iProjectionView * (vec4(2.0 * inUv - 1.0, 1.0, 1.0));
		const vec3 near3 = near4.xyz / near4.w;
		const vec3 far3 = far4.xyz / far4.w;

		surface.ray.direction = normalize( far3 - near3 );
	}
#else
	{
		const mat4 iProjectionView = inverse( ubo.matrices.projection[surface.pass] * mat4(mat3(ubo.matrices.view[surface.pass])) );
		const vec4 near4 = iProjectionView * (vec4(2.0 * inUv - 1.0, -1.0, 1.0));
		const vec4 far4 = iProjectionView * (vec4(2.0 * inUv - 1.0, 1.0, 1.0));
		const vec3 near3 = near4.xyz / near4.w;
		const vec3 far3 = far4.xyz / far4.w;

		surface.ray.direction = normalize( far3 - near3 );
		surface.ray.origin = ubo.matrices.eyePos[surface.pass].xyz;
	}
#endif
#if !MULTISAMPLING
	surface.normal.world = decodeNormals( subpassLoad(samplerNormal).xy );
	const uvec2 ID = subpassLoad(samplerId).xy;
#else
	surface.normal.world = decodeNormals( resolve(samplerNormal, ubo.msaa).xy );
	const uvec2 ID = subpassLoad(samplerId, 0).xy; //resolve(samplerId, ubo.msaa).xy;
#endif
	surface.normal.eye = vec3( ubo.matrices.view[surface.pass] * vec4(surface.normal.world, 0.0) );

	if ( ID.x == 0 || ID.y == 0 ) {
		surface.fragment.rgb = texture( samplerSkybox, surface.ray.direction ).rgb;
		surface.fragment.a = 0.0;
		postProcess();
		return;
	}
	const uint drawId = ID.x - 1;
	const DrawCall drawCall = drawCalls[drawId];
	surface.material.id = ID.y + drawCall.materialIndex - 1;
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
	const bool useAtlas = validTextureIndex( drawCall.textureIndex + material.indexAtlas );
	Texture textureAtlas;
	if ( useAtlas ) textureAtlas = textures[drawCall.textureIndex + material.indexAtlas];
	if ( validTextureIndex( drawCall.textureIndex + material.indexAlbedo ) ) {
		const Texture t = textures[drawCall.textureIndex + material.indexAlbedo];
		surface.material.albedo = textureLod( samplerTextures[nonuniformEXT((useAtlas)?textureAtlas.index:t.index)], ( useAtlas ) ? mix( t.lerp.xy, t.lerp.zw, surface.uv ) : surface.uv, mip );
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
	if ( validTextureIndex( drawCall.textureIndex + material.indexEmissive ) ) {
		const Texture t = textures[drawCall.textureIndex + material.indexEmissive];
		surface.fragment += textureLod( samplerTextures[nonuniformEXT((useAtlas)?textureAtlas.index:t.index)], ( useAtlas ) ? mix( t.lerp.xy, t.lerp.zw, surface.uv ) : surface.uv, mip );
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