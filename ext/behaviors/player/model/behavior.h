#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/math/vector.h>

namespace ext {
	namespace PlayerModelBehavior {
		UF_BEHAVIOR_DEFINE_TYPE;
		EXT_BEHAVIOR_DEFINE_TRAITS;
		EXT_BEHAVIOR_DEFINE_FUNCTIONS;
		UF_BEHAVIOR_DEFINE_METADATA {
			bool hide = true;
			bool track = true;
			bool set = false;
			pod::Vector3f scale = {1,1,1};
		};
	}
}