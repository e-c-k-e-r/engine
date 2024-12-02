#pragma once

#include <uf/config.h>

#include <uf/utils/memory/string.h>
#include <uf/utils/math/vector.h>

namespace pod {
	namespace payloads {	
		struct menuOpen {
			uf::stl::string name = "";
			uf::Serializer metadata;
		};

		struct worldCollision {
			size_t hit = 0;
			float depth = -1;
		};

		struct vrInputDigital {
			uf::stl::string type = "";
			int_fast8_t size{};
			int_fast8_t state{};
		};

		struct vrHaptics {
			float delay;
			float duration;
			float frequency;
			float amplitude;
			int_fast8_t side;
		};
	}
}