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
				pod::Vector2ui size{};
				uf::renderer::enums::Filter::type_t filter = uf::renderer::enums::Filter::NEAREST;

				bool full = true;
			} renderer;

			struct {
				pod::Vector2f defaultRayBounds = {0.001, 4096.0};
				float alphaTestOffset = 0.001;

				uint32_t samples = 1;
				uint32_t paths = 1;
				uint32_t frameAccumulationMinimum = 0;
			} settings;
		);
	}
}