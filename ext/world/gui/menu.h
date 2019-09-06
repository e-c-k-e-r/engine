#pragma once

#include "gui.h"

namespace ext {
	class EXT_API GuiMenu : public ext::Gui {
	public:
		virtual void initialize();
		virtual void tick();
		virtual void render();
		virtual void destroy();
	};
}