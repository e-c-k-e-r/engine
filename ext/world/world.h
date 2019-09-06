#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>

#include "player/player.h"
#include "craeture/craeture.h"
#include "object/object.h"

namespace ext {
	class EXT_API World : public ext::Object {
	public:
		ext::Player& getPlayer();
		const ext::Player& getPlayer() const;

		uf::Camera& getCamera();

		virtual void initialize();
		virtual void tick();
		virtual void render();
	};
}