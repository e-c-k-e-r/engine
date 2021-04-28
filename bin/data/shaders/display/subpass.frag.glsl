#version 450
#extension GL_EXT_samplerless_texture_functions : require

#define MULTISAMPLING 1
#define DEFERRED_SAMPLING 0
#define RAY_MARCH_FOG 1

#define LAMBERT 0
#define PBR 1

const float PI = 3.14159265359;
const float EPSILON = 0.00001;

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

struct Matrices {
	mat4 view[2];
	mat4 projection[2];
	mat4 iView[2];
	mat4 iProjection[2];
	mat4 iProjectionView[2];
	mat4 ortho;
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
	float kexp;

	uint msaa;
	uint poissonSamples;
	float gamma;
	float exposure;
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

layout (binding = 9) uniform sampler3D samplerNoise;
layout (binding = 10) uniform samplerCube samplerSkybox;
layout (binding = 11) uniform sampler2D samplerTextures[TEXTURES];


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

vec2 rayBoxDst( vec3 boundsMin, vec3 boundsMax, vec3 rayO, vec3 rayD ) {
	const vec3 t0 = (boundsMin - rayO) / rayD;
	const vec3 t1 = (boundsMax - rayO) / rayD;
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

void fog( vec3 rayO, vec3 rayD, inout vec3 i, float scale ) {
	if ( ubo.fog.stepScale <= 0 ) return;
	if ( ubo.fog.range.x == 0 || ubo.fog.range.y == 0 ) return;

#if RAY_MARCH_FOG
	const float range = ubo.fog.range.y;
	const vec3 boundsMin = vec3(-range,-range,-range) + rayO;
	const vec3 boundsMax = vec3(range,range,range) + rayO;
	const int numSteps = int(length(boundsMax - boundsMin) * ubo.fog.stepScale );

	const vec2 rayBoxInfo = rayBoxDst( boundsMin, boundsMax, rayO, rayD );
	const float dstToBox = rayBoxInfo.x;
	const float dstInsideBox = rayBoxInfo.y;
	const float depth = position.eye.z;

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

	const vec3 color = ubo.fog.color.rgb;
	const float inner = ubo.fog.range.x;
	const float outer = ubo.fog.range.y * scale;
	const float distance = length(-position.eye);
	const float factor = clamp( (distance - inner) / (outer - inner), 0.0, 1.0 );

	i.rgb = mix(i.rgb, color, factor);
}

vec3 decodeNormals( vec2 enc ) {
	const vec2 ang = enc*2-1;
	const vec2 scth = vec2( sin(ang.x * PI), cos(ang.x * PI) );
	const vec2 scphi = vec2(sqrt(1.0 - ang.y*ang.y), ang.y);
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

float shadowFactor( const Light light, float def ) {
	if ( !validTextureIndex(light.mapIndex) ) return 1.0;

	vec4 positionClip = light.projection * light.view * vec4(position.world, 1.0);
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
	const int samples = int(ubo.poissonSamples);
	if ( samples <= 1 ) return eyeDepth < texture(samplerTextures[light.mapIndex], uv).r - bias ? 0.0 : factor;
	for ( int i = 0; i < samples; ++i ) {
		const int index = int( float(samples) * random(floor(position.world.xyz * 1000.0), i)) % samples;
		const float lightDepth = texture(samplerTextures[light.mapIndex], uv + poissonDisk[index] / 700.0 ).r;
		if ( eyeDepth < lightDepth - bias ) factor -= 1.0 / samples;
	}
	return factor;
}

void main() {
	vec3 rayO = vec3(0);
	vec3 rayD = vec3(0);
	vec3 fragColor = vec3(0);

	{
	#if !MULTISAMPLING
		const float depth = subpassLoad(samplerDepth).r;
	#else
		const float depth = resolve( samplerDepth ).r;
	#endif

		vec4 positionEye = ubo.matrices.iProjection[inPushConstantPass] * vec4(inUv * 2.0 - 1.0, depth, 1.0);
		positionEye /= positionEye.w;
		position.eye = positionEye.xyz;
		position.world = vec3( ubo.matrices.iView[inPushConstantPass] * positionEye );
	}
	{
		const vec4 near4 = ubo.matrices.iProjectionView[inPushConstantPass] * (vec4(2.0 * inUv - 1.0, -1.0, 1.0));
		const vec4 far4 = ubo.matrices.iProjectionView[inPushConstantPass] * (vec4(2.0 * inUv - 1.0, 1.0, 1.0));
		const vec3 near3 = near4.xyz / near4.w;
		const vec3 far3 = far4.xyz / far4.w;

		rayO = near3;
		rayD = normalize( far3 - near3 );
	}
	// separate our ray direction due to floating point precision problems
	if ( false ) {
		const mat4 iProjectionView = inverse( ubo.matrices.projection[inPushConstantPass] * mat4(mat3(ubo.matrices.view[inPushConstantPass])) );
		const vec4 near4 = iProjectionView * (vec4(2.0 * inUv - 1.0, -1.0, 1.0));
		const vec4 far4 = iProjectionView * (vec4(2.0 * inUv - 1.0, 1.0, 1.0));
		const vec3 near3 = near4.xyz / near4.w;
		const vec3 far3 = far4.xyz / far4.w;

		rayD = normalize( far3 - near3 );
	}
#if !MULTISAMPLING
	normal.world = decodeNormals( subpassLoad(samplerNormal).xy );
	const uvec2 ID = subpassLoad(samplerId).xy;
#else
	normal.world = decodeNormals( resolve(samplerNormal).xy );
	const uvec2 ID = resolve(samplerId).xy;
#endif
	normal.eye = vec3( ubo.matrices.view[inPushConstantPass] * vec4(normal.world, 0.0) );

	if ( ID.x == 0 || ID.y == 0 ) {
		fragColor.rgb = texture( samplerSkybox, rayD ).rgb;
		fog(rayO, rayD, fragColor, 0.0);
		outFragColor = vec4(fragColor,1);
		return;
	}
	const uint drawId = ID.x - 1;
	const DrawCall drawCall = drawCalls[drawId];
	const uint materialId = ID.y + drawCall.materialIndex - 1;
	const Material material = materials[materialId];
	vec4 A = material.colorBase;
#if DEFERRED_SAMPLING
#if !MULTISAMPLING
	const vec2 uv = subpassLoad(samplerUv).xy;
#else
	const vec2 uv = resolve(samplerUv).xy;
#endif
	const float mip = mipLevel(inUv.xy);
	const bool useAtlas = validTextureIndex( drawCall.textureIndex + material.indexAtlas );
	Texture textureAtlas;
	if ( useAtlas ) textureAtlas = textures[drawCall.textureIndex + material.indexAtlas];
	if ( validTextureIndex( drawCall.textureIndex + material.indexAtlas ) ) {
		Texture t = textures[drawCall.textureIndex + material.indexAlbedo];
		A = texture( samplerTextures[(useAtlas)?textureAtlas.index:t.index], ( useAtlas ) ? mix( t.lerp.xy, t.lerp.zw, uv ) : uv, mip );
	}
	// OPAQUE
	if ( material.modeAlpha == 0 ) {
		A.a = 1;
	// BLEND
	} else if ( material.modeAlpha == 1 ) {

	// MASK
	} else if ( material.modeAlpha == 2 ) {

	}
#else
#if !MULTISAMPLING
	A = subpassLoad(samplerAlbedo);
#else
	A = resolve(samplerAlbedo);
#endif
#endif
	const float M = material.factorMetallic;
	const float R = material.factorRoughness;
	const float AO = 1.0f - material.factorOcclusion;

	float litFactor = 1.0;
	if ( 0 <= material.indexLightmap ) {
		fragColor = A.rgb + ubo.ambient.rgb * (1 - AO);
	} else {
		fragColor = A.rgb * ubo.ambient.rgb * (1 - AO);
	}
	{
		const vec3 N = normal.eye;
		const vec3 F0 = mix(vec3(0.04), A.rgb, M); 
		const vec3 Lo = normalize( -position.eye );
		const float cosLo = max(0.0, dot(N, Lo));

		for ( uint i = 0; i < ubo.lights; ++i ) {
			const Light light = lights[i];
			if ( light.power <= LIGHT_POWER_CUTOFF ) continue;
			const vec3 Lp = light.position;
			const vec3 Liu = vec3(ubo.matrices.view[inPushConstantPass] * vec4(light.position, 1)) - position.eye;
			const vec3 Li = normalize(Liu);
			const float Ls = shadowFactor( light, 0.0 );
			const float La = 1.0 / (PI * pow(length(Liu), 2.0));
			if ( light.power * La * Ls <= LIGHT_POWER_CUTOFF ) continue;

			const float cosLi = max(0.0, dot(N, Li));
			const vec3 Lr = light.color.rgb * light.power * La * Ls;
		#if LAMBERT
			const vec3 diffuse = A.rgb;
			const vec3 specular = vec3(0);
		#elif PBR
			const vec3 Lh = normalize(Li + Lo);
			const float cosLh = max(0.0, dot(N, Lh));
			
			const vec3 F = fresnelSchlick( F0, max( 0.0, dot(Lh, Lo) ) );
			const float D = ndfGGX( cosLh, R );
			const float G = gaSchlickGGX(cosLi, cosLo, R);
			const vec3 diffuse = mix( vec3(1.0) - F, vec3(0.0), M ) * A.rgb;
			const vec3 specular = (F * D * G) / max(EPSILON, 4.0 * cosLi * cosLo);
		#endif
			// lightmapped, compute only specular
			if ( light.type >= 0 && 0 <= material.indexLightmap ) fragColor.rgb += (specular) * Lr * cosLi;
			// point light, compute only diffuse
			// else if ( abs(light.type) == 1 ) fragColor.rgb += (diffuse) * Lr * cosLi;
			else fragColor.rgb += (diffuse + specular) * Lr * cosLi;
			litFactor += light.power * La * Ls;
		}
	}

	fog(rayO, rayD, fragColor, litFactor);

#if TONE_MAP
//	fragColor = fragColor / (fragColor + 1.0f);
	fragColor = vec3(1.0) - exp(-fragColor * ubo.exposure);
#endif
#if GAMMA_CORRECT
	fragColor = pow(fragColor, vec3(1.0 / ubo.gama));
#endif

/*

	if ( (ubo.mode.type & (0x1 << 0)) == (0x1 << 0) ) {
		//dither1(fragColor);
		fragColor += dither2();
	}
*/
	if ( (ubo.mode.type & (0x1 << 1)) == (0x1 << 1) ) {
		whitenoise(fragColor);
	}

	outFragColor = vec4(fragColor,1);
}