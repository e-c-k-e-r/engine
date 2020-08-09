#version 450

layout (input_attachment_index = 0, binding = 1) uniform subpassInput samplerAlbedo;
layout (input_attachment_index = 0, binding = 2) uniform subpassInput samplerPosition;
layout (input_attachment_index = 0, binding = 3) uniform subpassInput samplerNormal;

layout (location = 0) in vec2 inUv;
layout (location = 1) in flat uint inPushConstantPass;

layout (location = 0) out vec4 outFragColor;

layout (constant_id = 0) const uint LIGHTS = 32;

struct Light {
	vec3 position;
	float power;
	vec3 color;
	float radius;
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

void main() {
	position.eye = subpassLoad(samplerPosition).rgb;
	normal.eye = subpassLoad(samplerNormal).rgb;
	vec4 albedoSpecular = subpassLoad(samplerAlbedo);

	vec3 fragColor = albedoSpecular.rgb * ubo.ambient.rgb;
	bool lit = false;
	for ( uint i = 0; i < LIGHTS; ++i ) {
		Light light = ubo.lights[i];
		if ( light.power <= 0.001 ) continue;
		lit = true;
		light.position.xyz = vec3(ubo.matrices.view[inPushConstantPass] * vec4(light.position.xyz, 1));
		phong( light, albedoSpecular, fragColor );
	}
	if ( !lit ) fragColor = albedoSpecular.rgb;
	fog(fragColor);
	outFragColor = vec4(fragColor,1);
}