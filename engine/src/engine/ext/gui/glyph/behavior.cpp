#include "behavior.h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/image/atlas.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/window/window.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/text/glyph.h>
#include <uf/engine/asset/asset.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/memory/unordered_map.h>
#include <uf/utils/renderer/renderer.h>
#include <uf/ext/openvr/openvr.h>
#include <uf/utils/http/http.h>
#include <uf/utils/audio/audio.h>
#include <uf/utils/window/payloads.h>

#include "../payload.h"
#include "../behavior.h"
#include "../manager/behavior.h"

#if UF_USE_OPENGL
#define EXT_COLOR_FLOATS 0
#else
#define EXT_COLOR_FLOATS 1
#endif

namespace {
	struct Mesh {
		pod::Vector3f position;
		pod::Vector2f uv;

	#if EXT_COLOR_FLOATS
		pod::Vector4f color;
	#else
		pod::ColorRgba color;
	#endif

		static uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
	};
}

UF_VERTEX_DESCRIPTOR(Mesh,
	UF_VERTEX_DESCRIPTION(Mesh, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(Mesh, R32G32_SFLOAT, uv)
#if EXT_COLOR_FLOATS
	UF_VERTEX_DESCRIPTION(Mesh, R32G32B32A32_SFLOAT, color)
#else
	UF_VERTEX_DESCRIPTION(Mesh, R8G8B8A8_UNORM, color)
#endif
)

#include <regex>

#include <uf/utils/string/hash.h>

namespace {
	const uint64_t COLOR_CTRL = 0x7F;

	struct GlyphBox {
		struct {
			float x, y, w, h;
		} box;
		
		pod::Vector3f color;
		uint64_t code;
	};

	struct {
	#if UF_USE_FREETYPE
		ext::freetype::Glyph glyph;
		uf::stl::unordered_map<uf::stl::string, uf::stl::unordered_map<size_t, uf::Glyph>> cache;
	#else
		char glyph;
		uf::stl::unordered_map<uf::stl::string, uf::stl::unordered_map<size_t, char>> cache;
	#endif
	} glyphs;

	struct {
		uf::Serializer settings;
	#if UF_ENV_DREAMCAST
		pod::Vector2ui size = { 640, 480 };
	#else
		pod::Vector2ui size = { 1920, 1080 };
	#endif
	} defaults;
}

UF_BEHAVIOR_REGISTER_CPP(ext::GuiGlyphBehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::GuiGlyphBehavior, ticks = true, renders = false, thread = "")
#define this (&self)

namespace {
	size_t hashGlyphSettings( uint64_t c, const ext::GuiGlyphBehavior::Metadata& metadata ) {
		size_t seed{};
		uf::hash( seed, c, metadata.padding[0], metadata.padding[1], metadata.spread, metadata.size, metadata.font, metadata.sdf );
		return seed;
	}
	size_t hashGlyphSettings( const uf::stl::string& c, const ext::GuiGlyphBehavior::Metadata& metadata ) {
		size_t seed{};
		uf::hash( seed, c, metadata.padding[0], metadata.padding[1], metadata.spread, metadata.size, metadata.font, metadata.sdf );
		return seed;
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

	struct TextToken {
		uf::stl::string text;
		pod::Vector3f color;
	};

	float hexToFloat( const uf::stl::string& str ) {
		int value;
		uf::stl::stringstream stream;
		stream << str;
		stream >> std::hex >> value;
		return value / 255.0f;
	}

	// parses a text for special tokens, associating strings with color
	// should maybe be utf8, but in theory it shouldn't matter since tags are ASCII
	uf::stl::vector<TextToken> parseTextTokens( const uf::stl::string& text, pod::Vector3f color ) {
		uf::stl::vector<TextToken> tokens;

		auto tagLength = 10; // hard-coded cringe
		bool colorChanged = false;
		size_t currentPos = 0;
		size_t textLength = text.length();

		TextToken currentToken;
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

				// change color
				currentToken.color = { hexToFloat(rHex), hexToFloat(gHex), hexToFloat(bHex) };
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
	uf::stl::vector<GlyphBox> calculateGlyphLayout( uf::Object& self, const uf::stl::vector<TextToken>& tokens, const ext::GuiGlyphBehavior::Metadata& metadata, const ext::GuiBehavior::Metadata& metadataGui, const uf::stl::string& font ) {
		uf::stl::vector<GlyphBox> layout;
		auto& glyphsCache = ::glyphs.cache[font];

		if ( glyphsCache.empty() ) {
			ext::freetype::initialize( ::glyphs.glyph, font );
		}

		pod::Vector2f anchor = ::parseAnchor( metadataGui.alignment );
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

				auto key = hashGlyphSettings(c, metadata);
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
				auto key = hashGlyphSettings(c, metadata);
				auto& glyph = glyphsCache[key];
				auto& g = layout.emplace_back(GlyphBox{
					.box = {
						.x = cursor.x + glyph.getBearing().x,
						.y = cursor.y - glyph.getBearing().y,
						.w = glyph.getSize().x,
						.h = glyph.getSize().y,
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
	void generateAtlasAndMesh( const uf::stl::vector<GlyphBox>& layout, const ext::GuiGlyphBehavior::Metadata& metadata, const uf::stl::string& font, uf::Atlas& atlas, uf::Mesh& mesh ) {
		auto& glyphs_cache = ::glyphs.cache[font];
		uf::stl::unordered_map<size_t, uf::stl::string> glyph_atlas_map;

	#if UF_USE_FREETYPE
		// generate atlas
		{
			atlas.clear();

			// generate atlas
			for ( const auto& g : layout ) {
				auto key = hashGlyphSettings(g.code, metadata);
				auto& glyph = glyphs_cache[key];

				// already in atlas map
				if ( glyph_atlas_map.find(key) != glyph_atlas_map.end() ) continue;

				const uint8_t* buffer = glyph.getBuffer();
				uf::Image::container_t pixels;
				size_t len = glyph.getSize().x * glyph.getSize().y;

				if ( metadata.sdf ) {
					glyph_atlas_map[key] = atlas.addImage(glyph.getBuffer(), glyph.getSize(), 8, 1, true);
				} else {
					pixels.reserve(len * 4);
					for ( size_t i = 0; i < len; ++i ) {
						pixels.emplace_back(buffer[i]); // R
						pixels.emplace_back(buffer[i]); // G
						pixels.emplace_back(buffer[i]); // B
						pixels.emplace_back(buffer[i]); // A
					}
					glyph_atlas_map[key] = atlas.addImage(&pixels[0], glyph.getSize(), 8, 4, true);
				}
			}

			atlas.generate();
			atlas.clear(false);
		}
	#endif
		// generate mesh
		{
			mesh.destroy();
			mesh.bind<::Mesh, uint16_t>();

			uf::stl::vector<::Mesh> vertices;
			uf::stl::vector<uint16_t> indices;
			vertices.reserve(layout.size() * 4);
			indices.reserve(layout.size() * 6);

			for ( const auto& g : layout ) {
				auto key = hashGlyphSettings(g.code, metadata);
				auto hash = glyph_atlas_map[key];

			#if EXT_COLOR_FLOATS
				auto& color = g.color;
			#else
				pod::ColorRgba color = {
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
				vertices.emplace_back(::Mesh{pod::Vector3f{ g.box.x,		   g.box.y + g.box.h, 0 }, 	atlas.mapUv(pod::Vector2f{ 0.0f, 0.0f }, hash), color});
				vertices.emplace_back(::Mesh{pod::Vector3f{ g.box.x,		   g.box.y		  , 0 }, 	atlas.mapUv(pod::Vector2f{ 0.0f, 1.0f }, hash), color});
				vertices.emplace_back(::Mesh{pod::Vector3f{ g.box.x + g.box.w, g.box.y		  , 0 }, 	atlas.mapUv(pod::Vector2f{ 1.0f, 1.0f }, hash), color});
				vertices.emplace_back(::Mesh{pod::Vector3f{ g.box.x + g.box.w, g.box.y + g.box.h, 0 }, 	atlas.mapUv(pod::Vector2f{ 1.0f, 0.0f }, hash), color});
			}

			mesh.insertVertices(vertices);
			mesh.insertIndices(indices);
		}
	}
}

void ext::GuiGlyphBehavior::initialize( uf::Object& self ) {
	auto& metadata = this->getComponent<ext::GuiGlyphBehavior::Metadata>();
	auto& metadataGui = this->getComponent<ext::GuiBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();

	this->addHook( "gui:UpdateText.%UID%", [&](ext::json::Value& payload){
		auto string = payload["string"].as(metadata.string);
		auto font = uf::io::root+"/fonts/" + payload["font"].as(metadata.font);
		bool forced = payload["force"].as(false);

		// override
		// metadata.sdf = false;
		metadataGui.scaling = "none";

		auto& scene = uf::scene::getCurrentScene();
		auto& mesh = this->getComponent<uf::Mesh>();
		auto& atlas = this->getComponent<uf::Atlas>();
		auto& images = atlas.getImages();
		auto& glyphs_cache = ::glyphs.cache;

		uf::stl::unordered_map<size_t, uf::stl::string> glyph_atlas_map;

		
		auto tokens = ::parseTextTokens(string, metadataGui.color);
		auto layout = ::calculateGlyphLayout(self, tokens, metadata, metadataGui, font);
		::generateAtlasAndMesh( layout, metadata, font, atlas, mesh );

		// set proper shaders
		if ( metadata.sdf ) {
			metadataJson["shaders"]["vertex"] = uf::io::root+"/shaders/gui/text/vert.spv";
			metadataJson["shaders"]["fragment"] = uf::io::root+"/shaders/gui/text/frag.spv";
		}

		// fire image update
		{
			ext::payloads::GuiInitializationPayload payload;
			payload.image = &atlas.getAtlas();
			payload.mesh = &mesh;
			payload.free = false;
			this->callHook( "gui:Update.%UID%", payload );
		}

	});

	UF_BEHAVIOR_METADATA_BIND_SERIALIZER_HOOKS(metadata, metadataJson);
}
void ext::GuiGlyphBehavior::tick( uf::Object& self ) {
	if ( !this->hasComponent<uf::Graphic>() ) return;

#if !UF_USE_OPENGL

	auto& transform = this->getComponent<pod::Transform<>>();
	auto& metadata = this->getComponent<ext::GuiGlyphBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();
	auto& mesh = this->getComponent<uf::Mesh>();
	auto& graphic = this->getComponent<uf::Graphic>();
	auto model = uf::matrix::identity();
	
	auto& scene = uf::scene::getCurrentScene();
	auto& controller = scene.getController();
	auto& camera = controller.getComponent<uf::Camera>();

	// bind UBO
	if ( graphic.material.hasShader("vertex") ) {
		auto& shader = graphic.material.getShader("vertex");
		if ( shader.hasUniform("UBO_Glyph") ) {
			auto& uniformBuffer = shader.getUniformBuffer("UBO_Glyph");
			struct Glyph {
				/*alignas(16)*/ pod::Vector4f stroke;

				/*alignas(8)*/ pod::Vector2i range;
				/*alignas(4)*/ int32_t spread;
				/*alignas(4)*/ float weight;

				/*alignas(4)*/ float fillWeight;
				/*alignas(4)*/ float scale;
				/*alignas(4)*/ float padding1;
				/*alignas(4)*/ float padding2;
			} ubo = {
				.stroke = metadata.shader.stroke,
				.range = metadata.shader.range,
				
				.spread = metadata.spread,
				.weight = metadata.shader.weight,
				.fillWeight = metadata.shader.fillWeight,
				.scale = metadata.shader.scale,
			};

			shader.updateBuffer( (const void*) &ubo, sizeof(ubo), uniformBuffer );
		}
	}
#endif
}
void ext::GuiGlyphBehavior::render( uf::Object& self ){}
void ext::GuiGlyphBehavior::destroy( uf::Object& self ){}

#undef this

void ext::GuiGlyphBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ){
	serializer["string"] = /*this->*/string;
	serializer["font"] = /*this->*/font;
	serializer["sdf"] = /*this->*/sdf;
	serializer["spread"] = /*this->*/spread;
	serializer["padding"] = uf::vector::encode( /*this->*/padding);

	serializer["scale"] = /*this->*/shader.scale;
	serializer["weight"] = /*this->*/shader.weight;
	serializer["fillWeight"] = /*this->*/shader.fillWeight;
	serializer["stroke"] = uf::vector::encode( /*this->*/shader.stroke);
	serializer["range"] = uf::vector::encode( /*this->*/shader.range);
}
void ext::GuiGlyphBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ){
	size_t oldHash = ::hashGlyphSettings( string, *this );

	/*this->*/string = serializer["string"].as(/*this->*/string);
	/*this->*/font = serializer["font"].as(/*this->*/font);
	/*this->*/sdf = serializer["sdf"].as(/*this->*/sdf);
	/*this->*/spread = serializer["spread"].as(/*this->*/spread);
	/*this->*/padding = uf::vector::decode(serializer["padding"], /*this->*/padding);
	
	/*this->*/shader.scale = serializer["scale"].as(/*this->*/shader.scale);
	/*this->*/shader.weight = serializer["weight"].as(/*this->*/shader.weight);
	/*this->*/shader.fillWeight = serializer["fillWeight"].as(/*this->*/shader.fillWeight);
	/*this->*/shader.stroke = uf::vector::decode(serializer["stroke"], /*this->*/shader.stroke);
	/*this->*/shader.range = uf::vector::decode(serializer["range"], /*this->*/shader.range);

	size_t newHash = ::hashGlyphSettings( string, *this );
	
	// fire text update
	if ( oldHash != newHash ) {
		ext::json::Value payload;
		payload["string"] = /*this->*/string;
		self.callHook("gui:UpdateText.%UID%", payload);
	}
}