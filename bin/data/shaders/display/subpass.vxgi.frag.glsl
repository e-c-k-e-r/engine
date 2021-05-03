#version 450
#extension GL_EXT_samplerless_texture_functions : require

#define MULTISAMPLING 1
#define DEFERRED_SAMPLING 1

#define VXGI_NDC 0
#define VXGI_SHADOWS 1

#define FOG 1
#define FOG_RAY_MARCH 1

#define WHITENOISE 1
#define GAMMA_CORRECT 1
#define TONE_MAP 1

#define LAMBERT 0
#define PBR 1

const float PI = 3.14159265359;
const float EPSILON = 0.00001;
const float SQRT2 = 1.41421356237;

const float LIGHT_POWER_CUTOFF = 0.005;

const vec2 poissonDisk[16] = vec2[]( 
   vec2( -0.94201624, -0.39906216 ), 
   vec2( 0.94558609, -0.76890725 ), 
   vec2( -0.094184101, -0.92938870 ), 
   vec2( 0.34495938, 0.29387760 ), 
   vec2( -0.91588581, 0.45771432 ), 
   vec2( -0.81544232, -0.87912464 ), 
   vec2( -0.38277543, 0.27676845 ), 
   vec2( 0.97484398, 0.75648379 ), 
   vec2( 0.44323325, -0.97511554 ), 
   vec2( 0.53742981, -0.47373420 ), 
   vec2( -0.26496911, -0.41893023 ), 
   vec2( 0.79197514, 0.19090188 ), 
   vec2( -0.24188840, 0.99706507 ), 
   vec2( -0.81409955, 0.91437590 ), 
   vec2( 0.19984126, 0.78641367 ), 
   vec2( 0.14383161, -0.14100790 ) 
);

layout (constant_id = 0) const uint TEXTURES = 256;
layout (constant_id = 1) const uint CASCADES = 1;

struct Matrices {
	mat4 view[2];
	mat4 projection[2];
	mat4 iView[2];
	mat4 iProjection[2];
	mat4 iProjectionView[2];
	vec4 eyePos[2];
	mat4 vxgi;
};

struct Ray {
	vec3 origin;
	vec3 direction;

	vec3 position;
	float distance;
};

struct Space {
	vec3 eye;
	vec3 world;
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

struct Voxel {
	uvec2 id;
	vec3 position;
	vec3 normal;
	vec2 uv;
	vec4 color;
};

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
	uint padding1;
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

layout (binding = 9) uniform usampler3D voxelId[CASCADES];
layout (binding = 10) uniform sampler3D voxelUv[CASCADES];
layout (binding = 11) uniform sampler3D voxelNormal[CASCADES];
layout (binding = 12) uniform sampler3D voxelRadiance[CASCADES];

layout (binding = 13) uniform sampler3D samplerNoise;
layout (binding = 14) uniform samplerCube samplerSkybox;
layout (binding = 15) uniform sampler2D samplerTextures[TEXTURES];

layout (location = 0) in vec2 inUv;
layout (location = 1) in flat uint inPushConstantPass;

layout (location = 0) out vec4 outFragColor;

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
void whitenoise(inout vec3 color) {
	const float flicker = ubo.mode.parameters.x;
	const float pieces = ubo.mode.parameters.y;
	const float blend = ubo.mode.parameters.z;
	const float time = ubo.mode.parameters.w;
	if ( blend < 0.0001 ) return;
	const float freq = sin(pow(mod(time, flicker) + flicker, 1.9));
	const float whiteNoise = rand2( floor(gl_FragCoord.xy / pieces) + mod(time, freq) );
	color = mix( color, vec3(whiteNoise), blend );
}
vec3 gamma( vec3 i ) {
	return pow(i.rgb, vec3(1.0 / 2.2));
}

vec2 rayBoxDst( vec3 boundsMin, vec3 boundsMax, in Ray ray ) {
	const vec3 t0 = (boundsMin - ray.origin) / ray.direction;
	const vec3 t1 = (boundsMax - ray.origin) / ray.direction;
	const vec3 tmin = min(t0, t1);
	const vec3 tmax = max(t0, t1);
	const float tStart = max(0, max( max(tmin.x, tmin.y), tmin.z ));
	const float tEnd = max(0, min( tmax.x, min(tmax.y, tmax.z) ) - tStart);
	return vec2(tStart, tEnd);
}

float sampleDensity( vec3 position ) {
	const vec3 uvw = position * ubo.fog.densityScale * 0.001 + ubo.fog.offset * 0.01;
	return max(0, texture(samplerNoise, uvw).r - ubo.fog.densityThreshold) * ubo.fog.densityMultiplier;
}

void fog( in Ray ray, inout vec3 i, float scale ) {
	if ( ubo.fog.stepScale <= 0 ) return;
	if ( ubo.fog.range.x == 0 || ubo.fog.range.y == 0 ) return;

#if FOG_RAY_MARCH
	const float range = ubo.fog.range.y;
	const vec3 boundsMin = vec3(-range,-range,-range) + ray.origin;
	const vec3 boundsMax = vec3(range,range,range) + ray.origin;
	const int numSteps = int(length(boundsMax - boundsMin) * ubo.fog.stepScale );

	const vec2 rayBoxInfo = rayBoxDst( boundsMin, boundsMax, ray );
	const float dstToBox = rayBoxInfo.x;
	const float dstInsideBox = rayBoxInfo.y;
	const float depth = surface.position.eye.z;

	float lightEnergy = 0;
	// march
	if ( 0 <= dstInsideBox && dstToBox <= depth ) {
		float dstTravelled = 0;
		float stepSize = dstInsideBox / numSteps;
		float dstLimit = min( depth - dstToBox, dstInsideBox );
		float totalDensity = 0;
		float transmittance = 1;
		while ( dstTravelled < dstLimit ) {
			vec3 rayPos = ray.origin + ray.direction * (dstToBox + dstTravelled);
			float density = sampleDensity(rayPos);
			if ( density > 0 ) {
				transmittance *= exp(-density * stepSize * ubo.fog.absorbtion);
				if ( transmittance < 0.01 ) break;
			}
			dstTravelled += stepSize;
		}
		i.rgb = mix(ubo.fog.color.rgb, i.rgb, transmittance);
	}
#endif

	const vec3 color = ubo.fog.color.rgb;
	const float inner = ubo.fog.range.x;
	const float outer = ubo.fog.range.y * scale;
	const float distance = length(-surface.position.eye);
	const float factor = clamp( (distance - inner) / (outer - inner), 0.0, 1.0 );

	i.rgb = mix(i.rgb, color, factor);
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
float mipLevel( in vec2 uv ) {
	const vec2 dx_vtc = dFdx(uv);
	const vec2 dy_vtc = dFdy(uv);
	return 0.5 * log2(max(dot(dx_vtc, dx_vtc), dot(dy_vtc, dy_vtc)));
}
bool validTextureIndex( int textureIndex ) {
	return 0 <= textureIndex; // && textureIndex < ubo.textures;
}
vec4 resolve( subpassInputMS t ) {
	const int samples = int(ubo.msaa);
	vec4 resolved = vec4(0);
	for ( int i = 0; i < samples; ++i ) {
		resolved += subpassLoad(t, i);
	}
	resolved /= vec4(samples);
	return resolved;
}
uvec4 resolve( usubpassInputMS t ) {
	const int samples = int(ubo.msaa);
	uvec4 resolved = uvec4(0);
	for ( int i = 0; i < samples; ++i ) {
		resolved += subpassLoad(t, i);
	}
	resolved /= uvec4(samples);
	return resolved;
}

#if !VXGI_SHADOWS
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
	if ( samples < 1 ) return eyeDepth < texture(samplerTextures[light.mapIndex], uv).r - bias ? 0.0 : factor;
	for ( int i = 0; i < samples; ++i ) {
		const int index = int( float(samples) * random(floor(surface.position.world.xyz * 1000.0), i)) % samples;
		const float lightDepth = texture(samplerTextures[light.mapIndex], uv + poissonDisk[index] / 700.0 ).r;
		if ( eyeDepth < lightDepth - bias ) factor -= 1.0 / samples;
	}
	return factor;
}
#endif

vec4 postProcess() {
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
	if ( (ubo.mode.type & (0x1 << 1)) == (0x1 << 1) ) whitenoise(surface.fragment.rgb);
#endif

	return vec4(surface.fragment.rgb,1);
}

Voxel getVoxel( vec3 P ) {
	const vec3 uvw = vec3( ubo.matrices.vxgi * vec4( P, 1.0f ) ) * 0.5f + 0.5f;

	Voxel voxel;
	voxel.id = uvec2(texture(voxelId[0], uvw).xy);
	voxel.position = P;
	voxel.normal = decodeNormals( texture(voxelNormal[0], uvw).xy );
	voxel.uv = texture(voxelUv[0], uvw).xy;
	voxel.color = texture(voxelRadiance[0], uvw).rgba;

	return voxel;
}

struct VoxelInfo {
	vec3 min;
	vec3 max;

	float mipmapLevels;
	float radianceSize;
	float voxelSize;

	float radianceSizeRecip;
	float voxelSizeRecip;
} voxelInfo;

vec4 voxelTrace( inout Ray ray, float aperture, float maxDistance ) {
#if VXGI_NDC
	ray.origin = vec3( ubo.matrices.vxgi * vec4( ray.origin, 1.0 ) );
	ray.direction = vec3( ubo.matrices.vxgi * vec4( ray.direction, 0.0 ) );
	const float granularity = 1.0f / 16.0f;
	const uint maxSteps = uint(voxelInfo.radianceSize * 2);
#else
	const float granularity = 10.0f;
	const uint maxSteps = uint(voxelInfo.radianceSize * granularity);
#endif
	const float granularityRecip = 1.0f / granularity;
	// box
	const vec2 rayBoxInfo = rayBoxDst( voxelInfo.min, voxelInfo.max, ray );
	const float tStart = rayBoxInfo.x;
	const float tEnd = maxDistance > 0 ? min(maxDistance, rayBoxInfo.y) : rayBoxInfo.y;
	// steps
	const float tDelta = voxelInfo.voxelSize * granularityRecip;
	// marcher
	ray.distance = tStart + tDelta * granularity;
	ray.position = vec3(0);
	vec4 radiance = vec4(0);
	vec3 uvw = vec3(0);
	// cone mipmap shit
	const float coneCoefficient = 2.0 * tan(aperture * 0.5);
	float coneDiameter = coneCoefficient * ray.distance;
	float level = aperture > 0 ? log2( coneDiameter ) : 0;
	// results
	vec4 color = vec4(0);
	float occlusion = 0.0;
	uint stepCounter = 0;
	const float falloff = 32.0f; // maxDistance > 0.0 ? 0.0f : 256.0f;
	const vec3 voxelBoundsRecip = 1.0f / (voxelInfo.max - voxelInfo.min);
//	while ( ray.distance < tEnd && color.a < 1.0 && occlusion < 1.0 && stepCounter++ < maxSteps ) {
	while ( ray.distance < tEnd && color.a < 1.0 && occlusion < 1.0 ) {
		ray.distance += tDelta * (aperture > 0 ? coneDiameter : 1);
		ray.position = ray.origin + ray.direction * ray.distance;
	#if VXGI_NDC
		uvw = ray.position * 0.5 + 0.5;
	#else
		uvw = (ray.position - voxelInfo.min) * voxelBoundsRecip;
	#endif
		if ( abs(uvw.x) > 1.0 || abs(uvw.y) > 1.0 || abs(uvw.z) > 1.0 ) break;
		coneDiameter = coneCoefficient * ray.distance;
		level = log2( coneDiameter );
		if ( level >= voxelInfo.mipmapLevels ) break;
		radiance = textureLod(voxelRadiance[0], uvw.xzy, level);

		color += (1.0 - color.a) * radiance;
		occlusion += ((1.0f - occlusion) * radiance.a) / (1.0f + falloff * coneDiameter);
	}
	return maxDistance > 0 ? color : vec4(color.rgb, occlusion);
//	return vec4(color.rgb, occlusion);
}
vec4 voxelConeTrace( inout Ray ray, float aperture ) {
	return voxelTrace( ray, aperture, 0 );
}
vec4 voxelTrace( inout Ray ray, float maxDistance ) {
	return voxelTrace( ray, 0, maxDistance );
}

#if VXGI_SHADOWS
float shadowFactor( const Light light, float def ) {
	const float SHADOW_APERTURE = 0.2;
	const float DEPTH_BIAS = 0.0;

	Ray ray;
	ray.origin = surface.position.world;
	ray.direction = normalize( light.position - surface.position.world );
	ray.origin -= ray.direction * 0.5;
	float z = distance( surface.position.world, light.position ) - DEPTH_BIAS;
	return 1.0 - voxelTrace( ray, SHADOW_APERTURE, z ).a;
}
#endif

void main() {
	{
	#if !MULTISAMPLING
		const float depth = subpassLoad(samplerDepth).r;
	#else
		const float depth = resolve( samplerDepth ).r;
	#endif

		vec4 positionEye = ubo.matrices.iProjection[inPushConstantPass] * vec4(inUv * 2.0 - 1.0, depth, 1.0);
		positionEye /= positionEye.w;
		surface.position.eye = positionEye.xyz;
		surface.position.world = vec3( ubo.matrices.iView[inPushConstantPass] * positionEye );
	}
#if 0
	{
		const vec4 near4 = ubo.matrices.iProjectionView[inPushConstantPass] * (vec4(2.0 * inUv - 1.0, -1.0, 1.0));
		const vec4 far4 = ubo.matrices.iProjectionView[inPushConstantPass] * (vec4(2.0 * inUv - 1.0, 1.0, 1.0));
		const vec3 near3 = near4.xyz / near4.w;
		const vec3 far3 = far4.xyz / far4.w;

		surface.ray.origin = near3;
		surface.ray.direction = normalize( far3 - near3 );
	}
	// separate our ray direction due to floating point precision problems
	{
		const mat4 iProjectionView = inverse( ubo.matrices.projection[inPushConstantPass] * mat4(mat3(ubo.matrices.view[inPushConstantPass])) );
		const vec4 near4 = iProjectionView * (vec4(2.0 * inUv - 1.0, -1.0, 1.0));
		const vec4 far4 = iProjectionView * (vec4(2.0 * inUv - 1.0, 1.0, 1.0));
		const vec3 near3 = near4.xyz / near4.w;
		const vec3 far3 = far4.xyz / far4.w;

		surface.ray.direction = normalize( far3 - near3 );
	}
#else
	{
		const mat4 iProjectionView = inverse( ubo.matrices.projection[inPushConstantPass] * mat4(mat3(ubo.matrices.view[inPushConstantPass])) );
		const vec4 near4 = iProjectionView * (vec4(2.0 * inUv - 1.0, -1.0, 1.0));
		const vec4 far4 = iProjectionView * (vec4(2.0 * inUv - 1.0, 1.0, 1.0));
		const vec3 near3 = near4.xyz / near4.w;
		const vec3 far3 = far4.xyz / far4.w;

		surface.ray.direction = normalize( far3 - near3 );
		surface.ray.origin = ubo.matrices.eyePos[inPushConstantPass].xyz;
	}
#endif
#if !MULTISAMPLING
	surface.normal.world = decodeNormals( subpassLoad(samplerNormal).xy );
	const uvec2 ID = subpassLoad(samplerId).xy;
#else
	surface.normal.world = decodeNormals( resolve(samplerNormal).xy );
	const uvec2 ID = resolve(samplerId).xy;
#endif
	surface.normal.eye = vec3( ubo.matrices.view[inPushConstantPass] * vec4(surface.normal.world, 0.0) );

	if ( ID.x == 0 || ID.y == 0 ) {
		surface.fragment.rgb = texture( samplerSkybox, surface.ray.direction ).rgb;
		surface.fragment.a = 0.0;
		outFragColor = postProcess();
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
	surface.uv = resolve(samplerUv).xy;
#endif
	const float mip = mipLevel(inUv.xy);
	const bool useAtlas = validTextureIndex( drawCall.textureIndex + material.indexAtlas );
	Texture textureAtlas;
	if ( useAtlas ) textureAtlas = textures[drawCall.textureIndex + material.indexAtlas];
	if ( validTextureIndex( drawCall.textureIndex + material.indexAlbedo ) ) {
		const Texture t = textures[drawCall.textureIndex + material.indexAlbedo];
		surface.material.albedo = textureLod( samplerTextures[(useAtlas)?textureAtlas.index:t.index], ( useAtlas ) ? mix( t.lerp.xy, t.lerp.zw, surface.uv ) : surface.uv, mip );
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
		surface.fragment += textureLod( samplerTextures[(useAtlas)?textureAtlas.index:t.index], ( useAtlas ) ? mix( t.lerp.xy, t.lerp.zw, surface.uv ) : surface.uv, mip );
	}
#else
#if !MULTISAMPLING
	surface.material.albedo = subpassLoad(samplerAlbedo);
#else
	surface.material.albedo = resolve(samplerAlbedo);
#endif
#endif
	surface.material.metallic = material.factorMetallic;
	surface.material.roughness = material.factorRoughness;
	surface.material.occlusion = material.factorOcclusion;
	// GI
	{
		const vec3 P = surface.position.world;
		const vec3 N = surface.normal.world;

	#if 1
		const vec3 right = normalize(orthogonal(N));
		const vec3 up = normalize(cross(right, N));

		const uint CONES_COUNT = 6;
		const vec3 CONES[] = {
			N,
			normalize(N + 0.0f * right + 0.866025f * up),
			normalize(N + 0.823639f * right + 0.267617f * up),
			normalize(N + 0.509037f * right + -0.7006629f * up),
			normalize(N + -0.50937f * right + -0.7006629f * up),
			normalize(N + -0.823639f * right + 0.267617f * up),
		};
	#else
		const vec3 ortho = normalize(orthogonal(N));
		const vec3 ortho2 = normalize(cross(ortho, N));

		const vec3 corner = 0.5f * (ortho + ortho2);
		const vec3 corner2 = 0.5f * (ortho - ortho2);

		const uint CONES_COUNT = 9;
		const vec3 CONES[] = {
			N,
			normalize(mix(N, ortho, 0.5)),
			normalize(mix(N, -ortho, 0.5)),
			normalize(mix(N, ortho2, 0.5)),
			normalize(mix(N, -ortho2, 0.5)),
			normalize(mix(N, corner, 0.5)),
			normalize(mix(N, -corner, 0.5)),
			normalize(mix(N, corner2, 0.5)),
			normalize(mix(N, -corner2, 0.5)),
		};
	#endif

		const float DIFFUSE_CONE_APERTURE = 0.57735f;
		const float DIFFUSE_INDIRECT_FACTOR = 1.0f / float(CONES_COUNT);
		
		const float SPECULAR_CONE_APERTURE = clamp(tan(PI * 0.5f * surface.material.roughness), 0.0174533f, PI); // tan( R * PI * 0.5f * 0.1f );
		const float SPECULAR_INDIRECT_FACTOR = (1.0 - surface.material.metallic) * 0.5; // 1.0f;
		
		vec4 indirectDiffuse = vec4(0);
		vec4 indirectSpecular = vec4(0);

		{
			voxelInfo.radianceSize = textureSize( voxelRadiance[0], 0 ).x;
			voxelInfo.radianceSizeRecip = 1.0 / voxelInfo.radianceSize;
			voxelInfo.mipmapLevels = log2(voxelInfo.radianceSize) + 1;
		#if VXGI_NDC
			voxelInfo.min = vec3( -1 );
			voxelInfo.max = vec3(  1 );
			voxelInfo.voxelSize = voxelInfo.radianceSizeRecip;
		#else
			const mat4 inverseOrtho = inverse( ubo.matrices.vxgi );
			voxelInfo.min = vec3( inverseOrtho * vec4( -1, -1, -1, 1 ) );
			voxelInfo.max = vec3( inverseOrtho * vec4(  1,  1,  1, 1 ) );
			voxelInfo.voxelSize = 1;
		#endif
			voxelInfo.voxelSizeRecip = 1.0 / voxelInfo.voxelSize;
		}
	//	outFragColor.rgb = voxelConeTrace( surface.ray, 0 ).rgb; return;

		Ray ray;
		if ( DIFFUSE_INDIRECT_FACTOR > 0.0f ) {
			for ( uint i = 0; i < CONES_COUNT; ++i ) {
				float weight = i == 0 ? PI * 0.25f : PI * 0.15f;
				ray.origin = P;
				ray.direction = CONES[i].xyz;
				indirectDiffuse += voxelConeTrace( ray, DIFFUSE_CONE_APERTURE ) * weight;
			}
			surface.material.occlusion = 1.0 - clamp(indirectDiffuse.a, 0.0, 1.0);
		// 	outFragColor.rgb = indirectDiffuse.rgb; return;
		//	outFragColor.rgb = vec3(surface.material.occlusion); return;
		}
		if ( SPECULAR_INDIRECT_FACTOR > 0.0f ) {
			ray.origin = P;
			ray.direction = reflect( normalize(P - surface.ray.origin), N );
			indirectSpecular = voxelConeTrace( ray, SPECULAR_CONE_APERTURE );
		// 	outFragColor.rgb = indirectSpecular.rgb; return;
		/*
			if ( indirectSpecular.a < 1.0 ) {
				vec4 radiance = texture( samplerSkybox, ray.direction ) * 0.25;
				indirectSpecular += (1.0 - indirectSpecular.a) * radiance;
			}
		*/
		}
		surface.material.indirect = indirectDiffuse * DIFFUSE_INDIRECT_FACTOR + indirectSpecular * SPECULAR_INDIRECT_FACTOR;
	//	outFragColor.rgb = surface.material.indirect.rgb; return;
	}

	if ( 0 <= material.indexLightmap ) {
		surface.fragment.rgb += surface.material.albedo.rgb + ubo.ambient.rgb * surface.material.occlusion + surface.material.indirect.rgb;
	} else {
		surface.fragment.rgb += surface.material.albedo.rgb * ubo.ambient.rgb * surface.material.occlusion + surface.material.indirect.rgb;
	}
#if DEFERRED_SAMPLING
	// deferred sampling doesn't have a blended albedo buffer
	// in place we'll just cone trace behind the window
	if ( surface.material.albedo.a < 1.0 ) {
		Ray ray;
		ray.origin = surface.position.world;
		ray.direction = surface.ray.direction;
		vec4 radiance = voxelConeTrace( ray, surface.material.albedo.a );
		surface.fragment.rgb += (1.0 - surface.material.albedo.a) * radiance.rgb;
	}
#endif

	// corrections
	surface.material.roughness *= 4.0;

	{
		const vec3 F0 = mix(vec3(0.04), surface.material.albedo.rgb, surface.material.metallic); 
		const vec3 Lo = normalize( -surface.position.eye );
		const float cosLo = max(0.0, dot(surface.normal.eye, Lo));
		
		for ( uint i = 0; i < ubo.lights; ++i ) {
			const Light light = lights[i];
			if ( light.power <= LIGHT_POWER_CUTOFF ) continue;
			const vec3 Lp = light.position;
			const vec3 Liu = vec3(ubo.matrices.view[inPushConstantPass] * vec4(light.position, 1)) - surface.position.eye;
			const vec3 Li = normalize(Liu);
		#if VXGI_SHADOWS
			const float Ls = i < ubo.shadowSamples ? shadowFactor( light, 0.0 ) : 1.0;
		#else
			const float Ls = shadowFactor( light, 0.0 );
		#endif
			const float La = 1.0 / (PI * pow(length(Liu), 2.0));
			if ( light.power * La * Ls <= LIGHT_POWER_CUTOFF ) continue;

			const float cosLi = max(0.0, dot(surface.normal.eye, Li));
			const vec3 Lr = light.color.rgb * light.power * La * Ls;
		#if LAMBERT
			const vec3 diffuse = surface.material.albedo.rgb;
			const vec3 specular = vec3(0);
		#elif PBR
			const vec3 Lh = normalize(Li + Lo);
			const float cosLh = max(0.0, dot(surface.normal.eye, Lh));
			
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

	outFragColor = postProcess();
}