#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>

namespace ext {
	namespace AudioEmitterBehavior {
		UF_BEHAVIOR_DEFINE_TYPE();
		EXT_BEHAVIOR_DEFINE_TRAITS();
		EXT_BEHAVIOR_DEFINE_FUNCTIONS();
		UF_BEHAVIOR_DEFINE_METADATA(
			struct BGM {
				pod::Component::userdata_t userdata;
				uf::stl::string filename;

				uf::stl::string intro;
				float epsilon = 0.0001f;
				pod::Vector2f fade = {};
				float prevTime = 0.0f;

				bool active = false;
			};

			uf::stl::string current = "";
			
			uf::stl::unordered_map<uf::stl::string, BGM> tracks;
		);
	}
}