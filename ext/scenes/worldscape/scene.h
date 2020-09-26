#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>

#include <uf/engine/scene/scene.h>

namespace ext {
	class EXT_API TestScene_WorldScape : public uf::Scene {
	public:
		virtual void initialize();
		virtual void tick();
		virtual void render();
	};
}