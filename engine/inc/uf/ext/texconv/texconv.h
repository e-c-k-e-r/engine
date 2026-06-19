#pragma once

#include <uf/config.h>

#include <uf/utils/memory/string.h>
#include <uf/utils/memory/vector.h>

#include <uf/utils/image/image.h>

namespace pod {
	struct TextureOptions {
		uf::stl::string format = "ARGB1555";
		bool mipmap = false;
		bool compress = false;
		bool stride = false;
		bool nearest = false;
		bool bilinear = false;
		bool verbose = false;

		uf::stl::string input;
		uf::stl::string output;

		uf::stl::string previewFile;
		uf::stl::string codeUsageFile;
	};

	// in case palette data needs to be saved
	struct Dtex {
		uf::stl::vector<uint8_t> imageData;
		uf::stl::vector<uint8_t> paletteData;
	};
}

namespace ext {
	namespace texconv {
		// reads from memory
		pod::Dtex UF_API convert( const uf::Image& image, const uf::stl::string& format = "auto" );
		// reads from disk
		inline pod::Dtex UF_API convert( const uf::stl::string& input, const uf::stl::string& format = "auto" ) {
			uf::Image image;
			image.open( input );
			return convert( image, format );
		}
		// CLI-like
		bool UF_API convert( const pod::TextureOptions& opts );

		// dumps to disk
		bool UF_API save( const pod::Dtex&, const uf::stl::string&, bool = false );
		// dumps to buffer
		bool UF_API save( const pod::Dtex&, uf::stl::vector<uint8_t>& );
	}
}