#include <uf/utils/text/glyph.h>
#include <uf/utils/text/graphic.h>

#if UF_USE_OPENGL
#define EXT_COLOR_FLOATS 0
#else
#define EXT_COLOR_FLOATS 1
#endif

namespace {
	struct {
	#if UF_ENV_DREAMCAST
		pod::Vector2ui size = { 640, 480 };
	#else
		pod::Vector2ui size = { 1920, 1080 };
	#endif
	} defaults;
}

namespace {
	struct GlyphVertex {
		pod::Vector3f position;
		pod::Vector2f uv;

	#if EXT_COLOR_FLOATS
		pod::Vector4f color;
	#else
		pod::Vector4ub color;
	#endif

	#if !UF_USE_OPENGL
		pod::Vector3f offset;
	#endif

		static uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
	};
}

UF_VERTEX_DESCRIPTOR(GlyphVertex,
	UF_VERTEX_DESCRIPTION(GlyphVertex, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(GlyphVertex, R32G32_SFLOAT, uv)
#if EXT_COLOR_FLOATS
	UF_VERTEX_DESCRIPTION(GlyphVertex, R32G32B32A32_SFLOAT, color)
#else
	UF_VERTEX_DESCRIPTION(GlyphVertex, R8G8B8A8_UNORM, color)
#endif
#if !UF_USE_OPENGL
	UF_VERTEX_DESCRIPTION(GlyphVertex, R32G32_SFLOAT, offset)
#endif
)

namespace {
	struct {
	#if UF_USE_TRUETYPE
		pod::TrueTypeFont face;
		uf::stl::unordered_map<uf::stl::string, uf::stl::unordered_map<size_t, pod::Glyph>> cache;
	#else
		char glyph;
		uf::stl::unordered_map<uf::stl::string, uf::stl::unordered_map<size_t, char>> cache;
	#endif
	} glyphs;

	float hexToFloat( const uf::stl::string& str ) {
		int value;
		uf::stl::stringstream stream;
		stream << str;
		stream >> std::hex >> value;
		return value / 255.0f;
	}

	// default to left
	pod::Vector2f parseAnchor( const uf::stl::string& anchor, const pod::Vector2f& def = {0.0f, 0.0f} ) {
		if ( anchor == "top-center" || anchor == "top" ) return {0.5f, 0.0f};
		if ( anchor == "top-left" ) return {0.0f, 0.0f};
		if ( anchor == "top-right" ) return {1.0f, 0.0f};

		if ( anchor == "center" ) return {0.5f, 0.5f};
		if ( anchor == "center-left" || anchor == "left" ) return {0.0f, 0.5f};
		if ( anchor == "center-right" || anchor == "right" ) return {1.0f, 0.5f};

		if ( anchor == "bottom-center" || anchor == "bottom" ) return {0.5f, 1.0f};
		if ( anchor == "bottom-left" ) return {0.0f, 1.0f};
		if ( anchor == "bottom-right" ) return {1.0f, 1.0f};

		return def;
	}
}

size_t uf::glyph::hashSettings( uint64_t c, const pod::GlyphSettings& metadata ) {
	size_t seed{};
	uf::hash( seed, c, metadata.padding[0], metadata.padding[1], metadata.spread, metadata.size, metadata.font );
	return seed;
}
size_t uf::glyph::hashSettings( const uf::stl::string& c, const pod::GlyphSettings& metadata ) {
	size_t seed{};
	uf::hash( seed, c, metadata.padding[0], metadata.padding[1], metadata.spread, metadata.size, metadata.font );
	return seed;
}


// parses a text for special tokens, associating strings with color
// should maybe be utf8, but in theory it shouldn't matter since tags are ASCII
uf::stl::vector<pod::TextToken> uf::glyph::parseTextTokens( const uf::stl::string& text, const pod::Vector4f& color ) {
	uf::stl::vector<pod::TextToken> tokens;

	auto tagLength = 12; // hard-coded cringe
	bool colorChanged = false;
	size_t currentPos = 0;
	size_t textLength = text.length();

	pod::TextToken currentToken;
	currentToken.color = color;

	while ( currentPos < textLength ) {
		size_t tagStart = text.find("${#", currentPos);

		if ( tagStart == uf::stl::string::npos ) {
			currentToken.text += text.substr(currentPos);
			if ( !currentToken.text.empty() || colorChanged ) {
				tokens.emplace_back(currentToken);
			}
			break;
		}

		if ( tagStart > currentPos ) {
			currentToken.text += text.substr( currentPos, tagStart - currentPos ); // append everything up to the tag
			tokens.emplace_back( currentToken ); // commit token
			currentToken.text = ""; // reset text
		}

		// validate tag
		if ( tagStart + tagLength <= textLength && text[tagStart + tagLength - 1] == '}' ) {
			uf::stl::string rHex = text.substr(tagStart + 3, 2);
			uf::stl::string gHex = text.substr(tagStart + 5, 2);
			uf::stl::string bHex = text.substr(tagStart + 7, 2);
			uf::stl::string aHex = text.substr(tagStart + 9, 2);

			// change color
			currentToken.color = { hexToFloat(rHex), hexToFloat(gHex), hexToFloat(bHex), hexToFloat(aHex) };
			colorChanged = true;

			// advance past the tag
			currentPos = tagStart + tagLength;
		} else {
			currentToken.text += "${#"; // treat it as normal text
			currentPos = tagStart + 3;
		}
	}

	return tokens;
}

// compute the boxes for a given string and settings
uf::stl::vector<pod::GlyphBox> uf::glyph::calculateLayout( const uf::stl::vector<pod::TextToken>& tokens, const pod::GlyphSettings& metadata ) {
	uf::stl::vector<pod::GlyphBox> layout;
	auto& cache = ::glyphs.cache[metadata.font];

	if ( cache.empty() ) {
		ext::truetype::initialize( ::glyphs.face, uf::io::root + "/fonts/" + metadata.font );
	}

	pod::Vector2f anchor = ::parseAnchor( metadata.alignment );
	pod::Vector2f cursor = { 0.0f, 0.0f };
	float maxTextWidth = 0.0f;
	float maxTextHeight = 0.0f;
	float tallestGlyphY = 0.0f;
	float averageTabWidth = 0.0f;
	float totalWidth = 0.0f;
	int charCount = 0;

	// generate glyph and line height
	for ( const auto& token : tokens ) {
		for ( char raw_c : token.text ) {
			uint64_t c = (uint8_t)(raw_c); // Safely cast byte
			if ( c == '\n' || c == '\t' ) continue;

			auto key = uf::glyph::hashSettings(c, metadata);
			auto& glyph = cache[key];

			// generate glyph
			if ( glyph.buffer.empty() ) {
				glyph.padding = { metadata.padding[0], metadata.padding[1] };
				glyph.spread = metadata.spread;
				uf::glyph::generate( glyph, ::glyphs.face, c, metadata.size );
			}

			float g_y = (float)(glyph.size.y);
			if (g_y > tallestGlyphY) tallestGlyphY = g_y;

			totalWidth += (float)(glyph.size.x);
			charCount++;
		}
	}

	if ( charCount > 0 ) averageTabWidth = (totalWidth / (float)(charCount)) * 4.0f;
	cursor.y = tallestGlyphY;

	// calculate positions
	for ( const auto& token : tokens ) {
		for ( char raw_c : token.text ) {
			uint64_t c = (uint8_t)(raw_c);

			// advance cursor on special characters
			if ( c == '\n' ) {
				cursor.y += tallestGlyphY;
				cursor.x = 0;
				continue;
			} else if ( c == '\t' ) {
				cursor.x = ((int)(cursor.x / averageTabWidth) + 1) * averageTabWidth;
				continue;
			} else if ( c == ' ' ) {
				cursor.x += averageTabWidth / 4.0f;
				continue;
			}

			// retrieve glyph
			auto key = uf::glyph::hashSettings(c, metadata);
			auto& glyph = cache[key];

			pod::GlyphBox newBox;
			newBox.box.x = cursor.x + (float)(glyph.bearing.x);
			newBox.box.y = cursor.y - (float)(glyph.bearing.y);
			newBox.box.w = (float)(glyph.size.x);
			newBox.box.h = (float)(glyph.size.y);
			newBox.box.z = 0.0f;
			newBox.color = token.color;
			newBox.anchor = anchor;
			newBox.code = c;

			layout.emplace_back(newBox);
			pod::GlyphBox& g = layout.back();

			// advance cursor
			cursor.x += (float)(glyph.advance.x);

			// advance bounding box manually
			float right = g.box.x + g.box.w;
			if (right > maxTextWidth) maxTextWidth = right;

			float bottom = g.box.y + g.box.h;
			if (bottom > maxTextHeight) maxTextHeight = bottom;
		}
	}

	// calculate offset based on anchor
	float offsetX = maxTextWidth * anchor.x;
	float offsetY = maxTextHeight * anchor.y;

	// adjust all glyphs for our offset
	for ( auto& g : layout ) {
		g.box.x -= offsetX;
		g.box.y -= offsetY;

		// normalize
		g.box.x /= (float)(::defaults.size.x);
		g.box.w /= (float)(::defaults.size.x);
		g.box.y /= (float)(::defaults.size.y);
		g.box.h /= (float)(::defaults.size.y);
	}

	return layout;
}

// generate the mesh and texture atlas
bool uf::glyph::generateAtlas( const uf::stl::vector<pod::GlyphBox>& layout, const pod::GlyphSettings& metadata, uf::Atlas& atlas ) {
	bool dirty = false;
	auto& cache = ::glyphs.cache[metadata.font];

#if UF_USE_TRUETYPE
	// generate atlas
	for ( const auto& g : layout ) {
		auto key = uf::glyph::hashSettings( g.code, metadata );
		auto hash = FMT_FORMAT( "{}", key );
		auto& glyph = cache[key];

		// already in atlas map
		if ( atlas.has( hash ) ) continue;
		dirty = true;

		uf::Image image;
		if ( metadata.spread > 0 ) {
			image.loadFromBuffer( glyph.buffer.data(), glyph.size, 8, 1 );
		} else {
			const uint8_t* buffer = glyph.buffer.data();
			pod::Image::container_t pixels;
			size_t len = glyph.size.x * glyph.size.y;
			pixels.resize(len * 4);
			for ( auto i = 0; i < len; ++i ) {
				pixels[i * 4 + 0] = buffer[i]; // R
				pixels[i * 4 + 1] = buffer[i]; // G
				pixels[i * 4 + 2] = buffer[i]; // B
				pixels[i * 4 + 3] = buffer[i]; // A
			}
			image.loadFromBuffer( &pixels[0], glyph.size, 8, 4 );
		}
		atlas.addImage( image, hash );
	}
	atlas.generate();
#endif
	return dirty;
}
void uf::glyph::generateMesh( const uf::stl::vector<pod::GlyphBox>& layout, const pod::GlyphSettings& metadata, const uf::Atlas& atlas, uf::Mesh& mesh ) {
	// generate mesh
	mesh.clear();
	mesh.bind<::GlyphVertex, uint16_t>();

	uf::stl::vector<::GlyphVertex> vertices;
	uf::stl::vector<uint16_t> indices;
	vertices.reserve(layout.size() * 4);
	indices.reserve(layout.size() * 6);

	for ( const auto& g : layout ) {
		auto hash = FMT_FORMAT( "{}", uf::glyph::hashSettings(g.code, metadata) );

		auto& anchor = g.anchor;
	#if EXT_COLOR_FLOATS
		auto& color = g.color;
	#else
		pod::Vector4ub color = uf::vector::clamp( g.color, 0, 1 ) * 255;
	#endif

	/*
		pod::Vector3f anchor = { g.anchor[0], g.anchor[1], 0.0f };
	#if EXT_COLOR_FLOATS
		auto& color = g.color;
	#else
		pod::Vector4ub color = {
			(uint8_t)(CLAMP(g.color[0], 0, 1) * 255),
			(uint8_t)(CLAMP(g.color[1], 0, 1) * 255),
			(uint8_t)(CLAMP(g.color[2], 0, 1) * 255),
			(uint8_t)(CLAMP(g.color[3], 0, 1) * 255)
		};
	#endif
	*/
		// insert indices
		uint16_t idx = (uint16_t) vertices.size();
		indices.insert( indices.end(), { idx, idx + 1, idx + 2, idx, idx + 2, idx + 3 });

		// insert vertices
		auto p0 = pod::Vector2f{ g.box.x, g.box.y + g.box.h };
		auto p1 = pod::Vector2f{ g.box.x, g.box.y };
		auto p2 = pod::Vector2f{ g.box.x + g.box.w, g.box.y };
		auto p3 = pod::Vector2f{ g.box.x + g.box.w, g.box.y + g.box.h };

	#if UF_USE_OPENGL
		vertices.emplace_back(::GlyphVertex{anchor + p0, atlas.mapUv(pod::Vector2f{ 0.0f, 1.0f }, hash), color});
		vertices.emplace_back(::GlyphVertex{anchor + p1, atlas.mapUv(pod::Vector2f{ 0.0f, 0.0f }, hash), color});
		vertices.emplace_back(::GlyphVertex{anchor + p2, atlas.mapUv(pod::Vector2f{ 1.0f, 0.0f }, hash), color});
		vertices.emplace_back(::GlyphVertex{anchor + p3, atlas.mapUv(pod::Vector2f{ 1.0f, 1.0f }, hash), color});
	#else
		vertices.emplace_back(::GlyphVertex{anchor, atlas.mapUv(pod::Vector2f{ 0.0f, 1.0f }, hash), color, p0});
		vertices.emplace_back(::GlyphVertex{anchor, atlas.mapUv(pod::Vector2f{ 0.0f, 0.0f }, hash), color, p1});
		vertices.emplace_back(::GlyphVertex{anchor, atlas.mapUv(pod::Vector2f{ 1.0f, 0.0f }, hash), color, p2});
		vertices.emplace_back(::GlyphVertex{anchor, atlas.mapUv(pod::Vector2f{ 1.0f, 1.0f }, hash), color, p3});
	#endif
	}

	mesh.insertVertices(vertices);
	mesh.insertIndices(indices);
}