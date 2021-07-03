#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/math/vector.h>

namespace ext {
	namespace LightBehavior {
		UF_BEHAVIOR_DEFINE_TYPE;
		EXT_BEHAVIOR_DEFINE_TRAITS;
		EXT_BEHAVIOR_DEFINE_FUNCTIONS;
		UF_BEHAVIOR_DEFINE_METADATA {
			pod::Vector3f color = {1,1,1};
			float power = 0.0f;
			float bias = 0.0f;
			bool shadows = false;
			int32_t type = 1;
			struct {
				uf::stl::string mode = "in-range";
				float limiter = 0.0f;
				float timer = 0.0f;
				bool rendered = false;
				bool external = false;
			} renderer;
		};
	}
}