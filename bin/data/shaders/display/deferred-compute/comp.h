#extension GL_EXT_samplerless_texture_functions : require
#extension GL_EXT_nonuniform_qualifier : enable

#if RAYTRACE
	#extension GL_EXT_ray_query : require
#endif

layout (local_size_x = 8, local_size_y = 8, local_size_z = 2) in;

#define COMPUTE 1
#define TONE_MAP 0
#define GAMMA_CORRECT 0
#define DEFERRED 1
#define MAX_TEXTURES TEXTURES
#define WHITENOISE 0 
#define PBR 1
#define BUFFER_REFERENCE 1
//#define TEXTURE_WORKAROUND 0

#include "../../common/macros.h"

layout (constant_id = 0) const uint TEXTURES = 512;
layout (constant_id = 1) const uint CUBEMAPS = 128;
#if VXGI
	layout (constant_id = 2) const uint CASCADES = 16;
#endif

#if !MULTISAMPLING
	layout(binding = 0) uniform utexture2D samplerId;
	layout(binding = 1) uniform texture2D samplerBary;
	layout(binding = 5) uniform texture2D samplerDepth;
#else
	layout(binding = 0) uniform utexture2DMS samplerId;
	layout(binding = 1) uniform texture2DMS samplerBary;
	layout(binding = 5) uniform texture2DMS samplerDepth;
#endif

layout(binding = 6, rgba16f) uniform volatile coherent image2D imageColor;
layout(binding = 7, rgba16f) uniform volatile coherent image2D imageBright;
layout(binding = 8, rg16f) uniform volatile coherent image2D imageMotion;

layout( push_constant ) uniform PushBlock {
  uint pass;
  uint draw;
} PushConstant;

#include "../../common/structs.h"

layout (binding = 9) uniform UBO {
	EyeMatrices eyes[2];

	Settings settings;
} ubo;
/*
*/
layout (std140, binding = 10) readonly buffer DrawCommands {
	DrawCommand drawCommands[];
};
layout (std140, binding = 11) readonly buffer Instances {
	Instance instances[];
};
layout (std140, binding = 12) readonly buffer InstanceAddresseses {
	InstanceAddresses instanceAddresses[];
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

layout (binding = 16) uniform sampler2D samplerTextures[TEXTURES];
layout (binding = 17) uniform samplerCube samplerCubemaps[CUBEMAPS];
layout (binding = 18) uniform sampler3D samplerNoise;
#if VXGI
	layout (binding = 19) uniform usampler3D voxelId[CASCADES];
	layout (binding = 20) uniform sampler3D voxelNormal[CASCADES];
	layout (binding = 21) uniform sampler3D voxelRadiance[CASCADES];
#endif
#if RAYTRACE
	layout (binding = 22) uniform accelerationStructureEXT tlas;
#endif

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

#include "../../common/functions.h"
#include "../../common/fog.h"
#include "../../common/light.h"
#include "../../common/shadows.h"
#if VXGI
	#include "../../common/vxgi.h"
#endif

#define IMAGE_LOAD(X) texelFetch( X, ivec2(gl_GlobalInvocationID.xy), 0 ) //texelFetch( X, ivec2(gl_GlobalInvocationID.xy), 0 )
#define IMAGE_LOAD_MS(X, I) texelFetch( X, ivec2(gl_GlobalInvocationID.xy), I )

#define IMAGE_STORE(X, Y) imageStore( X, ivec2(gl_GlobalInvocationID.xy), Y )

void postProcess() {
	float brightness = dot(surface.fragment.rgb, vec3(0.2126, 0.7152, 0.0722));
	vec4 outFragBright = brightness > ubo.settings.bloom.brightnessThreshold ? vec4(surface.fragment.rgb, 1.0) : vec4(0, 0, 0, 1);
	vec2 outFragMotion = surface.motion;

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
	vec4 outFragColor = vec4(surface.fragment.rgb, 1.0);
	
	IMAGE_STORE( imageColor, outFragColor );
//	IMAGE_STORE( imageBright, outFragBright );
	IMAGE_STORE( imageMotion, vec4(outFragMotion, 0, 0) );
}

void populateSurface() {
	vec2 inUv = (vec2(gl_GlobalInvocationID.xy) / imageSize(imageColor)); // * 2.0f - 1.0f;
	if ( inUv.x >= 1.0 || inUv.y >= 1.0 ) return;

	surface.fragment = vec4(0);
	surface.pass = PushConstant.pass;

	{
	#if !MULTISAMPLING
		const float depth = IMAGE_LOAD(samplerDepth).r;
	#else
		const float depth = IMAGE_LOAD_MS(samplerDepth, msaa.currentID).r; // resolve(samplerDepth, ubo.settings.mode.msaa).r;
	#endif

		vec2 eUv = inUv * 2.0 - 1.0;

		vec4 positionEye = ubo.eyes[surface.pass].iProjection * vec4(eUv, depth, 1.0);
		positionEye /= positionEye.w;
		surface.position.eye = positionEye.xyz;
		surface.position.world = vec3( ubo.eyes[surface.pass].iView * positionEye );
	}
	{
		const mat4 iProjectionView = inverse( ubo.eyes[surface.pass].projection * mat4(mat3(ubo.eyes[surface.pass].view)) );
		const vec4 near4 = iProjectionView * (vec4(2.0 * inUv - 1.0, -1.0, 1.0));
		const vec4 far4 = iProjectionView * (vec4(2.0 * inUv - 1.0, 1.0, 1.0));
		const vec3 near3 = near4.xyz / near4.w;
		const vec3 far3 = far4.xyz / far4.w;

		surface.ray.direction = normalize( far3 - near3 );
		surface.ray.origin = ubo.eyes[surface.pass].eyePos.xyz;
	}

#if !MULTISAMPLING
	const uvec2 ID = uvec2(IMAGE_LOAD(samplerId).xy);
#else
	const uvec2 ID = msaa.IDs[msaa.currentID]; // IMAGE_LOAD_MS(samplerId, msaa.currentID).xy; //resolve(samplerId, ubo.settings.mode.msaa).xy;
#endif
	surface.motion = vec2(0);

	if ( ID.x == 0 || ID.y == 0 ) {
		if ( 0 <= ubo.settings.lighting.indexSkybox && ubo.settings.lighting.indexSkybox < CUBEMAPS ) {
			surface.fragment.rgb = texture( samplerCubemaps[ubo.settings.lighting.indexSkybox], surface.ray.direction ).rgb;
		}
		surface.fragment.a = 0.0;
	//postProcess();
		return;
	}

#if !MULTISAMPLING
//	surface.barycentrics = IMAGE_LOAD(samplerBary).xyz;
	{
		vec2 encodedBarycentrics = IMAGE_LOAD(samplerBary).xy;
		surface.barycentrics = vec3(
			1.0 - encodedBarycentrics.x - encodedBarycentrics.y,
			encodedBarycentrics.x,
			encodedBarycentrics.y
		);
	}
#else
//	surface.barycentrics = IMAGE_LOAD_MS(samplerBary, msaa.currentID).xyz; // resolve(samplerBary, ubo.settings.mode.msaa).xy;
	{
		vec2 encodedBarycentrics = IMAGE_LOAD_MS(samplerBary, msaa.currentID).xy; // resolve(samplerBary, ubo.settings.mode.msaa).xy;
		surface.barycentrics = vec3(
			1.0 - encodedBarycentrics.x - encodedBarycentrics.y,
			encodedBarycentrics.x,
			encodedBarycentrics.y
		);
	}
#endif

	const uint drawID = ID.x - 1;
	const DrawCommand drawCommand = drawCommands[drawID];
	const uint instanceID = drawID;
	const Instance instance = instances[instanceID];
	surface.instance = instance;

	{
		const uint triangleID = ID.y - 1;
		const InstanceAddresses instanceAddresses = instanceAddresses[instanceID];

		Triangle triangle;

		Vertex points[3];
		uvec3 indices = uvec3( triangleID * 3 + 0, triangleID * 3 + 1, triangleID * 3 + 2 );

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

		triangle.point.position = /*triangle.*/points[0].position * surface.barycentrics[0] + /*triangle.*/points[1].position * surface.barycentrics[1] + /*triangle.*/points[2].position * surface.barycentrics[2];
		triangle.point.uv = /*triangle.*/points[0].uv * surface.barycentrics[0] + /*triangle.*/points[1].uv * surface.barycentrics[1] + /*triangle.*/points[2].uv * surface.barycentrics[2];
		triangle.point.st = /*triangle.*/points[0].st * surface.barycentrics[0] + /*triangle.*/points[1].st * surface.barycentrics[1] + /*triangle.*/points[2].st * surface.barycentrics[2];
		triangle.point.normal = /*triangle.*/points[0].normal * surface.barycentrics[0] + /*triangle.*/points[1].normal * surface.barycentrics[1] + /*triangle.*/points[2].normal * surface.barycentrics[2];
		triangle.point.tangent = /*triangle.*/points[0].tangent * surface.barycentrics[0] + /*triangle.*/points[1].tangent * surface.barycentrics[1] + /*triangle.*/points[2].tangent * surface.barycentrics[2];

		// bind position
		if ( false ) {
			surface.position.world = vec3( instance.model * vec4(triangle.point.position, 1.0 ) );
			surface.position.eye = vec3( ubo.eyes[surface.pass].view * vec4(surface.position.world, 1.0) );
		}
		// bind normals
		{
			surface.normal.world = normalize(vec3( instance.model * vec4(triangle.point.normal, 0.0 ) ));
			surface.normal.eye = vec3( ubo.eyes[surface.pass].view * vec4(surface.normal.world, 0.0) );
		}
		// bind UVs
		{
			surface.uv.xy = triangle.point.uv;
			surface.uv.z = 0;
			surface.st.xy = triangle.point.st;
			surface.st.z = 0;
		}
	}

	const Material material = materials[surface.instance.materialID];
	surface.material.albedo = material.colorBase;
	surface.material.metallic = material.factorMetallic;
	surface.material.roughness = material.factorRoughness;
	surface.material.occlusion = material.factorOcclusion;
	surface.fragment = material.colorEmissive;

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

	{
		vec4 pNDC = ubo.eyes[surface.pass].previous * instance.previous * vec4(surface.position.world, 1);
		vec4 cNDC = ubo.eyes[surface.pass].model * instance.model * vec4(surface.position.world, 1);
		pNDC /= pNDC.w;
		cNDC /= cNDC.w;

		surface.motion = cNDC.xy - pNDC.xy;
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
		msaa.IDs[i] = uvec3(IMAGE_LOAD_MS(samplerId, msaa.currentID)).xy;

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