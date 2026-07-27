#pragma once

#include <uf/config.h>
#if UF_USE_TRUETYPE

#include <uf/utils/string/string.h>
#include <uf/utils/memory/vector.h>
#include <memory>

namespace pod {
	struct TrueTypeFont {
		void* info;
		float scale = 1.0f;
		uint64_t current_codepoint = 0;

		TrueTypeFont();
		~TrueTypeFont();
	};
}

namespace ext {
	namespace truetype {
		bool UF_API initialize();
		void UF_API terminate();

		bool UF_API initialize( pod::TrueTypeFont&, const uf::stl::string& );
		void UF_API destroy( pod::TrueTypeFont& );

		void UF_API setPixelSizes( pod::TrueTypeFont&, size_t );

		bool UF_API load( pod::TrueTypeFont&, uint64_t );
		bool UF_API load( pod::TrueTypeFont&, const uf::stl::string& );
	}
}

#endif
