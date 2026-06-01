#include <iostream>
#if UF_USE_FREETYPE
#include <uf/ext/freetype/freetype.h>

namespace impl {
	FT_Library library;

	uf::stl::string error( int error ) {
		#undef FTERRORS_H_
		#define FT_ERRORDEF( e, v, s )  { e, s },
		#define FT_ERROR_START_LIST 	{
		#define FT_ERROR_END_LIST	   	{ 0, NULL } };

		const struct FTErrors {
			int err_code;
			const char* err_msg;
		} ft_errors[] =
		#include FT_ERRORS_H

		for ( int i = 0; i < sizeof(ft_errors) / sizeof(FTErrors); ++i ) {
			if ( ft_errors[i].err_code == error ) return ft_errors[i].err_msg;
		}

		return ft_errors[0].err_msg;
	}

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

pod::FT_Glyph::~FT_Glyph() {
	ext::freetype::destroy( *this );
}

bool ext::freetype::initialize() {
	if ( auto error = FT_Init_FreeType( &impl::library ) ) {
		UF_MSG_ERROR("FreeType failed to initialize: {}", impl::error( error ));
		return false;
	}
	return true;
}
void ext::freetype::terminate() {
	FT_Done_FreeType( impl::library );
}

pod::FT_Glyph ext::freetype::initialize( const uf::stl::string& font ) {
	pod::FT_Glyph g;
	if ( auto error = FT_New_Face( impl::library, font.c_str(), 0, &g.face ) ) {
		UF_MSG_ERROR("FreeType failed to load file '{}': {}", font, impl::error( error ));
	}
	if ( auto error = FT_Select_Charmap( g.face, FT_ENCODING_UNICODE ) ) {
		UF_MSG_ERROR("FreeType failed to load file '{}': {}", font, impl::error( error ));
	}
	return g;
}
bool ext::freetype::initialize( pod::FT_Glyph& g, const uf::stl::string& font ) {
	if ( auto error = FT_New_Face( impl::library, font.c_str(), 0, &g.face ) ) {
		UF_MSG_ERROR("FreeType failed to load file '{}': {}", font, impl::error( error ));
		return false;
	}
	if ( auto error = FT_Select_Charmap( g.face, FT_ENCODING_UNICODE ) ) {
		UF_MSG_ERROR("FreeType failed to load file '{}': {}", font, impl::error( error ));
		return false;
	}
	return true;
}
void ext::freetype::destroy( pod::FT_Glyph& g ) {
	FT_Done_Face( g.face );
}

void ext::freetype::setPixelSizes( pod::FT_Glyph& g, size_t height ) {
	FT_Set_Pixel_Sizes( g.face, 0, height );
}
void ext::freetype::setPixelSizes( pod::FT_Glyph& g, size_t width, size_t height ) {
	FT_Set_Pixel_Sizes( g.face, width, height );
}

void ext::freetype::setRenderMode( pod::FT_Glyph& g, FT_Render_Mode mode ) {
	FT_Render_Glyph( g.face->glyph, mode );
}

bool ext::freetype::load( pod::FT_Glyph& g, uint64_t c ) {
	if ( auto error = FT_Load_Char(g.face, c, FT_LOAD_RENDER) ) {
		UF_MSG_ERROR("FreeType failed to load glyph '{}': {}", c, impl::error( error ));
		return false;
	}
	return true;
}
bool ext::freetype::load( pod::FT_Glyph& g, const uf::stl::string& string ) {
#if UF_USE_DEPRECATED_STRING
	uint64_t c = string.translate<uf::locale::Utf32>().at(0);
#else
	uint64_t c = impl::first_codepoint( std::u8string( string.begin(), string.end() ) );
#endif
	return ext::freetype::load( g, c );
}
#endif