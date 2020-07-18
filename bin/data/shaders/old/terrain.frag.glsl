#version 450

layout (binding = 1) uniform sampler2D samplerColor;

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec3 inPositionEye;
layout (location = 2) in vec3 inNormalEye;
layout (location = 3) in vec4 inColor;
layout (location = 0) out vec4 outFragColor;

void fog( inout vec3 i ) {
	vec3 color = vec3( 0, 0, 0 );
	float inner = 8, outer = 32;
	float distance = length(-inPositionEye);
	float factor = (distance - inner) / (outer - inner);
	factor = clamp( factor, 0.0, 1.0 );

	i = mix(i.rgb, color, factor);
}

void phong( inout vec3 i ) {
	vec3 Ls = vec3(1.0, 1.0, 1.0); 	// light specular
	vec3 Ld = vec3(0.4, 0.4, 1.0); 	// light color
	vec3 La = vec3(0.1, 0.1, 0.1);

	vec3 Ks = vec3(0.6, 0.6, 0.9); 	// material specular
	vec3 Kd = i; 					// material diffuse
	vec3 Ka = vec3(0.7, 0.7, 0.7); 

	float Kexp = 1000.0;

	vec3 dist_light_eye = vec3(0, 0, 0) - inPositionEye;
	vec3 dir_light_eye = normalize(dist_light_eye);
	float d_dot = max(dot( dir_light_eye, inNormalEye ), 0.0);

	vec3 reflection_eye = reflect( -dir_light_eye, inNormalEye );
	vec3 surface_eye = normalize(-inPositionEye);
	float s_dot = max(dot( reflection_eye, surface_eye ), 0.0);
	float s_factor = pow( s_dot, Kexp );

	vec3 Ia = La * Ka;
	vec3 Id = Ld * Kd * d_dot;
	vec3 Is = Ls * Ks * s_factor;
	i = Is + Id + Ia;
}

void main() {
	outFragColor = texture(samplerColor, inUv);
	phong(outFragColor.rgb);
	fog(outFragColor.rgb);
//	outFragColor.rgb = mix( outFragColor.rgb, inColor.rgb, inColor.a );
	outFragColor.rgb = outFragColor.rgb * inColor.rgb;
//	outFragColor.rgb = inColor.rgb;
	if ( outFragColor.a < 0.001 ) discard;
}