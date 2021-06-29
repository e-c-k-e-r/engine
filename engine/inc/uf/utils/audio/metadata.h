#pragma once

#include <uf/utils/memory/string.h>
#include <fstream>

#include <uf/ext/oal/source.h>
#include <uf/ext/oal/buffer.h>

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