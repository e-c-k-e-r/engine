#pragma once

#include <uf/config.h>
#if UF_USE_FREETYPE

#if UF_ENV_DREAMCAST
	#include <freetype/freetype2/ft2build.h>
#else
	#include <ft2build.h>
#endif
#include FT_FREETYPE_H 

#include <uf/utils/string/string.h>

namespace ext {
	namespace freetype {
		struct UF_API Library {
			bool loaded;
			FT_Library library;
			Library();
			~Library();
		};
		struct UF_API Glyph {
			FT_Face face;
			~Glyph();
		};
		extern UF_API ext::freetype::Library library;
		UF_API bool initialize();
		UF_API bool initialized();
		UF_API void terminate();
		
		UF_API ext::freetype::Glyph initialize( const std::string& );
		UF_API bool initialize( ext::freetype::Glyph&, const std::string& );
		UF_API void destroy( ext::freetype::Glyph& );
		
		UF_API void setPixelSizes( ext::freetype::Glyph&, int ); 
		UF_API void setPixelSizes( ext::freetype::Glyph&, int, int ); 
		
		UF_API bool load( ext::freetype::Glyph&, unsigned long );
		UF_API bool load( ext::freetype::Glyph&, const uf::String& );

		UF_API std::string getError( int );
	}
}

#endif