#pragma once

#include <uf/config.h>
#if UF_USE_TRUETYPE
#include <uf/utils/memory/vector.h>
#include <uf/utils/math/vector.h>
#include <uf/ext/truetype/truetype.h>
#include <uf/utils/image/atlas.h>
#include <uf/utils/mesh/mesh.h>

namespace pod {
	struct Glyph {
		pod::Vector2ui size = {};
		pod::Vector2i bearing = {};
		pod::Vector2i advance = {};
		pod::Vector2i padding = {};
		size_t spread = 0;

		uf::stl::vector<uint8_t> buffer;
	};
}

namespace uf {
	namespace glyph {
		pod::TrueTypeFont UF_API initialize( const uf::stl::string& font );
		uint8_t* UF_API generate( pod::Glyph& glyph, pod::TrueTypeFont& face, uint64_t c, size_t size = 48 );
		uint8_t* UF_API generate( pod::Glyph& glyph, pod::TrueTypeFont& face, const uf::stl::string& s, size_t size = 48 );
		uint8_t* UF_API generate( pod::Glyph& glyph, pod::TrueTypeFont& face );

		void UF_API generateSdf( pod::Glyph& glyph );
	}
}

#endif