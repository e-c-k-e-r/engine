#version 450
#extension GL_EXT_samplerless_texture_functions : require

#define MULTISAMPLING 1
#define RAY_MARCH_FOG 1
#define UF_DEFERRED_SAMPLING 0

layout (constant_id = 0) const uint TEXTURES = 256;

struct Matrices {
	mat4 view[2];
	mat4 projection[2];
	mat4 iView[2];
	mat4 iProjection[2];
	mat4 iProjectionView[2];
	mat4 voxel;
};

struct Space {
	vec3 eye;
	vec3 world;
} position, normal, view;

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
	#if UF_DEFERRED_SAMPLING
		layout (input_attachment_index = 2, binding = 2) uniform subpassInput samplerUv;
	#else
		layout (input_attachment_index = 2, binding = 2) uniform subpassInput samplerAlbedo;
	#endif
	layout (input_attachment_index = 3, binding = 3) uniform subpassInput samplerDepth;
#else
	layout (input_attachment_index = 0, binding = 0) uniform usubpassInputMS samplerId;
	layout (input_attachment_index = 1, binding = 1) uniform subpassInputMS samplerNormal;
	#if UF_DEFERRED_SAMPLING
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
	float kexp;

	uint msaa;
	uint poissonSamples;
	uint padding1;
	uint padding2;
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

layout (binding = 9) uniform usampler3D voxelId;
layout (binding = 10) uniform sampler3D voxelNormal;
layout (binding = 11) uniform sampler3D voxelUv;
layout (binding = 12) uniform sampler3D voxelAlbedo;

layout (binding = 13) uniform sampler3D samplerNoise;
layout (binding = 14) uniform samplerCube samplerSkybox;
layout (binding = 15) uniform sampler2D samplerTextures[TEXTURES];

layout (location = 0) in vec2 inUv;
layout (location = 1) in flat uint inPushConstantPass;

layout (location = 0) out vec4 outFragColor;
layout (location = 1) out vec4 outDebugValue;

vec2 poissonDisk[16] = vec2[]( 
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

const float PI = 3.14159265359;
const float EPSILON = 0.00001;

// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2.
float ndfGGX(float cosLh, float roughness) {
	float alpha   = roughness * roughness;
	float alphaSq = alpha * alpha;

	float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
}

// Single term for separable Schlick-GGX below.
float gaSchlickG1(float cosTheta, float k) {
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method.
float gaSchlickGGX(float cosLi, float cosLo, float roughness) {
	float r = roughness + 1.0;
	float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights.
	return gaSchlickG1(cosLi, k) * gaSchlickG1(cosLo, k);
}
vec3 fresnelSchlick(vec3 F0, float cosTheta) {
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float random(vec3 seed, int i){
	vec4 seed4 = vec4(seed,i);
	float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
	return fract(sin(dot_product) * 43758.5453);
}

float shadowFactor( Light light, uint shadowMap, float def ) {
	vec4 positionClip = light.projection * light.view * vec4(position.world, 1.0);
	positionClip.xyz /= positionClip.w;

	if ( positionClip.x < -1 || positionClip.x >= 1 ) return def; //0.0;
	if ( positionClip.y < -1 || positionClip.y >= 1 ) return def; //0.0;
	if ( positionClip.z <= 0 || positionClip.z >= 1 ) return def; //0.0;

	float factor = 1.0;

	// spot light
	if ( light.type == 1 || light.type == 2 ) {
		float dist = length( positionClip.xy );
		if ( dist > 0.5 ) return def; //0.0;
		
		// spot light with attenuation
		if ( light.type == 2 ) {
			factor = 1.0 - (pow(dist * 2,2.0));
		}
	}
	
	vec2 uv = positionClip.xy * 0.5 + 0.5;
	float bias = light.depthBias;

	float eyeDepth = positionClip.z;

	int samples = int(ubo.poissonSamples);
	if ( samples <= 1 ) {
		return eyeDepth < texture(samplerTextures[shadowMap], uv).r - bias ? 0.0 : factor;
	}
	for ( int i = 0; i < samples; ++i ) {
	//	int index = i;
	//	int index = int( float(samples) * random(gl_FragCoord.xyy, i) ) % samples;
		int index = int( float(samples) * random(floor(position.world.xyz * 1000.0), i)) % samples;
		float lightDepth = texture(samplerTextures[shadowMap], uv + poissonDisk[index] / 700.0 ).r;
		if ( eyeDepth < lightDepth - bias ) factor -= 1.0 / samples;
	}
	return factor;
}
// Returns a vector that is orthogonal to u.
vec3 orthogonal(vec3 u){
	u = normalize(u);
	vec3 v = vec3(0.99146, 0.11664, 0.05832); // Pick any normalized vector.
	return abs(dot(u, v)) > 0.99999f ? cross(u, vec3(0, 1, 0)) : cross(u, v);
}
float rand2(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 143758.5453);
}
float rand3(vec3 co){
    return fract(sin(dot(co.xyz ,vec3(12.9898,78.233, 37.719))) * 143758.5453);
}
void whitenoise(inout vec3 color) {
	float flicker = ubo.mode.parameters.x;
	float pieces = ubo.mode.parameters.y;
	float blend = ubo.mode.parameters.z;
	float time = ubo.mode.parameters.w;
	if ( blend < 0.0001 ) return;
	float freq = sin(pow(mod(time, flicker) + flicker, 1.9));
//	float whiteNoise = rand3( floor(position.world / pieces) + floor(time * 2) );
	float whiteNoise = rand2( floor(gl_FragCoord.xy / pieces) + mod(time, freq) );
	color = mix( color, vec3(whiteNoise), blend );
}
vec3 gamma( vec3 i ) {
	return pow(i.rgb, vec3(1.0 / 2.2));
}

vec2 rayBoxDst( vec3 boundsMin, vec3 boundsMax, vec3 rayO, vec3 rayD ) {
	vec3 t0 = (boundsMin - rayO) / rayD;
	vec3 t1 = (boundsMax - rayO) / rayD;
	vec3 tmin = min(t0, t1);
	vec3 tmax = max(t0, t1);
	float dstA = max( max(tmin.x, tmin.y), tmin.z );
	float dstB = min( tmax.x, min(tmax.y, tmax.z) );
	float tStart = max(0, dstA);
	float tEnd = max(0, dstB - tStart);
	return vec2(tStart, tEnd);
}

float sampleDensity( vec3 position ) {
	vec3 uvw = position * ubo.fog.densityScale * 0.001 + ubo.fog.offset * 0.01;
	return max(0, texture(samplerNoise, uvw).r - ubo.fog.densityThreshold) * ubo.fog.densityMultiplier;
}

void fog( vec3 rayO, vec3 rayD, inout vec3 i, float scale ) {
	if ( ubo.fog.stepScale <= 0 ) return;
	if ( ubo.fog.range.x == 0 || ubo.fog.range.y == 0 ) return;

#if RAY_MARCH_FOG
	float range = ubo.fog.range.y;
	vec3 boundsMin = vec3(-range,-range,-range) + rayO;
	vec3 boundsMax = vec3(range,range,range) + rayO;
	int numSteps = int(length(boundsMax - boundsMin) * ubo.fog.stepScale );

	vec2 rayBoxInfo = rayBoxDst( boundsMin, boundsMax, rayO, rayD );
	float dstToBox = rayBoxInfo.x;
	float dstInsideBox = rayBoxInfo.y;
	float depth = position.eye.z;

	float lightEnergy = 0;
	// march
	if ( 0 <= dstInsideBox && dstToBox <= depth ) {
		float dstTravelled = 0;
		float stepSize = dstInsideBox / numSteps;
		float dstLimit = min( depth - dstToBox, dstInsideBox );
		float totalDensity = 0;
		float transmittance = 1;
		while ( dstTravelled < dstLimit ) {
			vec3 rayPos = rayO + rayD * (dstToBox + dstTravelled);
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

	vec3 color = ubo.fog.color.rgb;
	float inner = ubo.fog.range.x;
	float outer = ubo.fog.range.y * scale;
	float distance = length(-position.eye);
	float factor = (distance - inner) / (outer - inner);
	factor = clamp( factor, 0.0, 1.0 );

	i.rgb = mix(i.rgb, color, factor);
}

vec3 decodeNormals( vec2 enc ) {
#define kPI 3.1415926536f
	vec2 ang = enc*2-1;
	vec2 scth = vec2( sin(ang.x * kPI), cos(ang.x * kPI) );
	vec2 scphi = vec2(sqrt(1.0 - ang.y*ang.y), ang.y);
	return normalize( vec3(scth.y*scphi.x, scth.x*scphi.x, scphi.y) );
/*
	vec2 fenc = enc*4-2;
	float f = dot(fenc,fenc);
	float g = sqrt(1-f/4);
	return normalize( vec3(fenc * g, 1 - f / 2) );
*/
}
float wrap( float i ) {
	return fract(i);
}
vec2 wrap( vec2 uv ) {
	return vec2( wrap( uv.x ), wrap( uv.y ) );
}
float mipLevel( in vec2 uv ) {
	vec2 dx_vtc = dFdx(uv);
	vec2 dy_vtc = dFdy(uv);
	return 0.5 * log2(max(dot(dx_vtc, dx_vtc), dot(dy_vtc, dy_vtc)));
}
bool validTextureIndex( int textureIndex ) {
	return 0 <= textureIndex; // && textureIndex < ubo.textures;
}
vec4 resolve( subpassInputMS t ) {
	int samples = int(ubo.msaa);
	vec4 resolved = vec4(0);
	for ( int i = 0; i < samples; ++i ) {
		resolved += subpassLoad(t, i);
	}
	resolved /= vec4(samples);
	return resolved;
}
uvec4 resolve( usubpassInputMS t ) {
	int samples = int(ubo.msaa);
	uvec4 resolved = uvec4(0);
	for ( int i = 0; i < samples; ++i ) {
		resolved += subpassLoad(t, i);
	}
	resolved /= uvec4(samples);
	return resolved;
}

Voxel getVoxel( vec3 P ) {
	vec3 uvw = vec3( ubo.matrices.voxel * vec4( P, 1.0f ) ) * 0.5f + 0.5f;

	Voxel voxel;
	voxel.id = uvec2(texture(voxelId, uvw).xy);
	voxel.position = P;
	voxel.normal = decodeNormals( texture(voxelNormal, uvw).xy );
	voxel.uv = texture(voxelUv, uvw).xy;
	voxel.color = texture(voxelAlbedo, uvw).rgba;

	return voxel;
}

#define VOXEL_TRACE_IN_NDC 0
vec4 voxelConeTrace( vec3 rayO, vec3 rayD, float aperture ) {
	// bounds
	float albedoSize = textureSize( voxelAlbedo, 0 ).x;
	float mipmapLevels = textureQueryLod( voxelAlbedo, vec3(0) ).x;
#if VOXEL_TRACE_IN_NDC
	rayO = vec3( ubo.matrices.voxel * vec4( rayO, 1.0 ) );
	rayD = vec3( ubo.matrices.voxel * vec4( rayD, 0.0 ) );

	vec3 boundsMin = vec3( -1 );
	vec3 boundsMax = vec3(  1 );
	float voxelSize = 1.0 / albedoSize;
#else
	mat4 inverseOrtho = inverse( ubo.matrices.voxel );
	vec3 boundsMin = vec3( inverseOrtho * vec4( -1, -1, -1, 1 ) );
	vec3 boundsMax = vec3( inverseOrtho * vec4(  1,  1,  1, 1 ) );
	float voxelSize = 1;
#endif
	float granularity = 1.0 / 6.0; // (2.0 * sqrt(2.0) );
	// box
	vec2 rayBoxInfo = rayBoxDst( boundsMin, boundsMax, rayO, rayD );
	float tStart = rayBoxInfo.x;
	float tEnd = rayBoxInfo.y;
	// steps
	float tDelta = voxelSize * granularity;
	uint maxSteps = uint(albedoSize / granularity);
	// marcher
	float t = tStart + tDelta * 2.0;
	vec3 rayPos = vec3(0);
	vec4 radiance = vec4(0);
	vec3 uvw = vec3(0);
	// cone mipmap shit
	float coneCoefficient = 2.0 * tan(aperture * 0.5);
	float coneDiameter = coneCoefficient * t;
	float level = aperture > 0 ? log2( coneDiameter / albedoSize ) : 0;
	// results
	vec4 color = vec4(0);
	float occlusion = 0.0;
	// do
	uint stepCounter = 0;
	while ( t < tEnd && occlusion < 1.0 && stepCounter++ < maxSteps ) {
		t += tDelta;
		rayPos = rayO + rayD * t;
	#if VOXEL_TRACE_IN_NDC
		uvw = rayPos * 0.5 + 0.5;
	#else
		uvw = (rayPos - boundsMin) / (boundsMax - boundsMin); // vec3( ubo.matrices.voxel * vec4( rayPos, 1 ) ) * 0.5 + 0.5;
	#endif
		if ( abs(uvw.x) > 1.0 || abs(uvw.y) > 1.0 || abs(uvw.z) > 1.0 ) break;
		coneDiameter = coneCoefficient * t;
		level = log2( coneDiameter / albedoSize );
		radiance = texture(voxelAlbedo, uvw, level) * granularity;
	
		occlusion += radiance.a;
		color.rgb += (1.0 - occlusion) * radiance.rgb;
	}
	return vec4(color.rgb, occlusion);
}

void main() {
	vec3 rayO = vec3(0);
	vec3 rayD = vec3(0);
	vec3 fragColor = vec3(0);

	{
	#if !MULTISAMPLING
		float depth = subpassLoad(samplerDepth).r;
	#else
		float depth = resolve( samplerDepth ).r;
	#endif

		vec4 positionClip = vec4(inUv * 2.0 - 1.0, depth, 1.0);
		vec4 positionEye = ubo.matrices.iProjection[inPushConstantPass] * positionClip;
		positionEye /= positionEye.w;
		position.eye = positionEye.xyz;

		vec4 positionWorld = ubo.matrices.iView[inPushConstantPass] * positionEye;
		position.world = positionWorld.xyz;
	}
	{
		vec4 near4 = ubo.matrices.iProjectionView[inPushConstantPass] * (vec4(2.0 * inUv - 1.0, -1.0, 1.0));
		vec4 far4 = ubo.matrices.iProjectionView[inPushConstantPass] * (vec4(2.0 * inUv - 1.0, 1.0, 1.0));
		vec3 near3 = near4.xyz / near4.w;
		vec3 far3 = far4.xyz / far4.w;

		rayO = near3;
	}
	{
		mat4 iProjectionView = inverse( ubo.matrices.projection[inPushConstantPass] * mat4(mat3(ubo.matrices.view[inPushConstantPass])) );
		vec4 near4 = iProjectionView * (vec4(2.0 * inUv - 1.0, -1.0, 1.0));
		vec4 far4 = iProjectionView * (vec4(2.0 * inUv - 1.0, 1.0, 1.0));
		vec3 near3 = near4.xyz / near4.w;
		vec3 far3 = far4.xyz / far4.w;

		rayD = normalize( far3 - near3 );
	}
#if !MULTISAMPLING
	normal.world = decodeNormals( subpassLoad(samplerNormal).xy );
	uvec2 ID = subpassLoad(samplerId).xy;
#else
	normal.world = decodeNormals( resolve(samplerNormal).xy );
	uvec2 ID = resolve(samplerId).xy;
#endif
	normal.eye = vec3( ubo.matrices.view[inPushConstantPass] * vec4(normal.world, 0.0) );

	uint drawId = ID.x;
	uint materialId = ID.y;
	if ( drawId == 0 || materialId == 0 ) {
		fragColor.rgb = texture( samplerSkybox, rayD ).rgb;
		fog(rayO, rayD, fragColor, 0.0);
		outFragColor = vec4(fragColor,1);
		return;
	}
	--drawId;
	--materialId;

	DrawCall drawCall = drawCalls[drawId];
	materialId += drawCall.materialIndex;

	Material material = materials[materialId];
	vec4 C = material.colorBase;
#if UF_DEFERRED_SAMPLING
#if !MULTISAMPLING
	vec2 uv = subpassLoad(samplerUv).xy;
#else
	vec2 uv = resolve(samplerUv).xy;
#endif
	bool useAtlas = validTextureIndex( drawCall.textureIndex + material.indexAtlas );
	Texture textureAtlas;
	if ( useAtlas ) textureAtlas = textures[drawCall.textureIndex + material.indexAtlas];
	if ( validTextureIndex( drawCall.textureIndex + material.indexAtlas ) ) {
		Texture t = textures[drawCall.textureIndex + material.indexAlbedo];
		C = texture( samplerTextures[(useAtlas)?textureAtlas.index:t.index], ( useAtlas ) ? mix( t.lerp.xy, t.lerp.zw, uv ) : uv, mip );
	}
	// OPAQUE
	if ( material.modeAlpha == 0 ) {
		C.a = 1;
	// BLEND
	} else if ( material.modeAlpha == 1 ) {

	// MASK
	} else if ( material.modeAlpha == 2 ) {

	}
#else
#if !MULTISAMPLING
	C = subpassLoad(samplerAlbedo);
#else
	C = resolve(samplerAlbedo);
#endif
#endif
	float M = material.factorMetallic;
	float R = material.factorRoughness;
	float AO = material.factorOcclusion;

	vec4 indirectLighting = vec4(0);
	// GI
	{
		vec3 P = position.world;
		vec3 N = normal.world;

	//	outFragColor = voxelConeTrace(rayO, rayD, 0 );
	//	return;

		const float ROUGHNESS_SCALE = clamp(sqrt( 0.111111f / ( R * R + 0.111111f ) ), 0.0, 1.0);
		const float ANGLE_MIX = 0.5f;
		const float DIFFUSE_CONE_APERTURE = 0.325;
		const float DIFFUSE_INDIRECT_FACTOR = ROUGHNESS_SCALE; //0.11111f * 0.111111f;
		
		const vec3 ortho = normalize(orthogonal(N));
		const vec3 ortho2 = normalize(cross(ortho, N));
		const vec3 corner = 0.5f * (ortho + ortho2);
		const vec3 corner2 = 0.5f * (ortho - ortho2);

		const vec3 CONES[9] = {
			N,

			mix(N, ortho, ANGLE_MIX),
			mix(N, -ortho, ANGLE_MIX),
			mix(N, ortho2, ANGLE_MIX),
			mix(N, -ortho2, ANGLE_MIX),

			mix(N, corner, ANGLE_MIX),
			mix(N, -corner, ANGLE_MIX),
			mix(N, corner2, ANGLE_MIX),
			mix(N, -corner2, ANGLE_MIX),
		};

		vec4 indirectDiffuse = vec4(0);
		vec4 indirectSpecular = vec4(0);

	//	indirectDiffuse += voxelConeTrace( P, N, 0.325 );
		for ( uint i = 0; i < 1; ++i ) {
			indirectDiffuse += voxelConeTrace(P, CONES[i], DIFFUSE_CONE_APERTURE );
		}

		const float SPECULAR_CONE_APERTURE = acos( ROUGHNESS_SCALE );
		const float SPECULAR_INDIRECT_FACTOR = ROUGHNESS_SCALE; // 0.1;

		vec3 R = reflect( normalize(P - rayO), N );
		indirectSpecular = voxelConeTrace( P, R, SPECULAR_CONE_APERTURE );
		indirectLighting = indirectDiffuse * DIFFUSE_INDIRECT_FACTOR + indirectSpecular * SPECULAR_INDIRECT_FACTOR;
	//	AO = 1.0 - clamp(indirectDiffuse.a, 0.0, 1.0);
	//	outFragColor.rgb = vec3(AO);
	//	return;

	//	outFragColor.rgb = indirectLighting.rgb;
	//	outFragColor.rgb = indirectDiffuse.rgb;
	//	outFragColor.rgb = indirectSpecular.rgb;
	//	return;
	}
	R *= 2.0f;

	bool usePbr = true;
	bool gammaCorrect = false;
	float litFactor = 1.0;
	bool useLightmap = 0 <= material.indexLightmap;
	if ( useLightmap ) {
		fragColor = C.rgb + ubo.ambient.rgb + indirectLighting.rgb;
	} else {
		fragColor = C.rgb * ubo.ambient.rgb * (1 - AO) + indirectLighting.rgb;
	}
	{
		const float LIGHT_POWER_CUTOFF = 0.0001;
		vec3 N = normal.eye;
		vec3 F0 = mix(vec3(0.04), C.rgb, M); 
		for ( uint i = 0; i < ubo.lights; ++i ) {
			Light light = lights[i];
			if ( light.power <= LIGHT_POWER_CUTOFF ) continue;
			light.position = vec3(ubo.matrices.view[inPushConstantPass] * vec4(light.position.xyz, 1));
			if ( validTextureIndex(light.mapIndex) ) {
				float factor = shadowFactor( light, light.mapIndex, 0.0 );
				light.power *= factor;
				litFactor += light.power;
			}
			vec3 Li = light.position - position.eye;
			light.power *= 1.0 / (PI * pow(length(Li), 2.0));
			if ( light.power <= LIGHT_POWER_CUTOFF ) continue;

			     Li = normalize(Li);
			vec3 Lo = normalize( -position.eye );
			vec3 Lh = normalize(Li + Lo);
			
			vec3 Lradiance = light.color.rgb * light.power;
			vec3 albedo = C.rgb;

			float cosLi = max(0.0, dot(N, Li));
			float cosLo = max(0.0, dot(N, Lo));
			float cosLh = max(0.0, dot(N, Lh));

			vec3 F = fresnelSchlick( F0, max( 0.0, dot(Lh, Lo) ) );
			float D = ndfGGX( cosLh, R );
			float G = gaSchlickGGX(cosLi, cosLo, R);

			vec3 Kd = mix( vec3(1.0) - F, vec3(0.0), M );
			vec3 diffuseBRDF = Kd * albedo;
			vec3 specularBRDF = (F * D * G) / max(EPSILON, 4.0 * cosLi * cosLo);
			if ( useLightmap ) {
				fragColor.rgb += (specularBRDF) * Lradiance * cosLi;
			} else {
				fragColor.rgb += (diffuseBRDF + specularBRDF) * Lradiance * cosLi;
			}
		}
	}
	if ( gammaCorrect ) fragColor = gamma( fragColor );

	fog(rayO, rayD, fragColor, litFactor);

	if ( (ubo.mode.type & (0x1 << 1)) == (0x1 << 1) ) {
		whitenoise(fragColor);
	}
	outFragColor = vec4(fragColor,1);

}