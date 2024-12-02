#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/math/vector.h>

namespace ext {
	namespace RegionBehavior {
		UF_BEHAVIOR_DEFINE_TYPE();
		EXT_BEHAVIOR_DEFINE_TRAITS();
		EXT_BEHAVIOR_DEFINE_FUNCTIONS();
		UF_BEHAVIOR_DEFINE_METADATA(
			struct {
				pod::Vector3ui chunkRange = { 3, 1, 3 };
				pod::Vector3ui chunkSize = { 32, 32, 32 };

				float cooldown = 5.0f;
			} settings;

			struct {
				float checkTime = 0.0f;
				pod::Vector3i controllerIndex = {};
			} last;

			bool ready = false;
			uf::stl::unordered_map<pod::Vector3i, uf::Object*> chunks;
		);
	}
}