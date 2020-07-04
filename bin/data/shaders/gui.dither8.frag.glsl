#version 450

layout (binding = 1) uniform sampler2D samplerColor;

struct Gui {
	vec4 offset;
	vec4 color;
	int mode;
	float depth;
};

layout (location = 0) in vec2 inUv;
layout (location = 1) in flat Gui inGui;
layout (location = 0) out vec4 outFragColor;

const int indexMatrix8x8[64] = int[](0,  32, 8,  40, 2,  34, 10, 42,
									 48, 16, 56, 24, 50, 18, 58, 26,
									 12, 44, 4,  36, 14, 46, 6,  38,
									 60, 28, 52, 20, 62, 30, 54, 22,
									 3,  35, 11, 43, 1,  33, 9,  41,
									 51, 19, 59, 27, 49, 17, 57, 25,
									 15, 47, 7,  39, 13, 45, 5,  37,
									 63, 31, 55, 23, 61, 29, 53, 21);

float indexValue8x8() {
	int x = int(mod(gl_FragCoord.x, 8));
	int y = int(mod(gl_FragCoord.y, 8));
	return indexMatrix8x8[(x + y * 8)] / 64.0;
}

const int paletteSize = 28;
const vec3 palette[28] = vec3[](
	vec3(0, 0, 0),
	vec3(0, 0, 1),
	vec3(0, 0, 0.502),
	vec3(0, 0, 0.753),
	vec3(0, 1, 0.251),
	vec3(0, 1, 0.50),
	vec3(60.0/360.0, 1, 0.251),
	vec3(60.0/360.0, 1, 0.5),
	vec3(120.0/360.0, 1, 0.251),
	vec3(120.0/360.0, 1, 0.5),
	vec3(180.0/360.0, 1, 0.251),
	vec3(180.0/360.0, 1, 0.5),
	vec3(240.0/360.0, 1, 0.251),
	vec3(240.0/360.0, 1, 0.5),
	vec3(300.0/360.0, 1, 0.251),
	vec3(300.0/360.0, 1, 0.5),
	vec3(60.0/360.0, 0.333, 0.376),
	vec3(60.0/360.0, 1, 0.751),
	vec3(180.0/360.0, 1, 0.125),
	vec3(150.0/360.0, 1, 0.5),
	vec3(210.0/360.0, 1, 0.5),
	vec3(180.0/360.0, 1, 0.751),
	vec3(210.0/360.0, 1, 0.251),
	vec3(240.0/360.0, 1, 0.751),
	vec3(270.0/360.0, 1, 0.5),
	vec3(330.0/360.0, 1, 0.5),
	vec3(30.0/360.0, 1, 0.251),
	vec3(24.0/360.0, 1, 0.625)
);

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

vec3[2] closestColors(float hue) {
	vec3 ret[2];
	vec3 closest = vec3(-2, 0, 0);
	vec3 secondClosest = vec3(-2, 0, 0);
	vec3 temp;
	for (int i = 0; i < paletteSize; ++i) {
		temp = palette[i];
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

const float lightnessSteps = 4.0;
float lightnessStep(float l) {
    /* Quantize the lightness to one of `lightnessSteps` values */
    return floor((0.5 + l * lightnessSteps)) / lightnessSteps;
}

vec3 dither1(vec3 color) {
    vec3 hsl = rgbToHsl(color);

    vec3 cs[2] = closestColors(hsl.x);
    vec3 c1 = cs[0];
    vec3 c2 = cs[1];
    float d = indexValue8x8();
    float hueDiff = hueDistance(hsl.x, c1.x) / hueDistance(c2.x, c1.x);

    float l1 = lightnessStep(max((hsl.z - 0.125), 0.0));
    float l2 = lightnessStep(min((hsl.z + 0.124), 1.0));
    float lightnessDiff = (hsl.z - l1) / (l2 - l1);

    vec3 resultColor = (hueDiff < d) ? c1 : c2;
    //resultColor.z = (lightnessDiff < d) ? l1 : l2;
    return hslToRgb(resultColor);
}

float dither(float color) {
	float closestColor = (color < 0.5) ? 0 : 1;
	float secondClosestColor = 1 - closestColor;
	float d = indexValue8x8();
	float distance = abs(closestColor - color);
	return (distance < d) ? closestColor : secondClosestColor;
}

vec3 dither(vec3 color) { 
	vec3 hsl = rgbToHsl(color);

	hsl.y = dither(hsl.y);
	return hslToRgb(hsl);
}

void main() {
	if ( inUv.x < inGui.offset.x ) discard;
	if ( inUv.y < inGui.offset.y ) discard;
	if ( inUv.x > inGui.offset.z ) discard;
	if ( inUv.y > inGui.offset.w ) discard;

	outFragColor = texture(samplerColor, inUv);// vec4(inUv.s, inUv.t, 1.0, 1.0);
	if ( outFragColor.a < 0.001 ) discard;
	if ( inGui.mode == 1 ) {
		outFragColor = inGui.color;
	} else {
		outFragColor *= inGui.color;
	}

	outFragColor.rgb = dither(outFragColor.rgb);
}