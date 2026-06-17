#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/math/vector.h>

namespace ext {
	namespace PlayerMovementBehavior {
		UF_BEHAVIOR_DEFINE_TYPE();
		EXT_BEHAVIOR_DEFINE_TRAITS();
		EXT_BEHAVIOR_DEFINE_FUNCTIONS();
		UF_BEHAVIOR_DEFINE_METADATA(
			struct Settings {
				float crouch = -1.0f;
				float rotate = 1.0f;
				float move = 4.0f;
				float run = 8.0f;
				float walk = 1.0f;
				float friction = 0.8f;
				float air = 1.0f;
				float stepHeight = 0.35f;
				bool strafe = true;
				pod::Vector3f jump = {0, 8, 0};
			} settings;

			bool walking = false;
			bool running = false;
			bool crouching = false;
			bool floored = true;
			bool noclipped = false;
			bool deltaCrouch = false;
		);
	}
}