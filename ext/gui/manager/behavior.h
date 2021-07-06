#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>

namespace ext {
	namespace GuiManagerBehavior {
		UF_BEHAVIOR_DEFINE_TYPE();
		EXT_BEHAVIOR_DEFINE_TRAITS();
		EXT_BEHAVIOR_DEFINE_FUNCTIONS();
		UF_BEHAVIOR_DEFINE_METADATA(
			struct {
				struct {
					uf::stl::string type = "";
					pod::Vector4f color{};
					pod::Vector3f position{};
					float radius{};
					bool enabled{};
				} cursor;

				bool enabled{};
				bool floating{};
				pod::Transform<> transform;
			} overlay;
		);
	}
}