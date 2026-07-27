#if UF_USE_TRUETYPE
#include <uf/ext/truetype/truetype.h>
#include <uf/utils/io/file.h>
#include <uf/utils/memory/unordered_map.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace impl {
	uf::stl::unordered_map<uf::stl::string, uf::stl::vector<uint8_t>> fontCache;

	uint64_t first_codepoint(const std::u8string& str) {
		if ( str.empty() ) UF_EXCEPTION("Empty string");

		const uint8_t* bytes = reinterpret_cast<const uint8_t*>(str.data());
		uint8_t b0 = bytes[0];
		uint64_t codepoint = 0;
		int extra_bytes = 0;

		if (b0 < 0x80) {
			return b0;
		} else if ((b0 >> 5) == 0x6) {
			codepoint = b0 & 0x1F;
			extra_bytes = 1;
		} else if ((b0 >> 4) == 0xE) {
			codepoint = b0 & 0x0F;
			extra_bytes = 2;
		} else if ((b0 >> 3) == 0x1E) {
			codepoint = b0 & 0x07;
			extra_bytes = 3;
		} else {
			UF_EXCEPTION("Invalid UTF-8 start byte");
		}

		for (int i = 0; i < extra_bytes; ++i) {
			uint8_t bx = bytes[i+1];
			if ((bx >> 6) != 0x2) UF_EXCEPTION("Invalid continuation byte");
			codepoint = (codepoint << 6) | (bx & 0x3F);
		}

		return codepoint;
	}
}

pod::TrueTypeFont::TrueTypeFont() : info(nullptr) {}

pod::TrueTypeFont::~TrueTypeFont() {
	ext::truetype::destroy( *this );
}

bool ext::truetype::initialize() {
	return true;
}

void ext::truetype::terminate() {
	impl::fontCache.clear();
}

bool ext::truetype::initialize( pod::TrueTypeFont& g, const uf::stl::string& filename ) {
	if ( impl::fontCache.find(filename) == impl::fontCache.end() ) {
		uf::stl::vector<uint8_t> buffer;
		if ( !uf::io::readAsBuffer( buffer, filename ) ) {
			UF_MSG_ERROR("stb_truetype failed to read file: {}", filename);
			return false;
		}
		impl::fontCache[filename] = std::move(buffer);
	}

	const auto& buffer = impl::fontCache[filename];

	g.info = new stbtt_fontinfo();
	stbtt_fontinfo* info = static_cast<stbtt_fontinfo*>(g.info);

	if ( !stbtt_InitFont(info, buffer.data(), stbtt_GetFontOffsetForIndex(buffer.data(), 0)) ) {
		UF_MSG_ERROR("stb_truetype failed to initialize font memory '{}'", filename);
		delete info;
		g.info = nullptr;
		return false;
	}
	return true;
}

void ext::truetype::destroy( pod::TrueTypeFont& g ) {
	if ( g.info ) {
		delete static_cast<stbtt_fontinfo*>(g.info);
		g.info = nullptr;
	}
}

void ext::truetype::setPixelSizes( pod::TrueTypeFont& g, size_t height ) {
	if ( !g.info ) return;
	g.scale = stbtt_ScaleForPixelHeight(static_cast<stbtt_fontinfo*>(g.info), static_cast<float>(height));
}

bool ext::truetype::load( pod::TrueTypeFont& g, uint64_t c ) {
	g.current_codepoint = c;
	return true;
}

bool ext::truetype::load( pod::TrueTypeFont& g, const uf::stl::string& string ) {
#if UF_USE_DEPRECATED_STRING
	uint64_t c = string.translate<uf::locale::Utf32>().at(0);
#else
	uint64_t c = impl::first_codepoint( std::u8string( string.begin(), string.end() ) );
#endif
	return ext::truetype::load( g, c );
}
#endif