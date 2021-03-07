#version 450

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
float random(vec3 seed, int i){
	vec4 seed4 = vec4(seed,i);
	float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
	return fract(sin(dot_product) * 43758.5453);
}
float shadowFactor( vec3 position, Light light, uint shadowMap, float def ) {
	vec4 positionClip = light.projection * light.view * vec4(position, 1.0);
	positionClip.xyz /= positionClip.w;

	if ( positionClip.x < -1 || positionClip.x >= 1 ) return def;
	if ( positionClip.y < -1 || positionClip.y >= 1 ) return def;
	if ( positionClip.z <= 0 || positionClip.z >= 1 ) return def;

	float factor = 1.0;

	// spot light
	if ( light.type == 2 || light.type == 3 ) {
		float dist = length( positionClip.xy );
		if ( dist > 0.5 ) return def;
		
		// spot light with attenuation
		if ( light.type == 3 ) {
			factor = 1.0 - (pow(dist * 2,2.0));
		}
	}
	
	vec2 uv = positionClip.xy * 0.5 + 0.5;
	float bias = light.depthBias;

	float eyeDepth = positionClip.z;

	int samples = 16;
	if ( samples <= 1 ) {
		return eyeDepth < texture(samplerTextures[shadowMap], uv).r - bias ? 0.0 : factor;
	}
	for ( int i = 0; i < samples; ++i ) {
		int index = int( float(samples) * random(floor(position * 1000.0), i)) % samples;
		float lightDepth = texture(samplerTextures[shadowMap], uv + poissonDisk[index] / 700.0 ).r;
		if ( eyeDepth < lightDepth - bias ) factor -= 1.0 / samples;
	}
	return factor;
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
	return 0 <= textureIndex && textureIndex < textures.length();
}
const float PI = 3.14159265359;
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
void pbr( Light light, vec3 albedo, float metallic, float roughness, vec3 lightPositionWorld, vec3 normal, vec3 position, inout vec3 i ) {
	vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic); 

	vec3 N = normalize(normal);
	vec3 L = light.position.xyz - position;
	float dist = length(L);

	L = normalize(L);
	vec3 V = normalize(-position);
	vec3 H = normalize(V + L);

	float NdotL = max(dot(N, L), 0.0);
	float NdotV = max(dot(N, V), 0.0);
	float attenuation = light.power / (dist * dist);
	vec3 radiance = light.color.rgb * attenuation;

	// cook-torrance brdf
	float NDF = DistributionGGX(N, H, roughness);        
	float G   = GeometrySmith(N, V, L, roughness);      
	vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       

	vec3 kD = vec3(1.0) - F;
	kD *= 1.0 - metallic;

	vec3 numerator    = NDF * G * F;
	float denominator = 4.0 * NdotV * NdotL;
	vec3 specular     = numerator / max(denominator, 0.001);  

	// add to outgoing radiance Lo
	i += (kD * albedo / PI + specular) * radiance * NdotL;
}

void main() {
	float mip = mipLevel(inUv.xy);
	vec2 uv = wrap(inUv.xy);
	vec4 C = vec4(1, 1, 1, 1);
	vec3 P = inPosition;
	vec3 N = inNormal;

	int materialId = int(inId.y);
	Material material = materials[materialId];
	
	float M = material.factorMetallic;
	float R = material.factorRoughness;
	float AO = material.factorOcclusion;

#if 0
	// sample albedo
	bool useAtlas = validTextureIndex( material.indexAtlas );
	Texture textureAtlas;
	if ( useAtlas ) textureAtlas = textures[material.indexAtlas];
	if ( !validTextureIndex( material.indexAlbedo ) ) discard;
	{
		Texture t = textures[material.indexAlbedo];
		C = textureLod( samplerTextures[(useAtlas) ? textureAtlas.index : t.index], (useAtlas) ? mix( t.lerp.xy, t.lerp.zw, uv ) : uv, mip );
		// alpha mode OPAQUE
		if ( material.modeAlpha == 0 ) {
			C.a = 1;
		// alpha mode BLEND
		} else if ( material.modeAlpha == 1 ) {

		// alpha mode MASK
		} else if ( material.modeAlpha == 2 ) {
			if ( C.a < abs(material.factorAlphaCutoff) ) discard;
			C.a = 1;
		}
		if ( C.a == 0 ) discard;
	}

	// sample normal
	if ( validTextureIndex( material.indexNormal ) ) {
		Texture t = textures[material.indexNormal];
		N = inTBN * normalize( textureLod( samplerTextures[(useAtlas)?textureAtlas.index:t.index], ( useAtlas ) ? mix( t.lerp.xy, t.lerp.zw, uv ) : uv, mip ).xyz * 2.0 - vec3(1.0));
	}
#endif
	C = vec4(1);
	bool lit = false;
	vec3 fragColor = vec3(0);
	for ( uint i = 0; i < lights.length(); ++i ) {
		Light light = lights[i];
		if ( light.power <= 0.001 ) continue;
		if ( 0 <= light.mapIndex ) {
			float factor = shadowFactor( P, light, light.mapIndex, 0.0 );
			light.power *= factor;
		}
		if ( light.power <= 0.0001 ) continue;
		pbr( light, C.rgb, M, R, light.position.xyz,  N, P, fragColor );
	//	if ( usePbr ) pbr( light, C.rgb, M, R, lightPositionWorld, fragColor ); else phong( light, C, fragColor );
/*
		Light light = lights[i];
		if ( validTextureIndex(light.mapIndex) ) {
			light.power *= shadowFactor( P, light, light.mapIndex, 0.0 );
		}
		if ( light.power < 0.001 ) continue;
		pbr( light, C.rgb, M, R, light.position.xyz,  N, P, fragColor );
		lit = true;
*/
	}
//	if ( !lit ) fragColor = C.rgb;
	outAlbedo = vec4(fragColor, 1);
}