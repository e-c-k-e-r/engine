#version 450
#extension GL_EXT_nonuniform_qualifier : enable

#define LAMBERT 1
#define PBR 0

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

layout (constant_id = 0) const uint TEXTURES = 1;
layout (binding = 0) uniform sampler2D samplerTextures[TEXTURES];

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

layout (std140, binding = 3) readonly buffer Materials {
	Material materials[];
};
layout (std140, binding = 4) readonly buffer Textures {
	Texture textures[];
};
layout (std140, binding = 5) readonly buffer Lights {
	Light lights[];
};

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec4 inColor;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in mat3 inTBN;
layout (location = 6) in vec3 inPosition;
layout (location = 7) flat in ivec4 inId;

layout (location = 0) out vec4 outAlbedo;

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
	const int samples = 16; //int(ubo.poissonSamples);
	if ( samples <= 1 ) return eyeDepth < texture(samplerTextures[nonuniformEXT(light.mapIndex)], uv).r - bias ? 0.0 : factor;
	for ( int i = 0; i < samples; ++i ) {
		const int index = int( float(samples) * random(floor(surface.position.world * 1000.0), i)) % samples;
		const float lightDepth = texture(samplerTextures[nonuniformEXT(light.mapIndex)], uv + poissonDisk[index] / 700.0 ).r;
		if ( eyeDepth < lightDepth - bias ) factor -= 1.0 / samples;
	}
	return factor;
}

void main() {
	vec4 A = vec4(1, 1, 1, 1);
	surface.normal.world = normalize( inNormal );
	const float mip = mipLevel(inUv.xy);
	surface.uv = wrap(inUv.xy);
	surface.position.world = inPosition;
	surface.material.id = int(inId.y);
	const Material material = materials[surface.material.id];
	
	surface.material.metallic = material.factorMetallic;
	surface.material.roughness = material.factorRoughness;
	surface.material.occlusion = 1.0f - material.factorOcclusion;

	surface.fragment = material.colorEmissive;
#if 0
	// sample albedo
	const bool useAtlas = validTextureIndex( material.indexAtlas );
	Texture textureAtlas;
	if ( useAtlas ) textureAtlas = textures[material.indexAtlas];
	if ( !validTextureIndex( material.indexAlbedo ) ) discard;
	{
		const Texture t = textures[material.indexAlbedo];
		surface.material.albedo = textureLod( samplerTextures[nonuniformEXT((useAtlas) ? textureAtlas.index : t.index)], (useAtlas) ? mix( t.lerp.xy, t.lerp.zw, uv ) : uv, mip );
		// alpha mode OPAQUE
		if ( material.modeAlpha == 0 ) {
			surface.material.albedo.a = 1;
		// alpha mode BLEND
		} else if ( material.modeAlpha == 1 ) {

		// alpha mode MASK
		} else if ( material.modeAlpha == 2 ) {
			if ( surface.material.albedo.a < abs(material.factorAlphaCutoff) ) discard;
			surface.material.albedo.a = 1;
		}
		if ( surface.material.albedo.a == 0 ) discard;
	}

	// sample normal
	if ( validTextureIndex( material.indexNormal ) ) {
		const Texture t = textures[material.indexNormal];
		surfacem.normal.world = inTBN * normalize( textureLod( samplerTextures[nonuniformEXT((useAtlas)?textureAtlas.index:t.index)], ( useAtlas ) ? mix( t.lerp.xy, t.lerp.zw, uv ) : uv, mip ).xyz * 2.0 - vec3(1.0));
	}

	// sample emissive
	if ( validTextureIndex( material.indexEmissive ) ) {
		const Texture t = textures[material.indexEmissive];
		surface.fragment = textureLod( samplerTextures[nonuniformEXT((useAtlas) ? textureAtlas.index : t.index)], (useAtlas) ? mix( t.lerp.xy, t.lerp.zw, uv ) : uv, mip );
	}
#else
	surface.material.albedo = vec4(1);
#endif

	{
		const vec3 F0 = mix(vec3(0.04), surface.material.albedo.rgb, surface.material.metallic); 
		const vec3 Lo = normalize( -surface.position.world );
		const float cosLo = max(0.0, dot(surface.normal.world, Lo));
		for ( uint i = 0; i < lights.length(); ++i ) {
			const Light light = lights[i];
			if ( light.power <= LIGHT_POWER_CUTOFF ) continue;
			const vec3 Lp = light.position;
			const vec3 Liu = light.position - surface.position.world;
			const float La = 1.0 / (PI * pow(length(Liu), 2.0));
			const float Ls = shadowFactor( light, 0.0 );
			if ( light.power * La * Ls <= LIGHT_POWER_CUTOFF ) continue;

			const vec3 Li = normalize(Liu);
			const vec3 Lr = light.color.rgb * light.power * La * Ls;
			const float cosLi = abs(dot(surface.normal.world, Li));// max(0.0, dot(N, Li));
		#if LAMBERT
			const vec3 diffuse = surface.material.albedo.rgb;
			const vec3 specular = vec3(0);
		#elif PBR
			const vec3 Lh = normalize(Li + Lo);
			const float cosLh = max(0.0, dot(N, Lh));
			
			const vec3 F = fresnelSchlick( F0, max( 0.0, dot(Lh, Lo) ) );
			const float D = ndfGGX( cosLh, surface.material.roughness );
			const float G = gaSchlickGGX(cosLi, cosLo, surface.material.roughness);
			const vec3 diffuse = mix( vec3(1.0) - F, vec3(0.0), surface.material.metallic ) * surface.material.albedo.rgb;
			const vec3 specular = (F * D * G) / max(EPSILON, 4.0 * cosLi * cosLo);
		#endif
			surface.fragment.rgb += (diffuse + specular) * Lr * cosLi;
			surface.fragment.a += light.power * La * Ls;
		}
	}

	outAlbedo = vec4(surface.fragment.rgb, 1);
}