#version 450

layout (binding = 1) uniform sampler2D samplerColor;

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec3 inPositionEye;
layout (location = 2) in vec3 inNormalEye;
layout (location = 3) in vec4 inColor;
layout (location = 0) out vec4 outFragColor;

void fog( inout vec3 i ) {
	vec3 color = vec3( 0, 0, 0 );
	float inner = 8, outer = 64;
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

float indexValue16x16() {
	int x = int(mod(gl_FragCoord.x, 16));
	int y = int(mod(gl_FragCoord.y, 16));
	return indexMatrix16x16[(x + y * 16)] / 256.0;
}
/*
const int paletteSize = 16;
const vec3 palette[paletteSize] = vec3[](
	vec3( 0x00/255.0, 0x00/255.0, 0x18/255.0),
	vec3( 0x60/255.0, 0x60/255.0, 0x78/255.0),
	vec3( 0x00/255.0, 0x18/255.0, 0x30/255.0),
	vec3( 0x18/255.0, 0x18/255.0, 0x30/255.0),
	vec3( 0x30/255.0, 0x30/255.0, 0x48/255.0),
	vec3( 0x30/255.0, 0x30/255.0, 0x30/255.0),
	vec3( 0x18/255.0, 0x18/255.0, 0x48/255.0),
	vec3( 0x00/255.0, 0x00/255.0, 0x00/255.0),
	vec3( 0x18/255.0, 0x00/255.0, 0x00/255.0),
	vec3( 0x48/255.0, 0x48/255.0, 0x60/255.0),
	vec3( 0x30/255.0, 0x00/255.0, 0x18/255.0),
	vec3( 0x00/255.0, 0x18/255.0, 0x48/255.0),
	vec3( 0x90/255.0, 0x90/255.0, 0xa8/255.0),
	vec3( 0x48/255.0, 0x18/255.0, 0x30/255.0),
	vec3( 0x18/255.0, 0x30/255.0, 0x48/255.0),
	vec3( 0x18/255.0, 0x00/255.0, 0x18/255.0)
);
*/

const int paletteSize = 40;
const vec3 palette[paletteSize] = vec3[](
	vec3( 0, 0, 0),
	vec3( 0, 0, 0.502),
	vec3( 0, 0, 0.753),
	vec3( 0, 0, 1),
	vec3( 0, 1, 0.251),
	vec3( 0, 1, 0.50),
	vec3( 24.0/360.0, 1, 0.251),
	vec3( 24.0/360.0, 1, 0.5),
	vec3( 24.0/360.0, 1, 0.625),
	vec3( 24.0/360.0, 1, 0.751),
	vec3( 30.0/360.0, 1, 0.251),
	vec3( 60.0/360.0, 0.333, 0.376),
	vec3( 60.0/360.0, 1, 0.251),
	vec3( 60.0/360.0, 1, 0.5),
	vec3( 60.0/360.0, 1, 0.751),
	vec3(120.0/360.0, 1, 0.251),
	vec3(120.0/360.0, 1, 0.5),
	vec3(120.0/360.0, 1, 0.751),
	vec3(150.0/360.0, 1, 0.251),
	vec3(150.0/360.0, 1, 0.5),
	vec3(150.0/360.0, 1, 0.751),
	vec3(180.0/360.0, 1, 0.125),
	vec3(180.0/360.0, 1, 0.251),
	vec3(180.0/360.0, 1, 0.5),
	vec3(180.0/360.0, 1, 0.751),
	vec3(210.0/360.0, 1, 0.251),
	vec3(210.0/360.0, 1, 0.5),
	vec3(210.0/360.0, 1, 0.751),
	vec3(240.0/360.0, 1, 0.251),
	vec3(240.0/360.0, 1, 0.5),
	vec3(240.0/360.0, 1, 0.751),
	vec3(270.0/360.0, 1, 0.251),
	vec3(270.0/360.0, 1, 0.5),
	vec3(270.0/360.0, 1, 0.751),
	vec3(300.0/360.0, 1, 0.251),
	vec3(300.0/360.0, 1, 0.5),
	vec3(300.0/360.0, 1, 0.751),
	vec3(330.0/360.0, 1, 0.251),
	vec3(330.0/360.0, 1, 0.5),
	vec3(330.0/360.0, 1, 0.751)
);

/*
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
*/

float hueToRgb(float f1, float f2, float hue) {
	if (hue < 0.0)
		hue += 1.0;
	else if (hue > 1.0)
		hue -= 1.0;
	float res;
	if ((6.0 * hue) < 1.0)
		res = f1 + (f2 - f1) * 6.0 * hue;
	else if ((2.0 * hue) < 1.0)
		res = f2;
	else if ((3.0 * hue) < 2.0)
		res = f1 + (f2 - f1) * ((2.0 / 3.0) - hue) * 6.0;
	else
		res = f1;
	return res;
}

vec3 hslToRgb(vec3 hsl) {
	vec3 rgb;
	
	if (hsl.y == 0.0) {
		rgb = vec3(hsl.z); // Luminance
	} else {
		float f2;
		
		if (hsl.z < 0.5)
			f2 = hsl.z * (1.0 + hsl.y);
		else
			f2 = hsl.z + hsl.y - hsl.y * hsl.z;
			
		float f1 = 2.0 * hsl.z - f2;
		
		rgb.r = hueToRgb(f1, f2, hsl.x + (1.0/3.0));
		rgb.g = hueToRgb(f1, f2, hsl.x);
		rgb.b = hueToRgb(f1, f2, hsl.x - (1.0/3.0));
	}   
	return rgb;
}

vec3 rgbToHcv(vec3 rgb) {
	float epsilon = 1e-10;
    // Based on work by Sam Hocevar and Emil Persson
    vec4 p = (rgb.g < rgb.b) ? vec4(rgb.bg, -1.0, 2.0 / 3.0) : vec4(rgb.gb, 0.0, -1.0 / 3.0);
    vec4 q = (rgb.r < p.x) ? vec4(p.xyw, rgb.r) : vec4(rgb.r, p.yzx);
    float c = q.x - min(q.w, q.y);
    float h = abs((q.w - q.y) / (6.0 * c + epsilon) + q.z);
    return vec3(h, c, q.x);
}

vec3 rgbToHsl(vec3 rgb) {
	float epsilon = 1e-10;
    vec3 hcv = rgbToHcv(rgb);
    float l = hcv.z - hcv.y * 0.5;
    float s = hcv.y / (1.0 - abs(l * 2.0 - 1.0) + epsilon);
    return vec3(hcv.x, s, l);
}


float hueDistance(float h1, float h2) {
	float diff = abs((h1 - h2));
	return min(abs((1.0 - diff)), diff);
}

vec3[2] closestColors(float hue) {
	vec3 ret[2];
	vec3 closest = vec3(-2, 0, 0);
	vec3 secondClosest = vec3(-2, 0, 0);
	vec3 temp;
	for (int i = 0; i < paletteSize; ++i) {
		temp = rgbToHsl(palette[i]);
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
	ret[0] = closest;
	ret[1] = secondClosest;
	return ret;
}

const float lightnessSteps = 8.0;
float lightnessStep(float l) {
	/* Quantize the lightness to one of `lightnessSteps` values */
	return floor((0.5 + l * lightnessSteps)) / lightnessSteps;
}

vec3 dither1(vec3 color) {
	vec3 hsl = rgbToHsl(color);

	vec3 cs[2] = closestColors(hsl.x);
	vec3 c1 = cs[0];
	vec3 c2 = cs[1];
	float d = indexValue16x16();
	float hueDiff = hueDistance(hsl.x, c1.x) / hueDistance(c2.x, c1.x);

	float l1 = lightnessStep(max((hsl.z - 0.125), 0.0));
	float l2 = lightnessStep(min((hsl.z + 0.124), 1.0));
	float lightnessDiff = (hsl.z - l1) / (l2 - l1);

	vec3 resultColor = (hueDiff < d) ? c1 : c2;
	resultColor.z = (lightnessDiff < d) ? l1 : l2;
	return hslToRgb(resultColor);
}

float dither(float color) {
	float closestColor = (color < 0.5) ? 0 : 1;
	float secondClosestColor = 1 - closestColor;
	float d = indexValue16x16();
	float distance = abs(closestColor - color);
	return (distance < d) ? closestColor : secondClosestColor;
}

vec3 dither(vec3 color) { 
	vec3 hsl = rgbToHsl(color);

	hsl.y = dither(hsl.y);
	return hslToRgb(hsl);
}

void main() {
	outFragColor = texture(samplerColor, inUv);
	phong(outFragColor.rgb);
	fog(outFragColor.rgb);
	outFragColor.rgb = mix( outFragColor.rgb, inColor.rgb, inColor.a );
	if ( outFragColor.a < 0.001 ) discard;
	outFragColor.rgb = dither(outFragColor.rgb);
}