#version 450

layout (constant_id = 0) const uint LIGHTS = 32;

layout (input_attachment_index = 0, binding = 1) uniform subpassInput samplerAlbedo;
layout (input_attachment_index = 0, binding = 2) uniform subpassInput samplerNormal;
layout (input_attachment_index = 0, binding = 3) uniform subpassInput samplerDepth;
layout (binding = 5) uniform sampler2D samplerShadows[LIGHTS];
/*
layout (std140, binding = 6) buffer Palette {
	vec4 palette[];
};
*/
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
	vec2 padding;
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
	vec4 color;
	vec2 range;
	vec2 padding;
};

struct Mode {
	uint type;
	uint scalar;
	vec2 padding;
	vec4 parameters;
};

layout (binding = 0) uniform UBO {
	Matrices matrices;
	vec3 ambient;
	float kexp;
	Mode mode;
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
	if ( !true ) {
		float cosTheta = clamp(dot(normal.eye, normalize(light.position.xyz - position.eye)), 0, 1);
		bias = clamp(bias * tan(acos(cosTheta)), 0, 0.01);
	} else if ( true ) {
        bias = max(bias * 10 * (1.0 - dot(normal.eye, normalize(light.position.xyz - position.eye))), bias);
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
vec3 hslToRgb(vec3 HSL) {
	vec3 RGB; {
		float H = HSL.x;
		float R = abs(H * 6 - 3) - 1;
		float G = 2 - abs(H * 6 - 2);
		float B = 2 - abs(H * 6 - 4);
		RGB = clamp(vec3(R,G,B), 0, 1);
	}
	float C = (1 - abs(2 * HSL.z - 1)) * HSL.y;
	return (RGB - 0.5) * C + HSL.z;
}
vec3 rgbToHsl(vec3 RGB) {
	float Epsilon = 1e-10;
	vec3 HCV; {
		vec4 P = (RGB.g < RGB.b) ? vec4(RGB.bg, -1.0, 2.0/3.0) : vec4(RGB.gb, 0.0, -1.0/3.0);
		vec4 Q = (RGB.r < P.x) ? vec4(P.xyw, RGB.r) : vec4(RGB.r, P.yzx);
		float C = Q.x - min(Q.w, Q.y);
		float H = abs((Q.w - Q.y) / (6 * C + Epsilon) + Q.z);
		HCV = vec3(H, C, Q.x);
	}
	float L = HCV.z - HCV.y * 0.5;
	float S = HCV.y / (1 - abs(L * 2 - 1) + Epsilon);
	return vec3(HCV.x, S, L);
}

float hueDistance(float h1, float h2) {
	float diff = abs((h1 - h2));
	return min(abs((1.0 - diff)), diff);
}
const float lightnessSteps = 16.0;
float lightnessStep(float l) {
	return floor((0.5 + l * lightnessSteps)) / lightnessSteps;
}
const int indexMatrix16x16[256] = int[](0,192, 48,240, 12,204, 60,252,  3,195, 51,243, 15,207, 63,255,
									 128, 64,176,112,140, 76,188,124,131, 67,179,115,143, 79,191,127,
									  32,224, 16,208, 44,236, 28,220, 35,227, 19,211, 47,239, 31,223,
									 160, 96,144, 80,172,108,156, 92,163, 99,147, 83,175,111,159, 95,
									   8,200, 56,248,  4,196, 52,244, 11,203, 59,251,  7,199, 55,247,
									 136, 72,184,120,132, 68,180,116,139, 75,187,123,135, 71,183,119,
									  40,232, 24,216, 36,228, 20,212, 43,235, 27,219, 39,231, 23,215,
									 168,104,152, 88,164,100,148, 84,171,107,155, 91,167,103,151, 87,
									   2,194, 50,242, 14,206, 62,254,  1,193, 49,241, 13,205, 61,253,
									 130, 66,178,114,142, 78,190,126,129, 65,177,113,141, 77,189,125,
									  34,226, 18,210, 46,238, 30,222, 33,225, 17,209, 45,237, 29,221,
									 162, 98,146, 82,174,110,158, 94,161, 97,145, 81,173,109,157, 93,
									  10,202, 58,250,  6,198, 54,246,  9,201, 57,249,  5,197, 53,245,
									 138, 74,186,122,134, 70,182,118,137, 73,185,121,133, 69,181,117,
									  42,234, 26,218, 38,230, 22,214, 41,233, 25,217, 37,229, 21,213,
									 170,106,154, 90,166,102,150, 86,169,105,153, 89,165,101,149, 85);
float indexValue16x16( float scale ) {
	int x = int(mod(gl_FragCoord.x * scale, 16));
	int y = int(mod(gl_FragCoord.y * scale, 16));
	return indexMatrix16x16[(x + y * 16)] / 256.0;
}
const int indexMatrix8x8[64] = int[](0,  32, 8,  40, 2,  34, 10, 42,
									 48, 16, 56, 24, 50, 18, 58, 26,
									 12, 44, 4,  36, 14, 46, 6,  38,
									 60, 28, 52, 20, 62, 30, 54, 22,
									 3,  35, 11, 43, 1,  33, 9,  41,
									 51, 19, 59, 27, 49, 17, 57, 25,
									 15, 47, 7,  39, 13, 45, 5,  37,
									 63, 31, 55, 23, 61, 29, 53, 21);
float indexValue8x8( float scale ) {
	int x = int(mod(gl_FragCoord.x * scale, 8));
	int y = int(mod(gl_FragCoord.y * scale, 8));
	return indexMatrix8x8[(x + y * 8)] / 64.0;
}
const int indexMatrix4x4[16] = int[](0,  8,  2,  10,
									 12, 4,  14, 6,
									 3,  11, 1,  9,
									 15, 7,  13, 5);
float indexValue4x4( float scale ) {
	int x = int(mod(gl_FragCoord.x * scale, 4));
	int y = int(mod(gl_FragCoord.y * scale, 4));
	return indexMatrix4x4[(x + y * 4)] / 16.0;
}
vec3[2] closestColors(float hue) {
	vec3 ret[2];
	vec3 closest = vec3(-2, 0, 0);
	vec3 secondClosest = vec3(-2, 0, 0);
	vec3 temp;
/*
	for (int i = 0; i < palette.length(); ++i) {
		temp = rgbToHsl(palette[i].rgb);
		float tempDistance = hueDistance(temp.x, hue);
		if (tempDistance < hueDistance(closest.x, hue)) {
			secondClosest = closest;
			closest = temp;
		} else {
			if (tempDistance < hueDistance(secondClosest.x, hue)) {
				secondClosest = temp;
			}
		}
	}
*/
	ret[0] = closest;
	ret[1] = secondClosest;
	return ret;
}
float dither(float color) {
	float closestColor = (color < 0.5) ? 0 : 1;
	float secondClosestColor = 1 - closestColor;
	float d = 1; // -0.5 - fract(ubo.mode.parameters.w / 8.0);
	float scale = 1;
	if ( ubo.mode.scalar == 16 ) {
	 	d = indexValue16x16(scale);
	} else if ( ubo.mode.scalar == 8 ) {
	 	d = indexValue8x8(scale);
	} else if ( ubo.mode.scalar == 4 ) {
	 	d = indexValue4x4(scale);
	}
	float distance = abs(closestColor - color);
	return (distance < d) ? closestColor : secondClosestColor;
}
void dither(inout vec3 color) { 
	vec3 hsl = rgbToHsl(color);
	hsl.x = dither(hsl.x);
	color = hslToRgb(hsl);
}
void dither1(inout vec3 color) { 
	vec3 hsl = rgbToHsl(color);

	float d = 0;
	vec3 cs[2] = { hsl, hsl }; // closestColors(hsl.x);
	float scale = 1;
	if ( ubo.mode.scalar == 16 ) {
	 	d = indexValue16x16(scale);
	} else if ( ubo.mode.scalar == 8 ) {
	 	d = indexValue8x8(scale);
	} else if ( ubo.mode.scalar == 4 ) {
	 	d = indexValue4x4(scale);
	}
	float hueDiff = hueDistance(hsl.x, cs[0].x) / hueDistance(cs[1].x, cs[0].x);
	float l1 = lightnessStep(max((hsl.z - 0.125), 0.0));
	float l2 = lightnessStep(min((hsl.z + 0.124), 1.0));
	float lightnessDiff = (hsl.z - l1) / (l2 - l1);

	vec3 resultColor = (hueDiff < d) ? cs[0] : cs[1];
	resultColor.z = (lightnessDiff < d) ? l1 : l2;
	color = hslToRgb(resultColor);
}
void dither2(inout vec3 color) { 
	vec3 hsl = rgbToHsl(color);
	float d = 1;
	float scale = 1;
	if ( ubo.mode.scalar == 16 ) {
	 	d = indexValue16x16(scale);
	} else if ( ubo.mode.scalar == 8 ) {
	 	d = indexValue8x8(scale);
	} else if ( ubo.mode.scalar == 4 ) {
	 	d = indexValue4x4(scale);
	}
	hsl.z *= d;
/*
	float l1 = lightnessStep(max((hsl.z - 0.125), 0.0));
	float l2 = lightnessStep(min((hsl.z + 0.124), 1.0));
	float lightnessDiff = (hsl.z - l1) / (l2 - l1);
	hsl.z = (lightnessDiff < d) ? l1 : l2;
*/
	color = hslToRgb(hsl);
}
vec3 dither3() {
    vec3 vDither = dot( vec2( 171.0, 231.0 ), inUv.xy + ubo.mode.parameters.w ).xxx;
    vDither.rgb = fract( vDither.rgb / vec3( 103.0, 71.0, 97.0 ) ) - vec3( 0.5, 0.5, 0.5 );
    return ( vDither.rgb / 255.0 ) * 0.375;
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

void main() {
	vec4 albedoSpecular = subpassLoad(samplerAlbedo);
	vec3 fragColor = albedoSpecular.rgb * ubo.ambient.rgb;

	normal.eye = subpassLoad(samplerNormal).rgb;
	{
		mat4 iProj = inverse( ubo.matrices.projection[inPushConstantPass] );
		mat4 iView = inverse( ubo.matrices.view[inPushConstantPass] );

		float depth = subpassLoad(samplerDepth).r;		
		vec4 positionClip = vec4(inUv * 2.0 - 1.0, depth, 1.0);
		vec4 positionEye = iProj * positionClip;
		positionEye /= positionEye.w;
		position.eye = positionEye.xyz;

		vec4 positionWorld = iView * positionEye;
		position.world = positionWorld.xyz;
	}
	float litFactor = 1.0;
	for ( uint i = 0, shadowMap = 0; i < LIGHTS; ++i ) {
		Light light = ubo.lights[i];
		
		if ( light.power <= 0.001 ) continue;
		
		light.position.xyz = vec3(ubo.matrices.view[inPushConstantPass] * vec4(light.position.xyz, 1));
		if ( light.type > 0 ) {
			float shadowFactor = shadowFactor( light, shadowMap++ );
			if ( shadowFactor <= 0.0001 ) continue;
			light.power *= shadowFactor;
			litFactor += light.power;
		}
		phong( light, albedoSpecular, fragColor );
	}
	
	fog(fragColor, litFactor);

	if ( (ubo.mode.type & (0x1 << 0)) == (0x1 << 0) ) {
	//	dither2(fragColor);
        fragColor += dither3();
	}
	
	if ( (ubo.mode.type & (0x1 << 1)) == (0x1 << 1) ) {
		whitenoise(fragColor);
	}

	outFragColor = vec4(fragColor,1);
}