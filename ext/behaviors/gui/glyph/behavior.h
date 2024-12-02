#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/renderer/renderer.h>

namespace ext {
	namespace GuiGlyphBehavior {
		UF_BEHAVIOR_DEFINE_TYPE();
		EXT_BEHAVIOR_DEFINE_TRAITS();
		EXT_BEHAVIOR_DEFINE_FUNCTIONS();
		UF_BEHAVIOR_DEFINE_METADATA(
			uf::stl::string font = "Coolvetica.ttf";
			uf::stl::string string = "";

			bool sdf = true;
			float size = 96;
			float spread = 2;

			pod::Vector2ui padding = { 2, 2 };

			size_t reserve = 128;

			struct {
				float scale = 1.5;
				float weight = 0.45;
				pod::Vector4f stroke = { 0, 0, 0, 1 };
				pod::Vector2i range = { -1, -1 };
			} shader;
		);
	}
}

/*

"stroke": [ 0, 0, 0, 1 ],

"spread": 4,
"scale": 1.5,
"weight": 0.45,
"padding": [2, 2],
"sdf": true

*/