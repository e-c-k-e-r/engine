#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/graphic/graphic.h>


namespace ext {
	namespace RayTraceSceneBehavior {
		UF_BEHAVIOR_DEFINE_TYPE();
		EXT_BEHAVIOR_DEFINE_TRAITS();
		EXT_BEHAVIOR_DEFINE_FUNCTIONS();
		UF_BEHAVIOR_DEFINE_METADATA(
			struct {
				bool bound = false;
				float scale = 1;
			} renderer;
		);
	}
}