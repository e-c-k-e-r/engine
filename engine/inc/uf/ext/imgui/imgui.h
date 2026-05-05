#pragma once

#include <uf/config.h>
#include <functional>

#if UF_USE_IMGUI

namespace ext {
	namespace imgui {
		extern UF_API bool focused;

		void initialize();
		void tick();
		void render();
		void terminate();
	}
}

#endif