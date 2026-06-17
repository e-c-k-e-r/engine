#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/math/vector.h>

namespace ext {
	namespace PlayerInputBehavior {
		UF_BEHAVIOR_DEFINE_TYPE();
		EXT_BEHAVIOR_DEFINE_TRAITS();
		EXT_BEHAVIOR_DEFINE_FUNCTIONS();
		UF_BEHAVIOR_DEFINE_METADATA(
			pod::Vector2f movement = {};
			pod::Vector2f look = {};

			bool control = true;
			uf::stl::string menu = "";

			bool jump = false;
			bool crouch = false;
			bool run = false;
			bool walk = false;
			bool use = false;
			bool noclipToggle = false;
			bool menuToggle = false;

			float magnitude = 1.0f;
		);
	};
}