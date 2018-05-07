#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include "../object/object.h"

namespace ext {
	class EXT_API Craeture : public ext::Object {
	public:
		struct Animation {
			std::map<std::string, pod::Transform<>> transforms;
			std::string state = "null";
		} m_animation;

		void initialize();
		void tick();
		void render();

		void animate();
	};
}