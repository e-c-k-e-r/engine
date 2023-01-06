#version 450
#pragma shader_stage(fragment)
#extension GL_EXT_samplerless_texture_functions : require

layout (location = 0) in vec2 inUv;
layout (location = 1) flat in uint inPass;

layout (location = 0) out vec4 outColor;

layout (binding = 0) uniform sampler2D	samplerColor;

layout (binding = 1) uniform UBO {
	float curTime;
	float gamma;
	float exposure;
	uint padding;
} ubo;

#define TONE_MAP 1
#define GAMMA_CORRECT 1
#define TEXTURES 1

#include "../../common/macros.h"
#include "../../common/structs.h"
#include "../../common/functions.h"

#define M_PI 3.1415926535897932384626433832795

const float u_imgx = 0;
const float u_imgy = 0;
const float u_imgw = 1;
const float u_imgh = 1;
const float u_img_gain = 2;
const float u_img_bias = 0;
const float u_beam_bias = 0.185;
const float u_beam_gain = 0.25;
const float u_corner = 0.05;
const float u_zoom = 1.0;
const float u_shape = 2;
const float u_round = -0.02;
const float u_grain = 0.4;
const float u_vpitch = 936.1;
const float u_hpitch = 1024.6;
const float u_top = 1;
const float u_bot = 1;

void main() {
	const vec2 screenResolution = textureSize( samplerColor, 0 );
	const float u_lines = screenResolution.y * 0.75;

	vec2 uv_orig = (inUv.xy * 2.0 - 1.0);
	vec2 uv_mod = uv_orig * pow(1.0-abs(uv_orig),vec2(u_round)) * (u_zoom + u_corner * pow( abs(uv_orig.yx), vec2(u_shape)) );
	vec2 uv = uv_mod / 2.0 + 0.5;

	if ( abs(uv_mod).x > 1.0 || abs(uv_mod.y) > 1.0 ) {
		outColor = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}

	float spacing = 1.0/u_lines;
	uv+=spacing*0.5;

	float line_top = ( ceil(uv.y*u_lines)) / u_lines;
	float line_bot = line_top - spacing;

	vec2 scale = vec2(u_imgw, u_imgh);
	vec2 offset = vec2(u_imgx, u_imgy);

	vec2 uv_top = (vec2(uv.x, line_top)+offset) * scale - (scale-1.0)*0.5 ;
	vec2 uv_bot = (vec2(uv.x, line_bot)+offset) * scale - (scale-1.0)*0.5 ;

	uv_top -= spacing * 0.5;
	uv_bot -= spacing * 0.5;

	vec4 sampled_top = texture(samplerColor, uv_top);
	vec4 sampled_bot = texture(samplerColor, uv_bot);

	vec3 color_top = sampled_top.xyz * u_img_gain + u_img_bias;
	vec3 color_bot = sampled_bot.xyz * u_img_gain + u_img_bias;

	float dist_top = pow(abs(uv.y - line_top), 1.0);
	float dist_bot = pow(abs(uv.y - line_bot), 1.0);

	vec3 beam_top = 1.0 - (dist_top / (spacing * (color_top * u_beam_gain + u_beam_bias)));
	vec3 beam_bot = 1.0 - (dist_bot / (spacing * (color_bot * u_beam_gain + u_beam_bias)));
	
	beam_top = clamp(beam_top, 0.0, 1.0) ;
	beam_bot = clamp(beam_bot, 0.0, 1.0) ;

	vec3 color = (color_top*beam_top)*u_top + (color_bot*beam_bot)*u_bot;

	vec2 dot;
	dot.y = floor(uv.y * u_vpitch) ;
	dot.x = uv.x;

	if (mod(dot.y, 2.0) > 0.5)
		dot.x += (4.5/3.0) / u_hpitch;

	dot.x = (floor(dot.x*u_hpitch)) ;

	int fil = int(mod(dot.x, 3.0));
	
	vec3 out_color = color * (1.0-u_grain);
	
	vec3 passthru = vec3( 
		float(fil == 0), 
		float(fil == 1),
		float(fil == 2)
	) * (u_grain);

	out_color += color * passthru;

	outColor = vec4(out_color, (sampled_top.a + sampled_bot.a) * 0.5 );

#if TONE_MAP
	toneMap(outColor, ubo.exposure);
#endif
#if GAMMA_CORRECT
	gammaCorrect(outColor, ubo.gamma);
#endif
}

#if 0
float iTime = 0;
float noise(vec2 p) {
	float s = (fract(sin(dot(p * sin( iTime * 0.5 ), vec2(12.9898,78.233)*2.0)) * 43758.5453)); // texture(iChannel1,vec2(1.,2.*cos(iTime))*iTime*8. + p*1.).x;
	s *= s;
	return s;
}

float onOff(float a, float b, float c) {
	return step(c, sin(iTime + a*cos(iTime*b)));
}

float ramp(float y, float start, float end) {
	float inside = step(start,y) - step(end,y);
	float fact = (y-start)/(end-start)*inside;
	return (1.-fact) * inside;
	
}

float stripes(vec2 uv) {
	float noi = noise(uv*vec2(0.5,1.) + vec2(1.,3.));
	return ramp(mod(uv.y*4. + iTime/2.+sin(iTime + sin(iTime*0.63)),1.),0.5,0.6)*noi;
}

vec4 getVideo(vec2 uv) {
	vec2 look = uv;
	float window = 1./(1.+20.*(look.y-mod(iTime/4.,1.))*(look.y-mod(iTime/4.,1.)));
	look.x = look.x + sin(look.y*10. + iTime)/50.*onOff(4.,4.,.3)*(1.+cos(iTime*80.))*window;
	float vShift = 0.4*onOff(2.,3.,.9)*(sin(iTime)*sin(iTime*20.) + 
										 (0.5 + 0.1*sin(iTime*200.)*cos(iTime)));
	look.y = mod(look.y + vShift, 1.);
	return texture(samplerColor,look);
}

vec2 screenDistort(vec2 uv) {
	uv -= vec2(.5,.5);
	uv = uv*1.2*(1./1.2+2.*uv.x*uv.x*uv.y*uv.y);
	uv += vec2(.5,.5);
	return uv;
}

void main() {
	vec2 uv = inUv.xy; // fragCoord.xy / iResolution.xy;
	iTime = ubo.curTime;

	uv = screenDistort(uv);
	vec4 video = getVideo(uv);
	float vigAmt = 3.+.3*sin(iTime + 5.*cos(iTime*5.));
	float vignette = (1.-vigAmt*(uv.y-.5)*(uv.y-.5))*(1.-vigAmt*(uv.x-.5)*(uv.x-.5));
	
	video.rgb += stripes(uv);
	video.rgb += noise(uv*2.)/4.;
	video.rgb *= vignette;
	video.rgb *= (12.+mod(uv.y*30.+iTime,1.))/13.;
	
	outColor = video;
#if TONE_MAP
	toneMap(outColor, ubo.exposure);
#endif
#if GAMMA_CORRECT
	gammaCorrect(outColor, ubo.gamma);
#endif
}
#endif
#if 0
vec2 curveRemapUV(vec2 uv, vec2 curvature) {
	uv = uv * 2.0 - 1.0;
	vec2 offset = abs(uv.yx) / vec2(curvature.x, curvature.y);
	uv = uv + uv * offset * offset;
	uv = uv * 0.5 + 0.5;
	return uv;
}

vec4 scanLineIntensity(float uv, float resolution, float opacity) {
	float intensity = sin(uv * resolution * PI * 2.0);
	intensity = ((0.5 * intensity) + 0.5) * 0.9 + 0.1;
	return vec4(vec3(pow(intensity, opacity)), 1.0);
}

vec4 vignetteIntensity(vec2 uv, vec2 resolution, float opacity, float roundness) {
	float intensity = uv.x * uv.y * (1.0 - uv.x) * (1.0 - uv.y);
	return vec4(vec3(clamp(pow((resolution.x / roundness) * intensity, opacity), 0.0, 1.0)), 1.0);
}

void main(void) {
	const vec2 screenResolution = textureSize( samplerColor, 0 );
	const vec2 scanLineOpacity = vec2(0.5);
	const float brightness = 2;
	const float vignetteOpacity = 0.5;
	const float vignetteRoundness = 0.5;
	const vec2 curvature = vec2(8);

	const vec2 remappedUV = curveRemapUV(vec2(inUv.x, inUv.y), curvature);
	vec4 baseColor = texture(samplerColor, remappedUV);

	baseColor *= vignetteIntensity(remappedUV, screenResolution, vignetteOpacity, vignetteRoundness);

	baseColor *= scanLineIntensity(remappedUV.x, screenResolution.y, scanLineOpacity.x);
	baseColor *= scanLineIntensity(remappedUV.y, screenResolution.x, scanLineOpacity.y);

	baseColor *= vec4(vec3(brightness), 1.0);

	if (remappedUV.x < 0.0 || remappedUV.y < 0.0 || remappedUV.x > 1.0 || remappedUV.y > 1.0){
		outColor = vec4(0.0, 0.0, 0.0, 1.0);
	} else {
		outColor = baseColor;
	}

#if TONE_MAP
	toneMap(outColor, ubo.exposure);
#endif
#if GAMMA_CORRECT
	gammaCorrect(outColor, ubo.gamma);
#endif
}
#endif
#if 0
vec2 fragCoord = vec2(0,0);
vec2 iResolution = vec2(640,480);

// Emulated input resolution.
#if 0
	// Fix resolution to set amount.
	#define res (vec2(320.0/1.0,160.0/1.0))
#else
	// Optimize for resize.
	#define res (iResolution.xy/6.0)
#endif

// Hardness of scanline.
//	-8.0 = soft
// -16.0 = medium
float hardScan=-8.0;

// Hardness of pixels in scanline.
// -2.0 = soft
// -4.0 = hard
float hardPix=-3.0;

// Display warp.
// 0.0 = none
// 1.0/8.0 = extreme
vec2 warp=vec2(1.0/32.0,1.0/24.0); 

// Amount of shadow mask.
float maskDark=0.5;
float maskLight=1.5;

//------------------------------------------------------------------------

// sRGB to Linear.
// Assuing using sRGB typed textures this should not be needed.
float ToLinear1(float c){return(c<=0.04045)?c/12.92:pow((c+0.055)/1.055,2.4);}
vec3 ToLinear(vec3 c){return vec3(ToLinear1(c.r),ToLinear1(c.g),ToLinear1(c.b));}

// Linear to sRGB.
// Assuing using sRGB typed textures this should not be needed.
float ToSrgb1(float c){return(c<0.0031308?c*12.92:1.055*pow(c,0.41666)-0.055);}
vec3 ToSrgb(vec3 c){return vec3(ToSrgb1(c.r),ToSrgb1(c.g),ToSrgb1(c.b));}

// Nearest emulated sample given floating point position and texel offset.
// Also zero's off screen.
vec3 Fetch(vec2 pos,vec2 off){
	pos=floor(pos*res+off)/res;
	if(max(abs(pos.x-0.5),abs(pos.y-0.5))>0.5)return vec3(0.0,0.0,0.0);
	return ToLinear(texture(samplerColor,pos.xy,-16.0).rgb);}

// Distance in emulated pixels to nearest texel.
vec2 Dist(vec2 pos){pos=pos*res;return -((pos-floor(pos))-vec2(0.5));}
	
// 1D Gaussian.
float Gaus(float pos,float scale){return exp2(scale*pos*pos);}

// 3-tap Gaussian filter along horz line.
vec3 Horz3(vec2 pos,float off){
	vec3 b=Fetch(pos,vec2(-1.0,off));
	vec3 c=Fetch(pos,vec2( 0.0,off));
	vec3 d=Fetch(pos,vec2( 1.0,off));
	float dst=Dist(pos).x;
	// Convert distance to weight.
	float scale=hardPix;
	float wb=Gaus(dst-1.0,scale);
	float wc=Gaus(dst+0.0,scale);
	float wd=Gaus(dst+1.0,scale);
	// Return filtered sample.
	return (b*wb+c*wc+d*wd)/(wb+wc+wd);}

// 5-tap Gaussian filter along horz line.
vec3 Horz5(vec2 pos,float off){
	vec3 a=Fetch(pos,vec2(-2.0,off));
	vec3 b=Fetch(pos,vec2(-1.0,off));
	vec3 c=Fetch(pos,vec2( 0.0,off));
	vec3 d=Fetch(pos,vec2( 1.0,off));
	vec3 e=Fetch(pos,vec2( 2.0,off));
	float dst=Dist(pos).x;
	// Convert distance to weight.
	float scale=hardPix;
	float wa=Gaus(dst-2.0,scale);
	float wb=Gaus(dst-1.0,scale);
	float wc=Gaus(dst+0.0,scale);
	float wd=Gaus(dst+1.0,scale);
	float we=Gaus(dst+2.0,scale);
	// Return filtered sample.
	return (a*wa+b*wb+c*wc+d*wd+e*we)/(wa+wb+wc+wd+we);}

// Return scanline weight.
float Scan(vec2 pos,float off){
	float dst=Dist(pos).y;
	return Gaus(dst+off,hardScan);}

// Allow nearest three lines to effect pixel.
vec3 Tri(vec2 pos){
	vec3 a=Horz3(pos,-1.0);
	vec3 b=Horz5(pos, 0.0);
	vec3 c=Horz3(pos, 1.0);
	float wa=Scan(pos,-1.0);
	float wb=Scan(pos, 0.0);
	float wc=Scan(pos, 1.0);
	return a*wa+b*wb+c*wc;}

// Distortion of scanlines, and end of screen alpha.
vec2 Warp(vec2 pos){
	pos=pos*2.0-1.0;		
	pos*=vec2(1.0+(pos.y*pos.y)*warp.x,1.0+(pos.x*pos.x)*warp.y);
	return pos*0.5+0.5;}

// Shadow mask.
vec3 Mask(vec2 pos){
	pos.x+=pos.y*3.0;
	vec3 mask=vec3(maskDark,maskDark,maskDark);
	pos.x=fract(pos.x/6.0);
	if(pos.x<0.333)mask.r=maskLight;
	else if(pos.x<0.666)mask.g=maskLight;
	else mask.b=maskLight;
	return mask;}		

void main() {
	iResolution = textureSize( samplerColor, 0 );
	fragCoord = inUv * iResolution;

	vec2 pos = Warp(fragCoord.xy / iResolution.xy);
	
		hardScan = -12.0;
		maskDark = maskLight = 1.0;

	outColor.rgb = Tri(pos) * Mask(fragCoord.xy);
	outColor.rgb = ToSrgb(outColor.rgb);
	outColor.a = 1.0;

#if TONE_MAP
	toneMap(outColor, ubo.exposure);
#endif
#if GAMMA_CORRECT
	gammaCorrect(outColor, ubo.gamma);
#endif
}
#endif
#if 0
void main() {
	const vec2 uv = 0.025 * sin( ubo.curTime ) * inUv.xy;
	const float mdf = 0.5;
	const float noise = (fract(sin(dot(uv, vec2(12.9898,78.233)*2.0)) * 43758.5453));
	const vec4 sampled = texture( samplerColor, inUv );
	
	outColor = sampled - noise * mdf;

	toneMap(outColor, ubo.exposure);
	gammaCorrect(outColor, ubo.gamma);
}
#endif