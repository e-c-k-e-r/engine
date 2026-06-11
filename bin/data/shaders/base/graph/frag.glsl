#version 450
#pragma shader_stage(fragment)

#define FORWARD 0
#define FRAGMENT 1
#define PBR 1

layout (constant_id = 0) const uint TEXTURES = 1;
layout (constant_id = 1) const uint CUBEMAPS = 128;

#include "../../common/macros.h"
#include "../../common/structs.h"

layout (binding = 6, set = 1) uniform sampler2D samplerTextures[TEXTURES];
layout (binding = 7, set = 1) uniform samplerCube samplerCubemaps[CUBEMAPS];
layout (binding = 8, set = 0) uniform sampler3D samplerNoise;
#if VXGI
	layout (binding = 9, set = 0) uniform sampler3D voxelOutput[CASCADES];
#endif
#if RT
	layout (binding = 10, set = 0) uniform accelerationStructureEXT tlas;
#endif
layout (std140, binding = 11, set = 0) readonly buffer DrawCommands { DrawCommand drawCommands[]; };
layout (std140, binding = 12, set = 0) readonly buffer Instances { Instance instances[]; };
layout (std140, binding = 13, set = 0) readonly buffer InstanceAddresseses { InstanceAddresses addresses[]; };
layout (std140, binding = 14, set = 0) readonly buffer Materials { Material materials[]; };
layout (std140, binding = 15, set = 0) readonly buffer Textures { Texture textures[]; };
layout (std140, binding = 16, set = 0) readonly buffer Lights { Light lights[]; };
layout (std140, binding = 17, set = 0) readonly buffer Objects { Object objects[]; };

layout (binding = 18) uniform Camera { Viewport viewport[2]; } camera;

// to-do: bind UBO for settings?
/*
layout (binding = 19) uniform UBO { EyeMatrices eyes[2]; Settings settings; } ubo;
*/

#include "../../common/functions.h"
//#include "../../common/light.h"
//#include "../../common/shadows.h"

layout (location = 0) flat in uvec4 inId;
layout (location = 1) flat in vec4 inPOS0;
layout (location = 2) in vec4 inPOS1;
layout (location = 3) in vec3 inPosition;
layout (location = 4) sample in vec2 inUv;
layout (location = 5) in vec4 inColor;
layout (location = 6) in vec2 inSt;
layout (location = 7) in vec3 inNormal;
layout (location = 8) in vec3 inTangent;

layout (location = 0) out vec4 outFragColor;

void main() {
	const uint triangleID = gl_PrimitiveID;
	const uint drawID = uint(inId.y);
	const uint instanceID = uint(inId.z);

#if 0

#if CAN_DISCARD
	const DrawCommand drawCommand = drawCommands[drawID];
	const Instance instance = instances[instanceID];
	const Material material = materials[instance.materialID];

	surface.uv.xy = wrap(inUv.xy);
	surface.uv.z = mipLevel(dFdx(inUv), dFdy(inUv));

	surface.st.xy = wrap(inSt.xy);
	surface.st.z = mipLevel(dFdx(inSt), dFdy(inSt));

	vec4 A = inColor * material.colorBase;  
	// sample albedo
	if ( validTextureIndex( material.indexAlbedo ) ) {
		A = sampleTexture( material.indexAlbedo );
	}
	if ( A.a < 0.0001 ) discard;
#endif

	outFragColor.rgb = A.rgb;
	outFragColor.a = A.a;
#else
	const DrawCommand drawCommand = drawCommands[drawID];
	surface.instance = instances[instanceID];
	surface.object = objects[surface.instance.objectID];
	const Material material = materials[surface.instance.materialID];

	surface.uv.xy = wrap(inUv.xy);
	surface.st.xy = wrap(inSt.xy);	
	
	surface.dUvDx = dFdx(inUv);
	surface.dUvDy = dFdy(inUv);

	surface.dStDx = dFdx(inSt);
	surface.dStDy = dFdy(inSt);

	surface.uv.z = mipLevel(surface.dUvDx, surface.dUvDy);
	surface.st.z = mipLevel(surface.dStDx, surface.dStDy);

	surface.position.world = inPosition;
	surface.position.eye = vec3( inverse( surface.object.model ) * vec4(surface.position.world, 1.0) );

	surface.normal.world = normalize(inNormal);
	surface.tangent.world = normalize(inTangent);
	vec3 bitangent = cross(surface.normal.world, surface.tangent.world);
	surface.tbn = mat3(surface.tangent.world, bitangent, surface.normal.world);

	// 
	surface.material.albedo = material.colorBase;
	surface.material.metallic = material.factorMetallic;
	surface.material.roughness = material.factorRoughness;
	surface.material.occlusion = material.factorOcclusion;
	surface.material.lightmapped = false;
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

	// EMISSIVE
	} else if ( material.modeAlpha == 3 ) {
		surface.light.rgb *= surface.material.albedo.rgb * surface.material.albedo.a;
		surface.material.albedo.a = 1.0;
	}

	// Emissive mapping
	if ( validTextureIndex( material.indexEmissive ) ) {
		surface.light *= sampleTexture( material.indexEmissive );
	}
	// (Occlusion/)Metallic/Roughness map
	if ( validTextureIndex( material.indexMetallicRoughness ) ) {
		vec4 samp = sampleTexture( material.indexMetallicRoughness );

		surface.material.metallic *= samp.b;
		surface.material.roughness *= samp.g;

		if ( material.indexOcclusion == material.indexMetallicRoughness ) {
			surface.material.occlusion = mix(1.0, samp.r, material.factorOcclusion);
		}
	}
	// Occlusion mapping
	if ( validTextureIndex( material.indexOcclusion ) && material.indexOcclusion != material.indexMetallicRoughness ) {
		float occ = sampleTexture( material.indexOcclusion ).r;
		surface.material.occlusion = mix(1.0, occ, material.factorOcclusion);
	}
	// Normal mapping
	if ( validTextureIndex( material.indexNormal ) && surface.tangent.world != vec3(0) ) {
		surface.normal.world = surface.tbn * normalize( sampleTexture( material.indexNormal ).xyz * 2.0 - vec3(1.0));
	}
	{
		surface.normal.eye = normalize(vec3( inverse( surface.object.model ) * vec4(surface.normal.world, 0.0) ));
	}

	// Light mapping
	if ( /*( bool(ubo.settings.lighting.useLightmaps)) &&*/ validTextureIndex( surface.instance.lightmapID ) ) {
		surface.material.lightmapped = true; // light.a > 0.001;
		vec4 light = decodeRGBE( sampleTexture( surface.instance.lightmapID, surface.st.xy, 0.0 ) );

		const vec3 F0 = mix(vec3(0.04), surface.material.albedo.rgb, surface.material.metallic);
		const vec3 Lo = normalize(-surface.position.eye);
		const float cosLo = max(0.0, dot(surface.normal.eye, Lo));
		const vec3 F = F0 + (max(vec3(1.0 - surface.material.roughness), F0) - F0) * pow(clamp(1.0 - cosLo, 0.0, 1.0), 5.0);
		const vec3 kD = (vec3(1.0) - F) * (1.0 - surface.material.metallic);

		surface.light.rgb += (kD * surface.material.albedo.rgb * light.rgb) * surface.material.occlusion;
	}

	if ( surface.material.albedo.a < 0.0001 && dot(surface.light.rgb, vec3(1.0)) < 0.0001 ) discard;

	surface.fragment = vec4(0.0);
	surface.light.rgb += surface.material.albedo.rgb /* ubo.settings.lighting.ambient.rgb*/ * surface.material.occlusion;

/*	
#if PBR
	pbr();
#elif LAMBERT
	lambert();
#endif
*/	
	outFragColor.rgb = surface.fragment.rgb + surface.light.rgb;
	outFragColor.a = surface.material.albedo.a;
#endif
}