#pragma once

#include <uf/config.h>
#if UF_USE_ULTRALIGHT

#include <uf/utils/memory/string.h>
#include <uf/utils/memory/vector.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/image/image.h>
#include <uf/utils/string/io.h>

#include <Ultralight/CAPI.h>

namespace pod {
	struct HTML {
		size_t uid = 0;
		ULView view;
		bool pending;
		uf::stl::string html;
		pod::Vector2ui size;
		struct {
			uf::stl::string load = "";
		} callbacks;
	};
}

namespace ext {
	namespace ultralight {
	//	extern UF_API uf::stl::string resourcesDir;
		extern UF_API uint8_t log;
		extern UF_API double scale;

		void UF_API initialize();
		void UF_API tick();
		void UF_API render();
		void UF_API terminate();

		pod::HTML UF_API create( const uf::stl::string&, const pod::Vector2ui& );
		pod::HTML& UF_API create( pod::HTML&, const uf::stl::string&, const pod::Vector2ui& );
		void UF_API destroy( pod::HTML& );
		void UF_API on( pod::HTML&, const uf::stl::string&, const uf::stl::string& );
		void UF_API resize( pod::HTML&, const pod::Vector2ui& );
		void UF_API input( pod::HTML&, const uf::stl::string& );
		uf::Image UF_API capture( pod::HTML& );
	}
}
#endif