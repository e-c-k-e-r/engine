#pragma once

#include <uf/config.h>

#include <uf/utils/memory/string.h>
#include <uf/utils/memory/vector.h>

namespace ext {
	namespace texconv {
		struct TextureOptions {
			uf::stl::vector<uf::stl::string> inputs;
			uf::stl::string output;
			uf::stl::string format;
			bool mipmap = false;
			bool compress = false;
			bool stride = false;
			bool nearest = false;
			bool bilinear = false;
			bool verbose = false;

			uf::stl::string previewFile;
			uf::stl::string codeUsageFile;
		};

		bool UF_API convertTexture( const TextureOptions& opts );
	}
}