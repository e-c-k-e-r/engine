#version 450
#extension GL_EXT_samplerless_texture_functions : require
#extension GL_EXT_nonuniform_qualifier : enable

layout (local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

#define MULTISAMPLING 1
#define DEFERRED_SAMPLING 1

#define LAMBERT 1
#define PBR 0

const float PI = 3.14159265359;
const float EPSILON = 0.00001;

const float LIGHT_POWER_CUTOFF = 0.005;

layout (constant_id = 0) const uint CASCADES = 16;
layout (constant_id = 1) const uint TEXTURES = 512;

struct Matrices {
	mat4 view[2];
	mat4 projection[2];
	mat4 iView[2];
	mat4 iProjection[2];
	mat4 iProjectionView[2];
	vec4 eyePos[2];
	mat4 vxgi;
};

struct Space {
	vec3 eye;
	vec3 world;
};

struct Ray {
	vec3 origin;
	vec3 direction;

	vec3 position;
	float distance;
};

struct Fog {
	vec3 color;
	float stepScale;

	vec3 offset;
	float densityScale;

	float densityThreshold;
	float densityMultiplier;
	float absorbtion;
	float padding1;

	vec2 range;
	float padding2;
	float padding3;
};

struct Mode {
	uint type;
	uint scalar;
	vec2 padding;
	vec4 parameters;
};

struct Light {
	vec3 position;
	float radius;
	
	vec3 color;
	float power;
	
	int type;
	int mapIndex;
	float depthBias;
	float padding;

	mat4 view;
	mat4 projection;
};

struct Material {
	vec4 colorBase;
	vec4 colorEmissive;

	float factorMetallic;
	float factorRoughness;
	float factorOcclusion;
	float factorAlphaCutoff;

	int indexAlbedo;
	int indexNormal;
	int indexEmissive;
	int indexOcclusion;
	
	int indexMetallicRoughness;
	int indexAtlas;
	int indexLightmap;
	int modeAlpha;
};

struct Texture {
	int index;
	int samp;
	int remap;
	float blend;

	vec4 lerp;
};

struct DrawCall {
	int materialIndex;
	uint materials;
	int textureIndex;
	uint textures;
};

struct SurfaceMaterial {
	uint id;

	vec4 albedo;
	vec4 indirect;

	float metallic;
	float roughness;
	float occlusion;
};

struct Surface {
	vec2 uv;
	Space position;
	Space normal;
	
	Ray ray;
	
	SurfaceMaterial material;

	vec4 fragment;
} surface;

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

layout (binding = 9, rg16ui) uniform volatile coherent uimage3D voxelId[CASCADES];
layout (binding = 10, rg16f) uniform volatile coherent image3D voxelUv[CASCADES];
layout (binding = 11, rg16f) uniform volatile coherent image3D voxelNormal[CASCADES];
layout (binding = 12, rgba8) uniform volatile coherent image3D voxelRadiance[CASCADES];

layout (binding = 13) uniform sampler3D samplerNoise;
layout (binding = 14) uniform samplerCube samplerSkybox;
layout (binding = 15) uniform sampler2D samplerTextures[TEXTURES];

// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2.
float ndfGGX(float cosLh, float roughness) {
	const float alpha   = roughness * roughness;
	const float alphaSq = alpha * alpha;
	const float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
}

// Single term for separable Schlick-GGX below.
float gaSchlickG1(float cosTheta, float k) {
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method.
float gaSchlickGGX(float cosLi, float cosLo, float roughness) {
	const float r = roughness + 1.0;
	const float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights.
	return gaSchlickG1(cosLi, k) * gaSchlickG1(cosLo, k);
}
vec3 fresnelSchlick(vec3 F0, float cosTheta) {
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float random(vec3 seed, int i){
	return fract(sin(dot(vec4(seed,i), vec4(12.9898,78.233,45.164,94.673))) * 43758.5453);
}

// Returns a vector that is orthogonal to u.
vec3 orthogonal(vec3 u){
	u = normalize(u);
	const vec3 v = vec3(0.99146, 0.11664, 0.05832); // Pick any normalized vector.
	return abs(dot(u, v)) > 0.99999f ? cross(u, vec3(0, 1, 0)) : cross(u, v);
}
float rand2(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 143758.5453);
}
float rand3(vec3 co){
    return fract(sin(dot(co.xyz ,vec3(12.9898,78.233, 37.719))) * 143758.5453);
}

vec3 gamma( vec3 i ) {
	return pow(i.rgb, vec3(1.0 / 2.2));
}

vec3 decodeNormals( vec2 enc ) {
	const vec2 ang = enc*2-1;
	const vec2 scth = vec2( sin(ang.x * PI), cos(ang.x * PI) );
	const vec2 scphi = vec2(sqrt(1.0 - ang.y*ang.y), ang.y);
	return normalize( vec3(scth.y*scphi.x, scth.x*scphi.x, scphi.y) );
}
float wrap( float i ) {
	return fract(i);
}
vec2 wrap( vec2 uv ) {
	return vec2( wrap( uv.x ), wrap( uv.y ) );
}
bool validTextureIndex( int textureIndex ) {
	return 0 <= textureIndex; // && textureIndex < ubo.textures;
}

float shadowFactor( const Light light, float def ) {
	if ( !validTextureIndex(light.mapIndex) ) return 1.0;

	vec4 positionClip = light.projection * light.view * vec4(surface.position.world, 1.0);
	positionClip.xyz /= positionClip.w;

	if ( positionClip.x < -1 || positionClip.x >= 1 ) return def; //0.0;
	if ( positionClip.y < -1 || positionClip.y >= 1 ) return def; //0.0;
	if ( positionClip.z <= 0 || positionClip.z >= 1 ) return def; //0.0;

	float factor = 1.0;

	// spot light
	if ( abs(light.type) == 2 || abs(light.type) == 3 ) {
		const float dist = length( positionClip.xy );
		if ( dist > 0.5 ) return def; //0.0;
		
		// spot light with attenuation
		if ( abs(light.type) == 3 ) {
			factor = 1.0 - (pow(dist * 2,2.0));
		}
	}
	
	const vec2 uv = positionClip.xy * 0.5 + 0.5;
	const float bias = light.depthBias;
	const float eyeDepth = positionClip.z;
	const int samples = int(ubo.shadowSamples);
	return eyeDepth < texture(samplerTextures[nonuniformEXT(light.mapIndex)], uv).r - bias ? 0.0 : factor;
#if 0
	for ( int i = 0; i < samples; ++i ) {
		const int index = int( float(samples) * random(floor(surface.position.world.xyz * 1000.0), i)) % samples;
		const float lightDepth = texture(samplerTextures[nonuniformEXT(light.mapIndex)], uv + poissonDisk[index] / 700.0 ).r;
		if ( eyeDepth < lightDepth - bias ) factor -= 1.0 / samples;
	}
	return factor;
#endif
}

float SCALE_CASCADE( uint x ) {
	return pow(1 + x, ubo.cascadePower);
}

void main() {
	const vec3 tUvw = gl_GlobalInvocationID.xzy;
	for ( uint CASCADE = 0; CASCADE < CASCADES; ++CASCADE ) {
		surface.normal.world = decodeNormals( vec2(imageLoad(voxelNormal[CASCADE], ivec3(tUvw) ).xy) );
		surface.normal.eye = vec3( ubo.matrices.vxgi * vec4( surface.normal.world, 0.0f ) );
	
		surface.position.eye = (vec3(gl_GlobalInvocationID.xyz) / vec3(imageSize(voxelRadiance[CASCADE])) * 2.0f - 1.0f) * SCALE_CASCADE(CASCADE);
		surface.position.world = vec3( inverse(ubo.matrices.vxgi) * vec4( surface.position.eye, 1.0f ) );
		
		const uvec2 ID = uvec2(imageLoad(voxelId[CASCADE], ivec3(tUvw) ).xy);
		if ( ID.x == 0 || ID.y == 0 ) {
			imageStore(voxelRadiance[CASCADE], ivec3(tUvw), vec4(0));
			continue;
		}
		const uint drawId = ID.x - 1;
		const DrawCall drawCall = drawCalls[drawId];
		surface.material.id = ID.y + drawCall.materialIndex - 1;
		const Material material = materials[surface.material.id];
		surface.material.albedo = material.colorBase;
		surface.fragment = material.colorEmissive;

	#if DEFERRED_SAMPLING
		surface.uv = imageLoad(voxelUv[CASCADE], ivec3(tUvw) ).xy;
		const bool useAtlas = validTextureIndex( drawCall.textureIndex + material.indexAtlas );
		Texture textureAtlas;
		if ( useAtlas ) textureAtlas = textures[drawCall.textureIndex + material.indexAtlas];
		if ( validTextureIndex( drawCall.textureIndex + material.indexAlbedo ) ) {
			const Texture t = textures[drawCall.textureIndex + material.indexAlbedo];
			surface.material.albedo = texture( samplerTextures[nonuniformEXT((useAtlas)?textureAtlas.index:t.index)], ( useAtlas ) ? mix( t.lerp.xy, t.lerp.zw, surface.uv ) : surface.uv );
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
			surface.fragment += texture( samplerTextures[nonuniformEXT((useAtlas)?textureAtlas.index:t.index)], ( useAtlas ) ? mix( t.lerp.xy, t.lerp.zw, surface.uv ) : surface.uv );
		}
	#else
		surface.material.albedo = imageLoad(voxelRadiance[CASCADE], ivec3(tUvw) );
	#endif
		surface.material.metallic = material.factorMetallic;
		surface.material.roughness = material.factorRoughness;
		surface.material.occlusion = material.factorOcclusion;

		float litFactor = 1.0;
		if ( 0 <= material.indexLightmap ) {
			surface.fragment.rgb += surface.material.albedo.rgb + ubo.ambient.rgb * surface.material.occlusion;
		} else {
			surface.fragment.rgb += surface.material.albedo.rgb * ubo.ambient.rgb * surface.material.occlusion;
		}
		// corrections
		surface.material.roughness *= 4.0;
		{
			const vec3 F0 = mix(vec3(0.04), surface.material.albedo.rgb, surface.material.metallic); 
			const vec3 Lo = normalize( surface.position.world );
			const float cosLo = max(0.0, dot(surface.normal.world, Lo));
			for ( uint i = 0; i < ubo.lights; ++i ) {
				const Light light = lights[i];
				if ( light.power <= LIGHT_POWER_CUTOFF ) continue;
				if ( light.type >= 0 && 0 <= material.indexLightmap ) continue;
				const vec3 Lp = light.position;
				const vec3 Liu = light.position - surface.position.world;
				const vec3 Li = normalize(Liu);
				const float Ls = shadowFactor( light, 0.0 );
				const float La = 1.0 / (PI * pow(length(Liu), 2.0));
				if ( light.power * La * Ls <= LIGHT_POWER_CUTOFF ) continue;

				const float cosLi = max(0.0, dot(surface.normal.world, Li));
				const vec3 Lr = light.color.rgb * light.power * La * Ls;
			#if LAMBERT
				const vec3 diffuse = surface.material.albedo.rgb;
				const vec3 specular = vec3(0);
			#elif PBR
				const vec3 Lh = normalize(Li + Lo);
				const float cosLh = max(0.0, dot(surface.normal.world, Lh));
				
				const vec3 F = fresnelSchlick( F0, max( 0.0, dot(Lh, Lo) ) );
				const float D = ndfGGX( cosLh, surface.material.roughness );
				const float G = gaSchlickGGX(cosLi, cosLo, surface.material.roughness);
				const vec3 diffuse = mix( vec3(1.0) - F, vec3(0.0), surface.material.metallic ) * surface.material.albedo.rgb;
				const vec3 specular = (F * D * G) / max(EPSILON, 4.0 * cosLi * cosLo);
			#endif
				// lightmapped, compute only specular
				if ( light.type >= 0 && 0 <= material.indexLightmap ) surface.fragment.rgb += (specular) * Lr * cosLi;
				// point light, compute only diffuse
				// else if ( abs(light.type) == 1 ) surface.fragment.rgb += (diffuse) * Lr * cosLi;
				else surface.fragment.rgb += (diffuse + specular) * Lr * cosLi;
				surface.fragment.a += light.power * La * Ls;
			}
		}

		imageStore(voxelRadiance[CASCADE], ivec3(tUvw), vec4(surface.fragment.rgb, 1));
	}
}