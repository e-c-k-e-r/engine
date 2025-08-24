#include <iostream>
#if UF_USE_FREETYPE
#include <uf/ext/freetype/freetype.h>

namespace {
	unsigned long first_codepoint(const std::u8string& str) {
		if (str.empty()) UF_EXCEPTION("Empty string");

		const unsigned char* bytes = reinterpret_cast<const unsigned char*>(str.data());
		unsigned char b0 = bytes[0];
		unsigned long codepoint = 0;
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
			unsigned char bx = bytes[i+1];
			if ((bx >> 6) != 0x2) UF_EXCEPTION("Invalid continuation byte");
			codepoint = (codepoint << 6) | (bx & 0x3F);
		}

		return codepoint;
	}
}

ext::freetype::Library ext::freetype::library;

ext::freetype::Library::Library() : loaded(false) {
	ext::freetype::initialize();
}
ext::freetype::Library::~Library() {
	this->loaded = false;
	ext::freetype::terminate();
}
ext::freetype::Glyph::~Glyph() {
	ext::freetype::destroy(*this);
}

bool ext::freetype::initialize() {
	int error = 0;
	if ( (error = FT_Init_FreeType( &ext::freetype::library.library ) )) {
		std::cout << "Error #" << ext::freetype::getError(error) << ": FreeType failed to initialize" << std::endl;
		return false;
	}
	ext::freetype::library.loaded = true;
	return true;
}
bool ext::freetype::initialized() {
	return ext::freetype::library.loaded;
}
void ext::freetype::terminate() {
	if ( !ext::freetype::library.loaded ) return;
	FT_Done_FreeType(ext::freetype::library.library);
	ext::freetype::library.loaded = false;
}

ext::freetype::Glyph ext::freetype::initialize( const uf::stl::string& font ) {
	if ( !ext::freetype::initialized() ) ext::freetype::initialize();
	ext::freetype::Glyph glyph;
	int error = 0;
	if ( (error = FT_New_Face( ext::freetype::library.library, font.c_str(), 0, &glyph.face )) ) {
		std::cout << "Error #" <<  ext::freetype::getError(error) << ": FreeType failed to load file `" << font << "`" << std::endl;
	}
	if ( (error = FT_Select_Charmap( glyph.face, FT_ENCODING_UNICODE )) ) {
		std::cout << "Error #" <<  ext::freetype::getError(error) << ": FreeType failed to load file `" << font << "`" << std::endl;
	}
	return glyph;
}
bool ext::freetype::initialize( ext::freetype::Glyph& glyph, const uf::stl::string& font ) {
	if ( !ext::freetype::initialized() ) ext::freetype::initialize();
	int error = 0;
	if ( (error = FT_New_Face( ext::freetype::library.library, font.c_str(), 0, &glyph.face )) ) {
		std::cout << "Error #" <<  ext::freetype::getError(error) << ": FreeType failed to load file `" << font << "`" << std::endl;
		return false;
	}
	if ( (error = FT_Select_Charmap( glyph.face, FT_ENCODING_UNICODE )) ) {
		std::cout << "Error #" <<  ext::freetype::getError(error) << ": FreeType failed to load file `" << font << "`" << std::endl;
		return false;
	}
	return true;
}
void ext::freetype::destroy( ext::freetype::Glyph& glyph ) {
	FT_Done_Face( glyph.face );
}

void ext::freetype::setPixelSizes( ext::freetype::Glyph& glyph, int height ) {
	FT_Set_Pixel_Sizes( glyph.face, 0, height );
}
void ext::freetype::setPixelSizes( ext::freetype::Glyph& glyph, int width, int height ) {
	FT_Set_Pixel_Sizes( glyph.face, width, height );
}

void ext::freetype::setRenderMode( ext::freetype::Glyph& glyph, decltype(FT_RENDER_MODE_NORMAL) mode ) {
	FT_Render_Glyph( glyph.face->glyph, mode );
}

bool ext::freetype::load( ext::freetype::Glyph& glyph, unsigned long c ) {
	int error = 0;
	if ( (error = FT_Load_Char(glyph.face, c, FT_LOAD_RENDER) )) {
		std::cout << "Error #" << ext::freetype::getError(error) << ": FreeType failed to load glyph `" << c << "`" << std::endl;
		return false;
	}
	return true;
}
bool ext::freetype::load( ext::freetype::Glyph& glyph, const uf::stl::string& string ) {
#if UF_USE_DEPRECATED_STRING
	unsigned long c = string.translate<uf::locale::Utf32>().at(0);
#else
	unsigned long c = first_codepoint( std::u8string( string.begin(), string.end() ) );
#endif
	int error = 0;
	if ( (error = FT_Load_Char(glyph.face, FT_Get_Char_Index(glyph.face, c), FT_LOAD_RENDER) )) {
		std::cout << "Error #" << ext::freetype::getError(error) << ": FreeType failed to load glyph `" << c << "`" << std::endl;
		return false;
	}
	return true;
}

uf::stl::string ext::freetype::getError( int error ) {
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
	//	std::cout << ft_errors[i].err_msg << std::endl;
		if ( ft_errors[i].err_code == error )
			return ft_errors[i].err_msg;
	}

	return ft_errors[0].err_msg;
}
#endif