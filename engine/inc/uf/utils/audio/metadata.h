#pragma once

#include <fstream>
#include <uf/utils/math/transform.h>

#include <uf/utils/memory/string.h>
#include <uf/utils/memory/vector.h>

#include <uf/ext/openal/source.h>
#include <uf/ext/openal/buffer.h>
#include <uf/utils/time/time.h>

#define UF_AUDIO_CALL_1(str, fmt, fun, ...) if ( (str) == #fmt ) ext::fmt::fun(__VA_ARGS__);
#define UF_AUDIO_CALL_0(str, fmt, fun, ...)

#define UF_AUDIO_DISPATCH_EVAL(use_val, str, fmt, fun, ...) \
	TOKEN_PASTE(UF_AUDIO_CALL_, use_val)(str, fmt, fun, __VA_ARGS__)

#define UF_AUDIO_DISPATCH(str, fmt, fun, ...) \
	UF_AUDIO_DISPATCH_EVAL(TOKEN_PASTE(UF_USE_, fmt), str, fmt, fun, __VA_ARGS__)


#define UF_AUDIO_CALL_SET_1(str, fmt, val, fun, ...) if ( (str) == #fmt ) val = ext::fmt::fun(__VA_ARGS__);
#define UF_AUDIO_CALL_SET_0(str, fmt, val, fun, ...)

#define UF_AUDIO_DISPATCH_SET_EVAL(use_val, str, fmt, val, fun, ...) \
    TOKEN_PASTE(UF_AUDIO_CALL_SET_, use_val)(str, fmt, val, fun, __VA_ARGS__)

#define UF_AUDIO_DISPATCH_SET(str, fmt, val, fun, ...) \
    UF_AUDIO_DISPATCH_SET_EVAL(TOKEN_PASTE(UF_USE_, fmt), str, fmt, val, fun, __VA_ARGS__)

// shoved here because dependencies
namespace pod {
	// this technically could either be a template or have the samples buffer be uint8_t and store the bit depth / an enum for the format but I only really care about supporting 16-bit PCMs
	struct UF_API PCM {
		uf::stl::vector<int16_t> samples;
		uint16_t sampleRate = 24000;
		uint16_t channels = 1;
	};
}

namespace pod {
	struct UF_API AudioClip {
		uf::stl::string filename = "";
		uf::stl::string extension = "";

		ext::al::Buffer alBuffer;

		struct {
			uint8_t channels = 0;
			uint8_t bitDepth = 0;
			uint32_t frequency = 0;
			size_t size = 0;
			uint32_t format = 0;
			float duration = 0;
			
			struct {
				bool has = false;
				uint32_t start = 0;
				uint32_t end = 0;
			} loop;
		} info;

		bool streamed = false;

		struct {
			void* buffer = NULL;
		} stream;
	};

	struct UF_API AudioSource {
		ext::al::Source alSource;
	#if !UF_ENV_DREAMCAST
		ext::al::Filter alFilter;
	#endif
		ext::al::Buffer streamBuffers;

		pod::AudioClip* clip = NULL;
		pod::Transform<> transform;

		struct {
			uf::Timer<> timer;
			float elapsed = 0;
			uf::stl::vector<uf::stl::string> pending;
		} info;

		struct {
			bool loop = false;
			bool spatial = false;
			uint8_t buffers = 4;
			uint8_t loopMode = 0;
		} settings;

		struct {
			void* context = NULL;
			void* handle = NULL;
			size_t consumed = 0;
			int bitStream = 0;
		} streamState;
	};
}