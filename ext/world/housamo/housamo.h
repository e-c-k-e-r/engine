#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include "../craeture/craeture.h"

namespace ext {
	class EXT_API Housamo : public ext::Object {
	protected:
	public:
		virtual void initialize();
		virtual void tick();
		virtual void render();
		virtual void destroy();
	};
}