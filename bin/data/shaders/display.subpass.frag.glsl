#version 450

layout (constant_id = 0) const uint LIGHTS = 32;

layout (input_attachment_index = 0, binding = 1) uniform subpassInput samplerAlbedo;
layout (input_attachment_index = 0, binding = 2) uniform subpassInput samplerNormal;
layout (input_attachment_index = 0, binding = 3) uniform subpassInput samplerPosition;
// layout (input_attachment_index = 0, binding = 4) uniform subpassInput samplerDepth;
layout (binding = 5) uniform sampler2D samplerShadows[LIGHTS];

layout (location = 0) in vec2 inUv;
layout (location = 1) in flat uint inPushConstantPass;

layout (location = 0) out vec4 outFragColor;

/*
const vec2 poissonDisk[4] = vec2[](
	vec2( -0.94201624, -0.39906216 ),
	vec2( 0.94558609, -0.76890725 ),
	vec2( -0.094184101, -0.92938870 ),
	vec2( 0.34495938, 0.29387760 )
);
*/

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

struct Light {
	vec3 position;
	float radius;
	vec3 color;
	float power;
	int type;
	float depthBias;
	mat4 view;
	mat4 projection;
};

struct Matrices {
	mat4 view[2];
	mat4 projection[2];
};

struct Space {
	vec3 eye;
	vec3 world;
} position, normal, view;

struct Fog {
	vec2 range;
	vec4 color;
};

layout (binding = 0) uniform UBO {
	Matrices matrices;
	vec3 ambient;
	float kexp;
	Fog fog;
	Light lights[LIGHTS];
} ubo;

void fog( inout vec3 i, float scale ) {
	if ( ubo.fog.range.x == 0 || ubo.fog.range.y == 0 ) return;

	vec3 color = ubo.fog.color.rgb;
	float inner = ubo.fog.range.x;
	float outer = ubo.fog.range.y * scale;
	float distance = length(-position.eye);
	float factor = (distance - inner) / (outer - inner);
	factor = clamp( factor, 0.0, 1.0 );

	i = mix(i.rgb, color, factor);
}

void phong( Light light, vec4 albedoSpecular, inout vec3 i ) {
	vec3 Ls = vec3(1.0, 1.0, 1.0); 		// light specular
	vec3 Ld = light.color; 				// light color
	vec3 La = vec3(1.0, 1.0, 1.0);

	vec3 Ks = vec3(albedoSpecular.a); 	// material specular
	vec3 Kd = albedoSpecular.rgb;		// material diffuse
	vec3 Ka = vec3(1.0, 1.0, 1.0); 

	float Kexp = ubo.kexp;

	vec3 L = light.position.xyz - position.eye;
	float dist = length(L);
	
	if ( light.radius > 0.001 && light.radius < dist ) return;

	vec3 D = normalize(L);
	float d_dot = max(dot( D, normal.eye ), 0.0);

	vec3 R = reflect( -D, normal.eye );
	vec3 S = normalize(-position.eye);
	float s_factor = pow( max(dot( R, S ), 0.0), Kexp );
	if ( Kexp < 0.0001 ) s_factor = 0;
	
	float attenuation = 1;
	if ( light.radius > 0.0001 )
		attenuation = clamp( light.radius / (pow(dist, 2.0) + 1.0), 0.0, 1.0 );

	vec3 Ia = La * Ka;
	vec3 Id = Ld * Kd * d_dot * attenuation;
	vec3 Is = Ls * Ks * s_factor * attenuation;
	
	i += Id * light.power + Is;
}

float random(vec3 seed, int i){
	vec4 seed4 = vec4(seed,i);
	float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
	return fract(sin(dot_product) * 43758.5453);
}

float shadowFactor( Light light, uint shadowMap ) {
	vec4 positionClip = light.projection * light.view * vec4(position.world, 1.0);
	positionClip.xyz /= positionClip.w;

	if ( positionClip.x < -1 || positionClip.x >= 1 ) return 0.0;
	if ( positionClip.y < -1 || positionClip.y >= 1 ) return 0.0;
	if ( positionClip.z <= 0 || positionClip.z >= 1 ) return 0.0;

	float factor = 1.0;

	// spot light
	if ( light.type == 2 || light.type == 3 ) {
		float dist = length( positionClip.xy );
		if ( dist > 0.5 ) return 0.0;
		
		// spot light with attenuation
		if ( light.type == 3 ) {
			factor = 1.0 - (pow(dist * 2,2.0));
		}
	}
	
	vec2 uv = positionClip.xy * 0.5 + 0.5;
	float bias = light.depthBias;
	if ( false ) {
		float cosTheta = clamp(dot(normal.eye, normalize(light.position.xyz - position.eye)), 0, 1);
		bias = clamp(bias * tan(acos(cosTheta)), 0, 0.01);
	}

	float eyeDepth = positionClip.z;
	int samples = poissonDisk.length();
	if ( samples <= 1 ) {
		return eyeDepth < texture(samplerShadows[shadowMap], uv).r - bias ? 0.0 : factor;
	}
	for ( int i = 0; i < samples; ++i ) {
	//	int index = i;
	//	int index = int( float(samples) * random(gl_FragCoord.xyy, i) ) % samples;
		int index = int( float(samples) * random(floor(position.world.xyz * 1000.0), i)) % samples;
		float lightDepth = texture(samplerShadows[shadowMap], uv + poissonDisk[index] / 700.0 ).r;
		if ( eyeDepth < lightDepth - bias ) factor -= 1.0 / samples;
	}
	return factor;
}

void main() {
	vec4 albedoSpecular = subpassLoad(samplerAlbedo);
	normal.eye = subpassLoad(samplerNormal).rgb;
	position.eye = subpassLoad(samplerPosition).rgb;

	{
		mat4 iView = inverse( ubo.matrices.view[inPushConstantPass] );
		vec4 positionWorld = iView * vec4(position.eye, 1);
		position.world = positionWorld.xyz;
	}
/*
	{
		for ( int i = 0; i < LIGHTS; ++i ) {
			outFragColor.rgb = texture(samplerShadows[i], inUv).rgb;
		}
		float depth = texture(samplerShadows[3], inUv).r;
		depth /= 0.00526;
		outFragColor.rgb = vec3(1 - depth);
	//	outFragColor.rgb = texture(samplerShadows[0], inUv).rgb;
		outFragColor.a = 1;
		return;
	}
*/
/*
	{
		mat4 iProj = inverse( ubo.matrices.projection[inPushConstantPass] );
		mat4 iView = inverse( ubo.matrices.view[inPushConstantPass] );

		float depth = subpassLoad(samplerDepth).r;

		if ( false ) {
			depth /= 0.00526;
			outFragColor.rgb = vec3( 1 - depth );
			outFragColor.a = 1;
			return;
		}
		
		vec4 positionClip = vec4(inUv * 2.0 - 1.0, depth, 1.0);
		vec4 positionEye = iProj * positionClip;
		positionEye /= positionEye.w;
		position.eye = positionEye.xyz;

		vec4 positionWorld = iView * positionEye;
		position.world = positionWorld.xyz;
	}
*/

	vec3 fragColor = albedoSpecular.rgb * ubo.ambient.rgb;
	bool lit = false;
	uint shadowMap = 0;
	float litFactor = 1.0;
	for ( uint i = 0; i < LIGHTS; ++i ) {
		Light light = ubo.lights[i];
		
		if ( light.power <= 0.001 ) continue;
		lit = true;
		
		light.position.xyz = vec3(ubo.matrices.view[inPushConstantPass] * vec4(light.position.xyz, 1));
		if ( light.type > 0 ) {
			float shadowFactor = shadowFactor( light, shadowMap++ );
			if ( shadowFactor <= 0.0001 ) continue;
			light.power *= shadowFactor;
			litFactor += light.power;
		}
		phong( light, albedoSpecular, fragColor );
	}
	// if ( !lit ) fragColor = albedoSpecular.rgb;
	fog(fragColor, litFactor);
	// if ( length(position.eye) < 0.01 ) fragColor = vec3(1,0,1);
	outFragColor = vec4(fragColor,1);
}