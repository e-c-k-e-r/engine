#ifndef NO_NONUNIFORM_EXT
	#define NO_NONUNIFORM_EXT 0
	// enable if shaderNonUniform is not supported
	// Nvidia hardware does not require nonuniformEXT, but AMD does
#endif

// implicit variables
#ifndef MULTISAMPLING
	#define MULTISAMPLING 1
#endif
#ifndef MAX_MSAA_SAMPLES
	#define MAX_MSAA_SAMPLES 16
#endif
#ifndef MAX_TEXTURES
	#define MAX_TEXTURES TEXTURES
#endif
#ifndef MAX_LIGHTS
	#define MAX_LIGHTS ubo.settings.lengths.lights
#endif
#ifndef MAX_SHADOWS
	#define MAX_SHADOWS ubo.settings.lighting.maxShadows
#endif
#ifndef VIEW_MATRIX
	#define VIEW_MATRIX ubo.eyes[surface.pass].view
#endif

// implicit shader settings
#ifndef CAN_DISCARD
	#define CAN_DISCARD 1
#endif
#ifndef USE_LIGHTMAP
	#define USE_LIGHTMAP 1
#endif
#if VXGI
	#define VXGI_NDC 1
	#define VXGI_SHADOWS 0
#endif

/*
#ifndef FOG
	#define FOG 1
#endif
#ifndef FOG_RAY_MARCH
	#define FOG_RAY_MARCH 1
#endif
#ifndef WHITENOISE
	#define WHITENOISE 1
#endif
#ifndef GAMMA_CORRECT
	#define GAMMA_CORRECT 1
#endif
#ifndef TONE_MAP
	#define TONE_MAP 1
#endif
#ifndef PHONG
	#define PHONG 0
#endif
#ifndef LAMBERT
	#define LAMBERT 0
#endif
#ifndef PBR
	#define PBR 1
#endif
*/

#if NO_NONUNIFORM_EXT
	#define nonuniformEXT(X) X
#else
	#extension GL_EXT_nonuniform_qualifier : enable
#endif

#if !UINT64_ENABLED
	#define uint64_t uvec2
#endif

// easy and accessible in one place
#ifndef BARYCENTRIC
	#define BARYCENTRIC 0
#endif
#if BARYCENTRIC
	#ifndef BARYCENTRIC_CALCULATE
		#define BARYCENTRIC_CALCULATE 0
	#endif
#endif

#if BUFFER_REFERENCE
	#extension GL_EXT_scalar_block_layout : enable
	#extension GL_EXT_buffer_reference : enable
	#extension GL_EXT_buffer_reference2 : enable
	#if UINT64_ENABLED
		#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable
	#else
		#extension GL_EXT_buffer_reference_uvec2 : enable
	#endif
#endif

const float PI = 3.14159265359;
const float EPSILON = 0.00001;
const float SQRT2 = 1.41421356237;

const float LIGHT_POWER_CUTOFF = 0.0005;
const float LIGHTMAP_GAMMA = 1.0;

#define SETTINGS_TYPE_FULLBRIGHT 0x10