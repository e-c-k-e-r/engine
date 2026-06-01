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
			uf::stl::string string = "";
			uf::stl::string font = "Coolvetica.ttf";
			size_t size = 96;
			size_t spread = 8;
			pod::Vector2ui padding = { 8, 8 };

			struct {
				float scale = 1.5;
				float weight = 0.45;
				float fillWeight = 0.5;
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