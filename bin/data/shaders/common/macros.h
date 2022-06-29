#ifndef NO_NONUNIFORM_EXT
	#define NO_NONUNIFORM_EXT 0
	// enable if shaderNonUniform is not supported
	// Nvidia hardware does not require nonuniformEXT, but AMD does
#endif
#ifndef MULTISAMPLING
	#define MULTISAMPLING 1
#endif
#ifndef MAX_MSAA_SAMPLES
	#define MAX_MSAA_SAMPLES 16
#endif
#ifndef DEFERRED_SAMPLING
	#define DEFERRED_SAMPLING 1
#endif
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
#ifndef MAX_TEXTURES
	#define MAX_TEXTURES TEXTURES
#endif
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
#ifndef LAMBERT
	#define LAMBERT 0
#endif
#ifndef PBR
	#define PBR 1
#endif

#if NO_NONUNIFORM_EXT
	#define nonuniformEXT(X) X
#endif

#ifndef BUFFER_REFERENCE
	#define BUFFER_REFERENCE 1
#else
	#define UINT64_ENABLED 1
#endif

#if BUFFER_REFERENCE
	#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable
	#extension GL_EXT_buffer_reference : enable
	#extension GL_EXT_buffer_reference2 : enable
	#extension GL_EXT_scalar_block_layout : enable
#endif

const float PI = 3.14159265359;
const float EPSILON = 0.00001;
const float SQRT2 = 1.41421356237;

const float LIGHT_POWER_CUTOFF = 0.0005;
const float LIGHTMAP_GAMMA = 1.0;