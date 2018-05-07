#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>

#include "player/player.h"
#include "craeture/craeture.h"
#include "object/object.h"

namespace ext {
	class EXT_API World : public uf::Entity {
	protected:
		ext::Player m_player;
	public:
		ext::Player& getPlayer();
		const ext::Player& getPlayer() const;

		uf::Camera& getCamera();

		void initialize();
		void tick();
		void render();

		bool load();
	};
}