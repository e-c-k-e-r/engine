#pragma once

#include "../base.h"

namespace ext {
	class EXT_API TestScene_WorldScape : public ext::Scene {
	public:
		virtual void initialize();
		virtual void tick();
		virtual void render();
	};
}