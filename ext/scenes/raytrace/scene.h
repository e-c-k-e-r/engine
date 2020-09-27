#pragma once

#include "../base.h"

namespace ext {
	class EXT_API TestScene_RayTracing : public ext::Scene {
	public:
		virtual void initialize();
		virtual void tick();
		virtual void render();
		virtual void destroy();
	};
}