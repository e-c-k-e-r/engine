#pragma once

#include <fstream>
#include <uf/utils/math/transform.h>

#include <uf/utils/memory/string.h>
#include <uf/utils/memory/vector.h>

#include <uf/ext/openal/source.h>
#include <uf/ext/openal/buffer.h>
#include <uf/utils/time/time.h>

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