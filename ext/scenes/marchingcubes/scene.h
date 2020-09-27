#pragma once

#include "../base.h"

namespace ext {
	class EXT_API TestScene_MarchingCubes : public ext::Scene {
	public:
		virtual void initialize();
		virtual void tick();
		virtual void render();
	};
}