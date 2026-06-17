#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/math/vector.h>

namespace ext {
	namespace PlayerCameraBehavior {
		UF_BEHAVIOR_DEFINE_TYPE();
		EXT_BEHAVIOR_DEFINE_TRAITS();
		EXT_BEHAVIOR_DEFINE_FUNCTIONS();
		UF_BEHAVIOR_DEFINE_METADATA(
			struct {
				pod::Vector3f current = {NAN, NAN, NAN};
				pod::Vector3f min = {NAN, NAN, NAN};
				pod::Vector3f max = {NAN, NAN, NAN};
			} limit;
			pod::Vector3t<bool> invert;
			pod::Vector2f queued = {};

			pod::Vector3f offset = {};
			pod::Transform<> intermediary;

			bool fixed = false;

			struct {
				pod::Vector2f sensitivity = {1, 1};
				pod::Vector2f smoothing = {0, 0};
			} mouse;

			float viewRoll = 0.0f;
			float previousRoll = 0.0f;
			float viewPunch = 0.0f;
			float previousPunch = 0.0f;
			float punchVelocity = 0.0f;
			float lastYVelocity = 0.0f;
			bool wasFloored = true;
			float stairOffset = 0.0f;
			float previousStairOffset = 0.0f;
		);
	}
}