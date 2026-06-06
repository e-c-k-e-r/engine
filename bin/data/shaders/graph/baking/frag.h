//#extension GL_EXT_nonuniform_qualifier : enable
#if RT
	#extension GL_EXT_ray_tracing : enable
	#extension GL_EXT_ray_query : enable
#endif

layout (constant_id = 0) const uint TEXTURES = 512;
layout (constant_id = 1) const uint CUBEMAPS = 128;
layout (constant_id = 2) const uint LAYERS = 32;

layout (binding = 6) uniform sampler2D samplerTextures[TEXTURES];
layout (binding = 7) uniform samplerCube samplerCubemaps[CUBEMAPS];

#define SHADOW_SAMPLES 16
#define FRAGMENT 1
#define BAKING 1

#define PBR 0
#define LAMBERT 1

#define LIGHTING_IN_WORLD_SPACE 1

#define MAX_LIGHTS min(ubo.lights, lights.length())
#define MAX_SHADOWS MAX_LIGHTS
#define VIEW_MATRIX camera.viewport[0].view

#include "../../common/macros.h"
#include "../../common/structs.h"

/*layout (binding = 8) uniform*/ struct Camera {
	Viewport viewport[6];
} camera;

layout (binding = 9) uniform UBO {
	uint lights;
	uint currentID;
	float exposure;
	float gamma;
} ubo;

layout (std140, binding = 10) readonly buffer DrawCommands {
	DrawCommand drawCommands[];
};
layout (std140, binding = 11) readonly buffer Instances {
	Instance instances[];
};
layout (std140, binding = 12) readonly buffer InstanceAddresseses {
	InstanceAddresses addresses[];
};
layout (std140, binding = 13) readonly buffer Materials {
	Material materials[];
};
layout (std140, binding = 14) readonly buffer Textures {
	Texture textures[];
};
layout (std140, binding = 15) readonly buffer Lights {
	Light lights[];
};

layout (binding = 16, rgba8) uniform writeonly image3D outAlbedos;

#if RT
	layout (binding = 17) uniform accelerationStructureEXT tlas;
#endif

#include "../../common/functions.h"
#if RT
float shadowFactor( const Light light, float def ) {
	Ray ray;
	ray.origin = surface.position.world;
	ray.direction = light.position - ray.origin;

	float tMin = 0.001;
	float tMax = length(ray.direction) + tMin;
	
	ray.direction = normalize(ray.direction);

	uint rayFlags = gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT;
	uint cullMask = 0xFF;

	rayQueryEXT rayQuery;
	rayQueryInitializeEXT(rayQuery, tlas, rayFlags, cullMask, ray.origin, tMin, ray.direction, tMax);

	while(rayQueryProceedEXT(rayQuery)) {}

	return rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionNoneEXT ? 1.0 : 0.0;
}
#else
	#include "../../common/shadows.h"
#endif

#include "../../common/light.h"

layout (location = 0) flat in uvec4 inId;
layout (location = 1) flat in vec4 inPOS0;
#if BARYCENTRIC_STABILIZE
	layout (location = 2) __explicitInterpAMD in vec4 inPOS1;
#else
	layout (location = 2) in vec4 inPOS1;
#endif
layout (location = 3) in vec3 inPosition;
layout (location = 4) in vec2 inUv;
layout (location = 5) in vec4 inColor;
layout (location = 6) in vec2 inSt;
layout (location = 7) in vec3 inNormal;
layout (location = 8) in vec3 inTangent;
layout (location = 9) in vec3 inBitangent;

void main() {
	const uint triangleID = gl_PrimitiveID; // uint(inId.x);
	const uint drawID = uint(inId.y);
	const uint instanceID = uint(inId.z);

//	if ( instanceID != ubo.currentID ) discard;

	surface.fragment = vec4(0);
	surface.light = vec4(0);

	surface.uv.xy = wrap(inUv.xy);
	surface.uv.z = 0; // mipLevel(dFdx(inUv), dFdy(inUv));
	
	surface.position.world = inPosition;
	surface.normal.world = inNormal;
	surface.tangent.world = inTangent;
	surface.tbn = mat3(inTangent, inBitangent, inNormal);

	const DrawCommand drawCommand = drawCommands[drawID];
	surface.instance = instances[instanceID >= instances.length() ? 0 : instanceID];
	const Material material = materials[surface.instance.materialID >= materials.length() ? 0 : surface.instance.materialID];

	const vec2 st = inSt.xy * imageSize(outAlbedos).xy;
	const ivec3 uvw = ivec3(int(st.x), int(st.y), int(surface.instance.auxID));

	surface.material.albedo = vec4(1); // set to 1 for additive lightmapping // material.colorBase;
	surface.material.metallic = material.factorMetallic;
	surface.material.roughness = material.factorRoughness;
	surface.material.occlusion = material.factorOcclusion;
	surface.light = material.colorEmissive;

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

#if PBR
	pbr();
#elif LAMBERT
	lambert();
#endif
	surface.fragment.rgb = surface.light.rgb;

	if ( length( surface.fragment.rgb ) < 0.001 ) discard;
		
	imageStore(outAlbedos, uvw, encodeRGBE(surface.fragment.rgb) );
}