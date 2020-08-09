#pragma once

#include "../../../gui/gui.h"

namespace ext {
	class EXT_API GuiWorldPauseMenu : public ext::Gui {
	public:
		virtual void initialize();
		virtual void tick();
		virtual void render();
		virtual void destroy();
	};
}