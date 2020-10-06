#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>

#include "behavior.h"

namespace ext {
	class EXT_API Gui : public uf::Object {
	public:
		Gui() {
			this->addBehavior<ext::GuiBehavior>();
		}
	};
}