#pragma once

#include <uf/config.h>
#include <uf/utils/memory/vector.h>
#include <uf/utils/image/atlas.h>
#include <uf/utils/mesh/mesh.h>


namespace pod {
	struct GlyphBox {
		struct {
			float x = 0, y = 0, w = 0, h = 0, z = 0;
		} box = {};
		
		pod::Vector4f color = { 1, 1, 1, 1 };
		pod::Vector3f anchor = { 0, 0, 0 };
		uint64_t code = 0;
	};
	struct TextToken {
		uf::stl::string text;
		pod::Vector4f color;
	};
	struct GlyphSettings {
		uf::stl::string alignment = "";
		uf::stl::string font = "Coolvetica.ttf";
		float size = 96;
		bool sdf = true;
		float spread = 8;
		pod::Vector2ui padding = { 8, 8 };
	};
}

namespace uf {
	namespace glyph {
		size_t UF_API hashSettings( uint64_t c, const pod::GlyphSettings& metadata );
		size_t UF_API hashSettings( const uf::stl::string& c, const pod::GlyphSettings& metadata );

		uf::stl::vector<pod::TextToken> UF_API parseTextTokens( const uf::stl::string& text, const pod::Vector4f& color = {1,1,1,1} );
		uf::stl::vector<pod::GlyphBox> UF_API calculateLayout( const uf::stl::vector<pod::TextToken>& tokens, const pod::GlyphSettings& metadata );
		bool UF_API generateAtlas( const uf::stl::vector<pod::GlyphBox>& layout, const pod::GlyphSettings& metadata, uf::Atlas& atlas );
		void UF_API generateMesh( const uf::stl::vector<pod::GlyphBox>& layout, const pod::GlyphSettings& metadata, const uf::Atlas& atlas, uf::Mesh& mesh );
	}
}