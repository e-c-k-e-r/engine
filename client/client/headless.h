#pragma once

#include <uf/utils/math/vector.h>
#include <uf/engine/ext.h>

namespace client {
	namespace headless {
		// yuck
		inline bool active() {
			return uf::headless;
		}

		inline constexpr pod::Vector2i defaultSize = { 1280, 720 };

		void configure();
		void tick();
		void terminate();
	}
}
