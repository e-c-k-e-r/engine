#pragma once
#if UF_USE_LGS
#include <uf/config.h>
#include <uf/engine/graph/graph.h>

namespace ext {
	namespace lgs {
		bool UF_API loadPalette( const uf::stl::string& family, uf::stl::vector<uint8_t>& palette );
		bool UF_API loadPcx( pod::Image& image, const uf::stl::vector<uint8_t>& buffer, const uint8_t* paletteData = NULL );
	}
}
#endif