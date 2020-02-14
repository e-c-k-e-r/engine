#include <iostream>
#if UF_USE_FREETYPE
#include <uf/ext/freetype/freetype.h>

ext::freetype::Library ext::freetype::library;

UF_API ext::freetype::Library::Library() : loaded(false) {
	ext::freetype::initialize();
}
ext::freetype::Library::~Library() {
	this->loaded = false;
	ext::freetype::terminate();
}
ext::freetype::Glyph::~Glyph() {
	ext::freetype::destroy(*this);
}

UF_API bool ext::freetype::initialize() {
	int error = 0;
	if ( (error = FT_Init_FreeType( &ext::freetype::library.library ) )) {
		std::cout << "Error #" << ext::freetype::getError(error) << ": FreeType failed to initialize" << std::endl;
		return false;
	}
	ext::freetype::library.loaded = true;
	return true;
}
UF_API bool ext::freetype::initialized() {
	return ext::freetype::library.loaded;
}
UF_API void ext::freetype::terminate() {
	FT_Done_FreeType(ext::freetype::library.library);
}

UF_API ext::freetype::Glyph ext::freetype::initialize( const std::string& font ) {
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
UF_API bool ext::freetype::initialize( ext::freetype::Glyph& glyph, const std::string& font ) {
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
UF_API void ext::freetype::destroy( ext::freetype::Glyph& glyph ) {
	FT_Done_Face( glyph.face );
}

UF_API void ext::freetype::setPixelSizes( ext::freetype::Glyph& glyph, int height ) {
	FT_Set_Pixel_Sizes( glyph.face, 0, height );
}
UF_API void ext::freetype::setPixelSizes( ext::freetype::Glyph& glyph, int width, int height ) {
	FT_Set_Pixel_Sizes( glyph.face, width, height );
}

UF_API bool ext::freetype::load( ext::freetype::Glyph& glyph, unsigned long c ) {
	int error = 0;
	if ( (error = FT_Load_Char(glyph.face, c, FT_LOAD_RENDER) )) {
		std::cout << "Error #" << ext::freetype::getError(error) << ": FreeType failed to load glyph `" << c << "`" << std::endl;
		return false;
	}
	return true;
}
UF_API bool ext::freetype::load( ext::freetype::Glyph& glyph, const uf::String& string ) {
	unsigned long c = string.translate<uf::locale::Utf32>().at(0);
	int error = 0;
	if ( (error = FT_Load_Char(glyph.face, FT_Get_Char_Index(glyph.face, c), FT_LOAD_RENDER) )) {
		std::cout << "Error #" << ext::freetype::getError(error) << ": FreeType failed to load glyph `" << c << "`" << std::endl;
		return false;
	}
	return true;
}

UF_API std::string ext::freetype::getError( int error ) {
	#undef FTERRORS_H_
	#define FT_ERRORDEF( e, v, s )  { e, s },
	#define FT_ERROR_START_LIST     {
	#define FT_ERROR_END_LIST       { 0, NULL } };

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