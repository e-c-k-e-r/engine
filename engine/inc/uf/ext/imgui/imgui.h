#pragma once

#include <uf/config.h>

#if UF_USE_IMGUI

namespace ext {
	namespace imgui {
		void initialize();
		void tick();
		void render();
		void terminate();
	}
}

#endif