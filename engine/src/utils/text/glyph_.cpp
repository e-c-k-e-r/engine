#include <uf/utils/text/glyph.h>
#include <uf/utils/text/glyph_.h>

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
		pod::Vector4b color;
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
)

namespace {
	struct {
	#if UF_USE_FREETYPE
		ext::freetype::Glyph glyph;
		uf::stl::unordered_map<uf::stl::string, uf::stl::unordered_map<size_t, uf::Glyph>> cache;
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
	uf::hash( seed, c, metadata.padding[0], metadata.padding[1], metadata.spread, metadata.size, metadata.font, metadata.sdf );
	return seed;
}
size_t uf::glyph::hashSettings( const uf::stl::string& c, const pod::GlyphSettings& metadata ) {
	size_t seed{};
	uf::hash( seed, c, metadata.padding[0], metadata.padding[1], metadata.spread, metadata.size, metadata.font, metadata.sdf );
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
	auto& glyphsCache = ::glyphs.cache[metadata.font];

	if ( glyphsCache.empty() ) {
		ext::freetype::initialize( ::glyphs.glyph, uf::io::root + "/fonts/" + metadata.font );
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
		std::u8string str(token.text.begin(), token.text.end());
		for ( uint64_t c : str ) {
			if ( c == '\n' || c == '\t' ) continue; // special characters

			auto key = uf::glyph::hashSettings(c, metadata);
			auto& glyph = glyphsCache[key];

			// generate glyph
			if ( !glyph.generated() ) {
				glyph.setPadding({ metadata.padding[0], metadata.padding[1] });
				glyph.setSpread(metadata.spread);
				glyph.useSdf(metadata.sdf);
				glyph.generate(::glyphs.glyph, c, metadata.size);
			}

			tallestGlyphY = std::max(tallestGlyphY, (float) glyph.getSize().y);
			totalWidth += glyph.getSize().x; // should probably be reset on new-line to find the widest line
			charCount++;
		}
	}

	if ( charCount > 0 ) averageTabWidth = (totalWidth / charCount) * 4.0f;
	cursor.y = tallestGlyphY;

	// calculate positions
	for ( const auto& token : tokens ) {
		std::u8string str( token.text.begin(), token.text.end() );
		for ( uint64_t c : str ) {
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
			auto& glyph = glyphsCache[key];
			auto& g = layout.emplace_back(pod::GlyphBox{
				.box = {
					.x = cursor.x + glyph.getBearing().x,
					.y = cursor.y - glyph.getBearing().y,
					.w = glyph.getSize().x,
					.h = glyph.getSize().y,
					.z = 0,
				},
				.color = token.color,
				.code = c,
			});

			// advance cursor
			cursor.x += glyph.getAdvance().x;

			// advance bounding box
			maxTextWidth = std::max(maxTextWidth, g.box.x + g.box.w);
			maxTextHeight = std::max(maxTextHeight, g.box.y + g.box.h);
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
		g.box.x /= ::defaults.size.x;
		g.box.w /= ::defaults.size.x;
		g.box.y /= ::defaults.size.y;
		g.box.h /= ::defaults.size.y;
	}

	return layout;
}

// generate the mesh and texture atlas
bool uf::glyph::generateAtlas( const uf::stl::vector<pod::GlyphBox>& layout, const pod::GlyphSettings& metadata, uf::Atlas& atlas ) {
	bool dirty = false;
	auto& cache = ::glyphs.cache[metadata.font];

#if UF_USE_FREETYPE
	// generate atlas
	for ( const auto& g : layout ) {
		auto key = uf::glyph::hashSettings( g.code, metadata );
		auto hash = std::to_string( key );
		auto& glyph = cache[key];

		// already in atlas map
		if ( atlas.has( hash ) ) continue;
		dirty = true;

		uf::Image image;
		const uint8_t* buffer = glyph.getBuffer();

		if ( metadata.sdf ) {
			image.loadFromBuffer( glyph.getBuffer(), glyph.getSize(), 8, 1, true );
		} else {
			uf::Image::container_t pixels;
			size_t len = glyph.getSize().x * glyph.getSize().y;
			pixels.resize(len * 4);
			for ( auto i = 0; i < len; ++i ) {
				pixels[i * 4 + 0] = buffer[i]; // R
				pixels[i * 4 + 1] = buffer[i]; // G
				pixels[i * 4 + 2] = buffer[i]; // B
				pixels[i * 4 + 3] = buffer[i]; // A
			}
			image.loadFromBuffer( &pixels[0], glyph.getSize(), 8, 4, true );
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
		auto hash = std::to_string( uf::glyph::hashSettings(g.code, metadata) );

		// zero-width
		if ( g.box.w == 0 || g.box.h == 0 || g.color.w == 0.0f ) continue;

	#if EXT_COLOR_FLOATS
		auto& color = g.color;
	#else
		pod::Vector4b color = {
			(uint8_t)(g.color[0] * 255),
			(uint8_t)(g.color[1] * 255),
			(uint8_t)(g.color[2] * 255),
			(uint8_t)(g.color[3] * 255)
		};
	#endif
		// insert indices
		uint16_t idx = (uint16_t) vertices.size();
		indices.insert( indices.end(), { idx, idx + 1, idx + 2, idx, idx + 2, idx + 3 });

		// insert vertices
		vertices.emplace_back(::GlyphVertex{pod::Vector3f{ g.box.x,	g.box.y + g.box.h, g.box.z }, 			atlas.mapUv(pod::Vector2f{ 0.0f, 0.0f }, hash), color});
		vertices.emplace_back(::GlyphVertex{pod::Vector3f{ g.box.x, g.box.y, g.box.z }, 					atlas.mapUv(pod::Vector2f{ 0.0f, 1.0f }, hash), color});
		vertices.emplace_back(::GlyphVertex{pod::Vector3f{ g.box.x + g.box.w, g.box.y, g.box.z }, 			atlas.mapUv(pod::Vector2f{ 1.0f, 1.0f }, hash), color});
		vertices.emplace_back(::GlyphVertex{pod::Vector3f{ g.box.x + g.box.w, g.box.y + g.box.h, g.box.z }, atlas.mapUv(pod::Vector2f{ 1.0f, 0.0f }, hash), color});
	}

	mesh.insertVertices(vertices);
	mesh.insertIndices(indices);
}