#extension GL_EXT_samplerless_texture_functions : require
#extension GL_EXT_nonuniform_qualifier : enable

#if RT
	#extension GL_EXT_ray_tracing : enable
	#extension GL_EXT_ray_query : enable
#endif

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#define COMPUTE 1
#define DEFERRED 1
#define DEFERRED_SAMPLING 1

#define PBR 1
#define LAMBERT 0
#define FOG 1
#define FOG_RAY_MARCH 1

#include "../../../common/macros.h"

layout (constant_id = 0) const uint TEXTURES = 512;
layout (constant_id = 1) const uint CUBEMAPS = 128;
#if VXGI
	layout (constant_id = 2) const uint CASCADES = 16;
#endif

#if !MULTISAMPLING
	layout(binding = 0) uniform utexture2D samplerId;
	#if BARYCENTRIC
		#if !BARYCENTRIC_CALCULATE
			layout(binding = 1) uniform texture2D samplerBary;
		#endif
	#else
		layout(binding = 1) uniform texture2D samplerUv;
		layout(binding = 2) uniform texture2D samplerNormal;
	#endif
	layout(binding = 3) uniform texture2D samplerDepth;
#else
	layout(binding = 0) uniform utexture2DMS samplerId;
	#if BARYCENTRIC
		#if !BARYCENTRIC_CALCULATE
			layout(binding = 1) uniform texture2DMS samplerBary;
		#endif
	#else
		layout(binding = 1) uniform texture2DMS samplerUv;
		layout(binding = 2) uniform texture2DMS samplerNormal;
	#endif
	layout(binding = 3) uniform texture2DMS samplerDepth;
#endif


layout(binding = 7, rgba16f) uniform volatile coherent image2D imageColor;
layout(binding = 8, rgba16f) uniform volatile coherent image2D imageBright;
layout(binding = 9, rg16f) uniform volatile coherent image2D imageMotion;

layout( push_constant ) uniform PushBlock {
  uint pass;
  uint draw;
} PushConstant;

#include "../../../common/structs.h"

layout (binding = 10) uniform UBO {
	EyeMatrices eyes[2];

	Settings settings;
} ubo;
/*
*/
layout (std140, binding = 11) readonly buffer DrawCommands {
	DrawCommand drawCommands[];
};
layout (std140, binding = 12) readonly buffer Instances {
	Instance instances[];
};
layout (std140, binding = 13) readonly buffer InstanceAddresseses {
	InstanceAddresses instanceAddresses[];
};
layout (std140, binding = 14) readonly buffer Materials {
	Material materials[];
};
layout (std140, binding = 15) readonly buffer Textures {
	Texture textures[];
};
layout (std140, binding = 16) readonly buffer Lights {
	Light lights[];
};

layout (binding = 17) uniform sampler2D samplerTextures[TEXTURES];
layout (binding = 18) uniform samplerCube samplerCubemaps[CUBEMAPS];
layout (binding = 19) uniform sampler3D samplerNoise;
#if VXGI
	layout (binding = 20) uniform usampler3D voxelDrawId[CASCADES];
	layout (binding = 21) uniform usampler3D voxelInstanceId[CASCADES];
	layout (binding = 22) uniform sampler3D voxelNormalX[CASCADES];
	layout (binding = 23) uniform sampler3D voxelNormalY[CASCADES];
	layout (binding = 24) uniform sampler3D voxelRadianceR[CASCADES];
	layout (binding = 25) uniform sampler3D voxelRadianceG[CASCADES];
	layout (binding = 26) uniform sampler3D voxelRadianceB[CASCADES];
	layout (binding = 27) uniform sampler3D voxelRadianceA[CASCADES];
	layout (binding = 28) uniform sampler3D voxelCount[CASCADES];
	layout (binding = 29) uniform sampler3D voxelOutput[CASCADES];
#endif
#if RT
	layout (binding = 30) uniform accelerationStructureEXT tlas;
#endif

#if BUFFER_REFERENCE
	//layout(buffer_reference, scalar) buffer Vertices { Vertex v[]; };
	layout(buffer_reference, scalar) buffer Indices { uint i[]; };
	//layout(buffer_reference, scalar) buffer Indices { uvec3 i[]; };
	layout(buffer_reference, scalar) buffer Indirects { DrawCommand dc[]; };

	//layout(buffer_reference, scalar) buffer VPos { vec3 v[]; };
	layout(buffer_reference, scalar) buffer VPos { float v[]; };
	layout(buffer_reference, scalar) buffer VUv { vec2 v[]; };
	//layout(buffer_reference, scalar) buffer VUv { float v[]; };
	layout(buffer_reference, scalar) buffer VColor { uint v[]; };
	layout(buffer_reference, scalar) buffer VSt { vec2 v[]; };
	//layout(buffer_reference, scalar) buffer VSt { float v[]; };
	//layout(buffer_reference, scalar) buffer VNormal { vec3 v[]; };
	layout(buffer_reference, scalar) buffer VNormal { float v[]; };
	//layout(buffer_reference, scalar) buffer VTangent { vec3 v[]; };
	layout(buffer_reference, scalar) buffer VTangent { float v[]; };
	layout(buffer_reference, scalar) buffer VID { uint v[]; };
#endif

#include "../../../common/functions.h"
#include "../../../common/fog.h"
#include "../../../common/light.h"
#include "../../../common/shadows.h"
#if VXGI
	#include "../../../common/vxgi.h"
#endif
#if RT
	#include "../../../common/rt.h"
#endif

#if MULTISAMPLING
	#define IMAGE_LOAD(X) texelFetch( X, ivec2(gl_GlobalInvocationID.xy), msaa.currentID )
#else
	#define IMAGE_LOAD(X) texelFetch( X, ivec2(gl_GlobalInvocationID.xy), 0 )
#endif

#define IMAGE_STORE(X, Y) imageStore( X, ivec2(gl_GlobalInvocationID.xy), Y )

bool USE_SKYBOX_ON_DIVERGENCE = false;

void postProcess() {
	if ( USE_SKYBOX_ON_DIVERGENCE ) {
		if ( 0 <= ubo.settings.lighting.indexSkybox && ubo.settings.lighting.indexSkybox < CUBEMAPS ) {
			surface.fragment.rgb = texture( samplerCubemaps[ubo.settings.lighting.indexSkybox], surface.ray.direction ).rgb;
		}
	}
#if FOG
	fog( surface.ray, surface.fragment.rgb, surface.fragment.a );
#endif
	float brightness = dot(surface.fragment.rgb, vec3(0.2126, 0.7152, 0.0722));
	bool bloom = brightness > ubo.settings.bloom.threshold;
//if ( bloom ) toneMap( surface.fragment.rgb, brightness );
	vec4 outFragColor = vec4(surface.fragment.rgb, 1.0);
	vec4 outFragBright = bloom ? vec4(surface.fragment.rgb, 1.0) : vec4(0, 0, 0, 1);
	vec2 outFragMotion = surface.motion;

	if ( ubo.settings.mode.type > 0x0000 ) {
		uvec2 renderSize = imageSize(imageColor);
		vec2 inUv = (vec2(gl_GlobalInvocationID.xy) / vec2(renderSize)) * 2.0f - 1.0f;
		if ( true ) {
		//	if ( ubo.settings.mode.type == 0x0001 ) outFragColor = vec4(surface.barycentric.rgb, 1);
			if ( ubo.settings.mode.type == 0x0001 ) outFragColor = vec4(surface.material.albedo.rgb, 1);
			else if ( ubo.settings.mode.type == 0x0002 ) outFragColor = vec4(surface.light.rgb, 1);
			else if ( ubo.settings.mode.type == 0x0003 ) outFragColor = vec4(vec3(surface.light.a), 1);
			else if ( ubo.settings.mode.type == 0x0004 ) outFragColor = vec4(surface.normal.eye.rgb, 1);
			else if ( ubo.settings.mode.type == 0x0005 ) outFragColor = vec4(surface.uv.xy, 0, 1);
			else if ( ubo.settings.mode.type == 0x0006 ) outFragColor = vec4(surface.st.xy, 0, 1);
			else if ( ubo.settings.mode.type == 0x0007 ) outFragColor = vec4(vec3(surface.material.metallic), 1);
			else if ( ubo.settings.mode.type == 0x0008 ) outFragColor = vec4(vec3(surface.material.roughness * 4), 1);
			else if ( ubo.settings.mode.type == 0x0009 ) outFragColor = vec4(vec3(surface.material.occlusion), 1);
		}
	}

	IMAGE_STORE( imageColor, outFragColor );
	IMAGE_STORE( imageBright, outFragBright );
	IMAGE_STORE( imageMotion, vec4(outFragMotion, 0, 0) );
}

void populateSurface() {
	const uvec2 renderSize = imageSize(imageColor);
	if ( gl_GlobalInvocationID.x >= renderSize.x || gl_GlobalInvocationID.y >= renderSize.y || gl_GlobalInvocationID.z > PushConstant.pass ) return;

	surface.pass = PushConstant.pass;
	surface.fragment = vec4(0);
	surface.light = vec4(0);
	surface.motion = vec2(0);
	surface.material.indirect = vec4(0);
	surface.material.metallic = 1;
	surface.material.roughness = 0;
	surface.material.occlusion = 0;

	float depth = 0.0;
	{
		vec2 inUv = (vec2(gl_GlobalInvocationID.xy) / vec2(renderSize)) * 2.0f - 1.0f;
		const mat4 iProjectionView = inverse( ubo.eyes[surface.pass].projection * mat4(mat3(ubo.eyes[surface.pass].view)) );
		const vec4 near4 = iProjectionView * (vec4(inUv, -1.0, 1.0));
		const vec4 far4 = iProjectionView * (vec4(inUv, 1.0, 1.0));
		const vec3 near3 = near4.xyz / near4.w;
		const vec3 far3 = far4.xyz / far4.w;

		surface.ray.direction = normalize( far3 - near3 );
//	surface.ray.origin = near3.xyz;
		surface.ray.origin = ubo.eyes[surface.pass].eyePos.xyz;
//	surface.ray.origin = vec3( -ubo.eyes[surface.pass].view[0][3], -ubo.eyes[surface.pass].view[1][3], -ubo.eyes[surface.pass].view[2][3] );

		depth = IMAGE_LOAD(samplerDepth).r;
		
		vec4 eye = ubo.eyes[surface.pass].iProjection * vec4(inUv, depth, 1.0);
		eye /= eye.w;

		surface.position.eye = eye.xyz;
		surface.position.world = vec3( ubo.eyes[surface.pass].iView * eye );
	}


#if !MULTISAMPLING
	const uvec2 ID = uvec2(IMAGE_LOAD(samplerId).xy);
#else
	const uvec2 ID = msaa.IDs[msaa.currentID];
#endif

	if ( ID.x == 0 || ID.y == 0 || depth <= 0.0 ) {
		USE_SKYBOX_ON_DIVERGENCE = true;
	}
/*
	if ( ID.x == 0 || ID.y == 0 || depth <= 0.0 ) {
		if ( 0 <= ubo.settings.lighting.indexSkybox && ubo.settings.lighting.indexSkybox < CUBEMAPS ) {
			surface.fragment.rgb = texture( samplerCubemaps[ubo.settings.lighting.indexSkybox], surface.ray.direction ).rgb;
		}
		return;
	}
*/
	{
		const uint triangleID = ID.x == 0 ? 0 : ID.x - 1;
		const uint instanceID = ID.y == 0 ? 0 : ID.y - 1;
		surface.subID = 1;

	#if BARYCENTRIC
		#if !BARYCENTRIC_CALCULATE
			surface.barycentric = decodeBarycentrics(IMAGE_LOAD(samplerBary).xy).xyz;
		#endif
		populateSurface( instanceID, triangleID );
	#else
		vec4 uvst = IMAGE_LOAD(samplerUv);
		vec4 normaltangent = IMAGE_LOAD(samplerNormal);

		surface.uv.xy = uvst.xy;
		surface.uv.z = 0;
		surface.st.xy = uvst.zw;
		surface.st.z = 0;

		surface.normal.world = decodeNormals(normaltangent.xy);
	//	surface.tangent.world = decodeNormals(normaltangent.zw);

		surface.instance = instances[instanceID >= instances.length() ? 0 : instanceID];

		populateSurfaceMaterial();
	#endif
	}

	{
		vec4 pNDC = ubo.eyes[surface.pass].previous * surface.instance.previous * vec4(surface.position.world, 1);
		vec4 cNDC = ubo.eyes[surface.pass].model * surface.instance.model * vec4(surface.position.world, 1);
		pNDC /= pNDC.w;
		cNDC /= cNDC.w;

		surface.motion = cNDC.xy - pNDC.xy;
	}

}

void directLighting() {
#if VXGI
	// to-do: proper "visual" of the VXGI maps (directly pick the pixel instead of just rawdog tracing for it)
	if ( ubo.settings.mode.type == 0x000A ) {
		Ray ray;
		ray.direction = surface.ray.direction;
		ray.origin = surface.ray.origin;
		ray.origin -= ray.direction;

		vec4 radiance = voxelConeTrace( ray, 0 );

		surface.material.albedo.rgb = radiance.rgb;
		surface.material.indirect.rgb = vec3(0);
		surface.fragment.rgb = radiance.rgb;
		surface.fragment.a = radiance.a;
		//return;
	}
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
}

void indirectLighting() {
	uint scale = 0;
#if RT
	++scale;
	indirectLightingRT();
#endif
#if VXGI
	++scale;
	indirectLightingVXGI();
#endif

//	if ( scale > 1 ) surface.material.indirect.rgb /= scale;
}

#if MULTISAMPLING
void resolveSurfaceFragment() {
	for ( int i = 0; i < ubo.settings.mode.msaa; ++i ) {
		msaa.currentID = i;
		msaa.IDs[i] = uvec3(IMAGE_LOAD(samplerId)).xy;

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
		#if VXGI || RT
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