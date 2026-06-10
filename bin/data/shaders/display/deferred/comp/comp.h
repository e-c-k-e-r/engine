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
	layout(binding = 0, set = 0) uniform utexture2DArray samplerId;
	#if BARYCENTRIC
		#if !BARYCENTRIC_CALCULATE
			layout(binding = 1, set = 0) uniform texture2DArray samplerBary;
		#endif
	#else
		layout(binding = 1, set = 0) uniform texture2DArray samplerUv;
		layout(binding = 2, set = 0) uniform texture2DArray samplerNormal;
	#endif
	layout(binding = 3, set = 0) uniform texture2DArray samplerDepth;
#else
	layout(binding = 0, set = 0) uniform utexture2DMSArray samplerId;
	#if BARYCENTRIC
		#if !BARYCENTRIC_CALCULATE
			layout(binding = 1, set = 0) uniform texture2DMSArray samplerBary;
		#endif
	#else
		layout(binding = 1, set = 0) uniform texture2DMSArray samplerUv;
		layout(binding = 2, set = 0) uniform texture2DMSArray samplerNormal;
	#endif
	layout(binding = 3, set = 0) uniform texture2DMSArray samplerDepth;
#endif


layout(binding = 7, set = 0, rgba16f) uniform writeonly image2DArray imageColor;
layout(binding = 8, set = 0, rgba16f) uniform writeonly image2DArray imageBright;
layout(binding = 9, set = 0, rg16f) uniform writeonly image2DArray imageMotion;

layout( push_constant ) uniform PushBlock {
  uint pass;
  uint draw;
} PushConstant;

#include "../../../common/structs.h"

layout (binding = 10, set = 0) uniform Camera {
	Viewport viewport[2];
} camera;

layout (binding = 11, set = 0) uniform UBO {
	EyeMatrices eyes[2];

	Settings settings;
} ubo;

layout (std140, binding = 12, set = 0) readonly buffer DrawCommands {
	DrawCommand drawCommands[];
};
layout (std140, binding = 13, set = 0) readonly buffer Instances {
	Instance instances[];
};
layout (std140, binding = 14, set = 0) readonly buffer InstanceAddresseses {
	InstanceAddresses addresses[];
};
layout (std140, binding = 15, set = 0) readonly buffer Objects {
	Object objects[];
};
layout (std140, binding = 16, set = 0) readonly buffer Materials {
	Material materials[];
};
layout (std140, binding = 17, set = 0) readonly buffer Textures {
	Texture textures[];
};
layout (std140, binding = 18, set = 0) readonly buffer Lights {
	Light lights[];
};

layout (binding = 19, set = 1) uniform sampler2D samplerTextures[TEXTURES];
layout (binding = 20, set = 1) uniform samplerCube samplerCubemaps[CUBEMAPS];
layout (binding = 21, set = 0) uniform sampler3D samplerNoise;
#if VXGI
	layout (binding = 22, set = 0) uniform sampler3D voxelOutput[CASCADES];
#endif
#if RT
	layout (binding = 23, set = 0) uniform accelerationStructureEXT tlas;
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
	#define IMAGE_LOAD(X) texelFetch( X, ivec3(gl_GlobalInvocationID.xyz), msaa.currentID )
#else
	#define IMAGE_LOAD(X) texelFetch( X, ivec3(gl_GlobalInvocationID.xyz), 0 )
#endif

#define IMAGE_STORE(X, Y) imageStore( X, ivec3(gl_GlobalInvocationID.xyz), Y )

bool USE_SKYBOX_ON_DIVERGENCE = false;

void postProcess() {
#if !MULTISAMPLING
	if ( USE_SKYBOX_ON_DIVERGENCE ) {
		if ( 0 <= ubo.settings.lighting.indexSkybox && ubo.settings.lighting.indexSkybox < CUBEMAPS ) {
			surface.fragment.rgb = texture( samplerCubemaps[ubo.settings.lighting.indexSkybox], surface.ray.direction ).rgb;
		}
	}
#endif
#if FOG
	fog( surface.ray, surface.fragment.rgb, surface.fragment.a );
#endif
	float brightness = luma(surface.fragment.rgb);
	bool bloom = brightness > ubo.settings.bloom.threshold;
//if ( bloom ) toneMap( surface.fragment.rgb, brightness );
	vec4 outFragColor = vec4(surface.fragment.rgb, 1.0);
	vec4 outFragBright = bloom ? vec4(surface.fragment.rgb, 1.0) : vec4(0, 0, 0, 1);
	vec2 outFragMotion = surface.motion;

	if ( ubo.settings.mode.type > 0x0000 ) {
		uvec2 renderSize = imageSize(imageColor).xy;
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
	//IMAGE_STORE( imageBright, outFragBright );
	IMAGE_STORE( imageMotion, vec4(outFragMotion, 0, 0) );
}

void populateSurface() {
	const uvec2 renderSize = imageSize(imageColor).xy;
	if ( gl_GlobalInvocationID.x >= renderSize.x || gl_GlobalInvocationID.y >= renderSize.y /*|| gl_GlobalInvocationID.z > PushConstant.pass*/ ) return;

	surface.pass = gl_GlobalInvocationID.z;
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

	#if USE_CAMERA_VIEWPORT
		const mat4 iProjection = inverse( camera.viewport[surface.pass].projection );
		const mat4 iView = inverse( camera.viewport[surface.pass].view );
		const mat4 iProjectionView = /*iProjection * iView;*/ inverse( camera.viewport[surface.pass].projection * mat4(mat3(camera.viewport[surface.pass].view)) );
	#else
		const mat4 iView = ubo.eyes[surface.pass].iView;
		const mat4 iProjection = ubo.eyes[surface.pass].iProjection;
		const mat4 iProjectionView = /*iProjection * iView;*/ inverse( ubo.eyes[surface.pass].projection * mat4(mat3(ubo.eyes[surface.pass].view)) );
	#endif
		const vec4 near4 = iProjectionView * (vec4(inUv, -1.0, 1.0));
		const vec4 far4 = iProjectionView * (vec4(inUv, 1.0, 1.0));
		const vec3 near3 = near4.xyz / near4.w;
		const vec3 far3 = far4.xyz / far4.w;

		surface.ray.direction = normalize( far3 - near3 );
		surface.ray.origin = ubo.eyes[surface.pass].eyePos.xyz; // near3.xyz; // eyePos.xyz

		depth = IMAGE_LOAD(samplerDepth).r;

		vec4 eye = iProjection * vec4(inUv, depth, 1.0);
		eye /= eye.w;

		surface.position.eye = eye.xyz;
		surface.position.world = vec3( iView * eye );
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
		surface.object = objects[surface.instance.objectID];

		populateSurfaceMaterial();
	#endif
	}

	{
		vec4 pNDC = ubo.eyes[surface.pass].previous * surface.object.previous * vec4(surface.position.world, 1);
		vec4 cNDC = ubo.eyes[surface.pass].model * surface.object.model * vec4(surface.position.world, 1);
		pNDC /= pNDC.w;
		cNDC /= cNDC.w;

		surface.motion = (pNDC.xy - cNDC.xy) * 0.5;
	}

}

void directLighting() {
#if VXGI
	// to-do: proper "visual" of the VXGI maps (directly pick the pixel instead of just rawdog tracing for it)
	if ( ubo.settings.mode.type == 0x000A ) {
		Ray ray;
		ray.direction = surface.ray.direction;
		ray.origin = surface.position.world;
		ray.origin -= ray.direction;

		vec4 radiance = voxelConeTrace( ray, 0 );

		surface.material.albedo.rgb = radiance.rgb;
		surface.material.indirect.rgb = vec3(0);
		surface.fragment.rgb = radiance.rgb;
		surface.fragment.a = 1; // radiance.a;
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
	msaa.fragment = vec4(0.0);

	for ( int i = 0; i < ubo.settings.mode.msaa; ++i ) {
		msaa.currentID = i;
		msaa.IDs[i] = uvec3(IMAGE_LOAD(samplerId)).xy;

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

			if ( msaa.IDs[i].x == 0 || msaa.IDs[i].y == 0 ) {
				if ( 0 <= ubo.settings.lighting.indexSkybox && ubo.settings.lighting.indexSkybox < CUBEMAPS ) {
					surface.fragment.rgb = texture( samplerCubemaps[ubo.settings.lighting.indexSkybox], surface.ray.direction ).rgb;
					surface.fragment.a = 1.0;
				}
			} else {
			#if VXGI || RT
				indirectLighting();
			#endif
				directLighting();
			}
		}

		msaa.fragment += surface.fragment;
		msaa.fragments[msaa.currentID] = surface.fragment;
	}

	surface.fragment = msaa.fragment / float(ubo.settings.mode.msaa);
}
#endif