#version 450

#define BASE_LIGHTS_SIZE 16
layout (constant_id = 0) const uint LIGHTS = BASE_LIGHTS_SIZE;

layout (input_attachment_index = 0, binding = 1) uniform subpassInput samplerAlbedo;
layout (input_attachment_index = 0, binding = 2) uniform subpassInput samplerNormal;
layout (input_attachment_index = 0, binding = 3) uniform subpassInput samplerPosition;
// layout (input_attachment_index = 0, binding = 4) uniform subpassInput samplerDepth;
layout (binding = 5) uniform sampler2D samplerShadows[LIGHTS];

layout (location = 0) in vec2 inUv;
layout (location = 1) in flat uint inPushConstantPass;

layout (location = 0) out vec4 outFragColor;


struct Light {
	vec3 position;
	float power;
	vec3 color;
	float radius;
	int type;
	int shadowed;
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

layout (binding = 0) uniform UBO {
	Matrices matrices;
	vec3 ambient;
	float kexp;
	Light lights[LIGHTS];
} ubo;

void fog( inout vec3 i ) {
	vec3 color = vec3( 0, 0, 0 );
	float inner = 8, outer = 64;
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
	
	if ( light.radius < dist ) return;

	vec3 D = normalize(L);
	float d_dot = max(dot( D, normal.eye ), 0.0);

	vec3 R = reflect( -D, normal.eye );
	vec3 S = normalize(-position.eye);
	float s_factor = pow( max(dot( R, S ), 0.0), Kexp );
	
	float attenuation = light.radius / (pow(dist, 2.0) + 1.0);

	vec3 Ia = La * Ka;
	vec3 Id = Ld * Kd * d_dot * attenuation;
	vec3 Is = Ls * Ks * s_factor * attenuation;
	
	i += Id * light.power;
}

bool debugShadow( Light light, uint i ) {
//	return false;

	{
		outFragColor.rgb = texture(samplerShadows[i], inUv).rgb;
		outFragColor.a = 1;
		return true;
	}

	vec4 positionClip = light.projection * light.view * vec4(position.world, 1.0);
	positionClip.xyz /= positionClip.w;
	float lightDepth = texture(samplerShadows[i], positionClip.xy * 0.5 + 0.5).r;

	float eyeDepth = positionClip.z;
//	eyeDepth = lightDepth;
	float bias = 0.0005;

		 if ( positionClip.x < -1 || positionClip.x >= 1 ) eyeDepth = 1;
	else if ( positionClip.y < -1 || positionClip.y >= 1 ) eyeDepth = 1;
	else if ( positionClip.z <  0 || positionClip.z >= 1 ) eyeDepth = 1;
	else eyeDepth /= 0.00526;

	outFragColor.rgb = vec3( 1 - eyeDepth );
	outFragColor.a = 1;
	
	return true;
}

float shadowFactor( Light light, uint i ) {
	vec4 positionClip = light.projection * light.view * vec4(position.world, 1.0);
	positionClip.xyz /= positionClip.w;
	float lightDepth = texture(samplerShadows[(i*4)+3], positionClip.xy * 0.5 + 0.5).r;
	float eyeDepth = positionClip.z;
	float bias = 0.0005;
	// bias = max(0.05 * (1.0 - dot(normal.eye, light.position.xyz - position.eye)), bias);

//	eyeDepth /= 0.00526;
//	lightDepth /= 0.00526;

	eyeDepth = 1 - eyeDepth;
	lightDepth = 1 - lightDepth;

	if ( positionClip.x < -1 || positionClip.x >= 1 ) return 0.0;
	if ( positionClip.y < -1 || positionClip.y >= 1 ) return 0.0;
	if ( positionClip.z <  0 || positionClip.z >= 1 ) return 0.0;

	return eyeDepth - bias < lightDepth ? 1.0 : 0.0;
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

	for ( uint i = 0; i < LIGHTS; ++i ) {
		Light light = ubo.lights[i];
		
		if ( light.power <= 0.001 ) continue;
		lit = true;
		
		light.position.xyz = vec3(ubo.matrices.view[inPushConstantPass] * vec4(light.position.xyz, 1));
		if ( light.shadowed > 0 ) {
			float shadowFactor = shadowFactor( light, i );
			if ( shadowFactor <= 0.0001 ) continue;
		}
		phong( light, albedoSpecular, fragColor );
	}
	
	if ( !lit ) fragColor = albedoSpecular.rgb;
	
	fog(fragColor);

	outFragColor = vec4(fragColor,1);
/*
	if ( !false ) {
		float depth = texture(samplerShadows[0], inUv).r;
		depth /= 0.00526;
		outFragColor = vec4( vec3(1 - depth), 1 );
		return;
	} else if ( !false ) {
		outFragColor = vec4( texture(samplerShadows[0], inUv).rgb, 1 );
		return;
	} else {
		outFragColor = vec4( inUv.x, 0, inUv.y, 1 );
		return;
	}
*/
}