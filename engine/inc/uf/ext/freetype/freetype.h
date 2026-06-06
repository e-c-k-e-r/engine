#pragma once

#include <uf/config.h>
#if UF_USE_FREETYPE

#if UF_ENV_DREAMCAST
	#include <ft2build.h>
#else
	#include <ft2build.h>
#endif
#include FT_FREETYPE_H 

#include <uf/utils/string/string.h>
#include <uf/utils/memory/vector.h>
#include <memory>

namespace pod {
	struct FT_Glyph {
		FT_Face face;
		~FT_Glyph();
	};
}

namespace ext {
	namespace freetype {
		bool UF_API initialize();
		void UF_API terminate();
		
		bool UF_API initialize( pod::FT_Glyph&, const uf::stl::string& );
		void UF_API destroy( pod::FT_Glyph& );
		
		void UF_API setPixelSizes( pod::FT_Glyph&, size_t ); 
		void UF_API setPixelSizes( pod::FT_Glyph&, size_t, size_t ); 
		void UF_API setRenderMode( pod::FT_Glyph&, FT_Render_Mode = FT_RENDER_MODE_NORMAL );
		
		bool UF_API load( pod::FT_Glyph&, uint64_t );
		bool UF_API load( pod::FT_Glyph&, const uf::stl::string& );
	}
}

#endif