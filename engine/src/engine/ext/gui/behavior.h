#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/renderer/renderer.h>


namespace ext {
	struct Gui : public uf::Object {

	};

	namespace GuiBehavior {
		UF_BEHAVIOR_DEFINE_TYPE();
		EXT_BEHAVIOR_DEFINE_TRAITS();
		EXT_BEHAVIOR_DEFINE_FUNCTIONS();
		UF_BEHAVIOR_DEFINE_METADATA(
			bool initialized = false;

			bool world = false;

			float depth = 0;
			size_t mode = 0;

			pod::Vector2ui size = {};
			pod::Vector2f scale = { 1, 1 };
			pod::Vector4f uv = { 0, 0, 1, 1 };
			pod::Vector4f color = { 1, 1, 1, 1 };

			uf::stl::string renderMode = "Gui";
			uf::stl::string scaleMode = "fixed";

			struct {
				struct {
					bool able = false;
					bool ed = false;
				} click, hover;
			} ui;

			struct {
				pod::Vector2f min = {  1,  1 };
				pod::Vector2f max = { -1, -1 };
			} box;
		);
	}
}