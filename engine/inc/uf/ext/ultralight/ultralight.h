#pragma once

#include <uf/config.h>
#if UF_USE_ULTRALIGHT

#include <string>
#include <vector>
#include <uf/utils/math/vector.h>
#include <uf/utils/image/image.h>
#include <uf/utils/string/io.h>

#include <Ultralight/CAPI.h>

namespace pod {
	struct HTML {
		size_t uid = 0;
		ULView view;
		bool pending;
		std::string html;
		pod::Vector2ui size;
		struct {
			std::string load = "";
		} callbacks;
	};
}

namespace ext {
	namespace ultralight {
	//	extern UF_API std::string resourcesDir;
		extern UF_API uint8_t log;
		extern UF_API double scale;

		void UF_API initialize();
		void UF_API tick();
		void UF_API render();
		void UF_API terminate();

		pod::HTML UF_API create( const std::string&, const pod::Vector2ui& );
		pod::HTML& UF_API create( pod::HTML&, const std::string&, const pod::Vector2ui& );
		void UF_API destroy( pod::HTML& );
		void UF_API on( pod::HTML&, const std::string&, const std::string& );
		void UF_API resize( pod::HTML&, const pod::Vector2ui& );
		void UF_API input( pod::HTML&, const std::string& );
		uf::Image UF_API capture( pod::HTML& );
	}
}
#endif