#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include "../object/object.h"
#include "../craeture/craeture.h"

namespace ext {
	class EXT_API Player : public ext::Craeture {
	protected:
	public:
		void initialize();
		void tick();
		void render();
	};
}