#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/renderer/renderer.h>

#include "behavior.h"

namespace pod {
	struct GlyphBox {
		struct {
			float x, y, w, h;
		} box;
		uint64_t code;
		pod::Vector3f color;
	};
}

namespace ext {
	class EXT_API Gui : public uf::Object {
	public:
		typedef uf::BaseMesh<pod::Vertex_3F2F3F> mesh_t;
		typedef uf::BaseMesh<pod::Vertex_3F2F3F> glyph_mesh_t;
	//	Gui();
		uf::stl::vector<pod::GlyphBox> generateGlyphs( const uf::stl::string& = "" );
		void load( const uf::Image& );
	};
	namespace gui {
		struct Size {
			pod::Vector2ui current = {};
			pod::Vector2ui reference = {};
		};
		extern Size size;
	}
}