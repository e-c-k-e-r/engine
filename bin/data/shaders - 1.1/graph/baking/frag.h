//#extension GL_EXT_nonuniform_qualifier : enable
#if RT
	#extension GL_EXT_ray_tracing : enable
	#extension GL_EXT_ray_query : enable
#endif

layout (constant_id = 0) const uint TEXTURES = 512;
layout (constant_id = 1) const uint CUBEMAPS = 128;
layout (constant_id = 2) const uint LAYERS = 32;

layout (binding = 5) uniform sampler2D samplerTextures[TEXTURES];
layout (binding = 6) uniform samplerCube samplerCubemaps[CUBEMAPS];

#define SHADOW_SAMPLES 16
#define FRAGMENT 1
#define BAKING 1
#define PBR 1
#define MAX_LIGHTS min(ubo.lights, lights.length())
#define MAX_SHADOWS MAX_LIGHTS
#define VIEW_MATRIX camera.viewport[0].view

#include "../../common/macros.h"
#include "../../common/structs.h"

layout (binding = 7) uniform Camera {
	Viewport viewport[6];
} camera;

layout (binding = 8) uniform UBO {
	uint lights;
	uint currentID;
	uint padding1;
	uint padding2;
} ubo;

layout (std140, binding = 9) readonly buffer DrawCommands {
	DrawCommand drawCommands[];
};
layout (std140, binding = 10) readonly buffer Instances {
	Instance instances[];
};
layout (std140, binding = 11) readonly buffer InstanceAddresseses {
	InstanceAddresses instanceAddresses[];
};
layout (std140, binding = 12) readonly buffer Materials {
	Material materials[];
};
layout (std140, binding = 13) readonly buffer Textures {
	Texture textures[];
};
layout (std140, binding = 14) readonly buffer Lights {
	Light lights[];
};

layout (binding = 15, rgba8) uniform volatile coherent image3D outAlbedos;

#if RT
	layout (binding = 16) uniform accelerationStructureEXT tlas;
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
layout (location = 2) in vec4 inPOS1;
layout (location = 3) in vec3 inPosition;
layout (location = 4) in vec2 inUv;
layout (location = 5) in vec4 inColor;
layout (location = 6) in vec2 inSt;
layout (location = 7) in vec3 inNormal;
layout (location = 8) in vec3 inTangent;

layout (location = 0) out vec4 outAlbedo;

void main() {
	const uint triangleID = uint(inId.x); // gl_PrimitiveID
	const uint drawID = uint(inId.y);
	const uint instanceID = uint(inId.z);

	if ( instanceID != ubo.currentID ) discard;

	surface.uv.xy = wrap(inUv.xy);
	surface.uv.z = mipLevel(dFdx(inUv), dFdy(inUv));
	
	surface.position.world = inPosition;
	surface.normal.world = inNormal;

	const DrawCommand drawCommand = drawCommands[drawID];
	const Instance instance = instances[instanceID];
	const Material material = materials[instance.materialID];

	const uint mapID = instance.auxID;

	vec4 A = material.colorBase;
	surface.material.metallic = material.factorMetallic;
	surface.material.roughness = material.factorRoughness;
	surface.material.occlusion = 1.0f - material.factorOcclusion;

#if 0
	vec3 N = inNormal;
	vec3 T = inTangent;
	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(T, N);
	mat3 TBN = mat3(T, B, N);
//	mat3 TBN = mat3(N, B, T);
	if ( T != vec3(0) && validTextureIndex( material.indexNormal ) ) {
		surface.normal.world = TBN * normalize( sampleTexture( material.indexNormal ).xyz * 2.0 - 1.0 );
	} else {
		surface.normal.world = N;
	}
#endif

	surface.light = material.colorEmissive;
	surface.material.albedo = vec4(1);
	{
		surface.normal.eye = surface.normal.eye;
		surface.position.eye = surface.position.eye;
	//	pbr();
	
		const vec3 F0 = mix(vec3(0.04), surface.material.albedo.rgb, surface.material.metallic); 
		for ( uint i = 0; i < min(ubo.lights, lights.length()); ++i ) {
			const Light light = lights[i];

			if ( light.type <= 0 ) continue;

			const mat4 mat = light.view; // inverse(light.view);
			const vec3 position = surface.position.world;
		//	const vec3 position = vec3( mat * vec4(surface.position.world, 1.0) );
			const vec3 normal = surface.normal.world;
		//	const vec3 normal = vec3( mat * vec4(surface.normal.world, 0.0) );

			if ( light.power <= LIGHT_POWER_CUTOFF ) continue;
			const vec3 Lp = light.position;
			const vec3 Liu = light.position - surface.position.world;
			const float La = 1.0 / (1 + PI * pow(length(Liu), 2.0));
			const float Ls = shadowFactor( light, 0.0 );
			if ( light.power * La * Ls <= LIGHT_POWER_CUTOFF ) continue;

			const vec3 Lo = normalize( -position );
			const float cosLo = max(0.0, abs(dot(normal, Lo)));

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
#define EXPOSURE 0
#define GAMMA 0

//	surface.light.rgb = vec3(1.0) - exp(-surface.light.rgb * EXPOSURE);
//	surface.light.rgb = pow(surface.light.rgb, vec3(1.0 / GAMMA));

	outAlbedo = vec4(surface.light.rgb, 1);

	{
		const vec2 st = inSt.xy * imageSize(outAlbedos).xy;
		const ivec3 uvw = ivec3(int(st.x), int(st.y), int(mapID));
		imageStore(outAlbedos, uvw, vec4(surface.light.rgb, 1) );
	}
}