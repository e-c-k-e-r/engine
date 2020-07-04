#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>

#include "../scene/scene.h"

namespace ext {
	class EXT_API MainMenu : public ext::Scene {
	public:
		virtual void initialize();
		virtual void tick();
		virtual void render();
		virtual uf::Entity* getController();
		virtual const uf::Entity* getController() const;
	};
}