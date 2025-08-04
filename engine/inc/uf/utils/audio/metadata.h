#pragma once

#include <fstream>

#include <uf/utils/memory/string.h>
#include <uf/utils/memory/vector.h>

#include <uf/ext/oal/source.h>
#include <uf/ext/oal/buffer.h>
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

namespace uf {
	namespace audio {
		struct UF_API Metadata {
			uf::stl::string filename = "";
			uf::stl::string extension = "";
			struct {
				std::ifstream* file = NULL;
				void* handle = NULL;
				char* buffer = NULL;
				
				size_t consumed = 0;
				int bitStream = 0;
			} stream;
			struct {
				ext::al::Source source;
				ext::al::Buffer buffer;
			} al;
			struct {
				uint8_t channels = 0;
				uint8_t bitDepth = 0;
				uint32_t frequency = 0;
				size_t size = 0;

				uint32_t format = 0;
				float duration = 0;

				uf::Timer<> timer;
				float elapsed = 0;
			} info;
			struct {
				bool streamed = true;
				bool loop = false;
				uint8_t buffers = 4;
				uint8_t loopMode = 0;
			} settings;
		};
	}
}