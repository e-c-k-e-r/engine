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
		pod::Vector2ui size = { 1920, 1080 };
	} defaults;
}

UF_BEHAVIOR_REGISTER_CPP(ext::GuiGlyphBehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::GuiGlyphBehavior, ticks = true, renders = false, multithread = false)
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

	uf::stl::vector<::GlyphBox> generateGlyphs( uf::Object& self, const uf::stl::string& _string ) {
		uf::stl::vector<::GlyphBox> gs;
	
	#if UF_USE_FREETYPE
		auto& glyphs = ::glyphs;
		auto& metadata = this->getComponent<ext::GuiGlyphBehavior::Metadata>();
		auto& metadataGui = this->getComponent<ext::GuiBehavior::Metadata>();
		auto& transform = this->getComponent<pod::Transform<>>();


		auto string = _string == "" ? metadata.string : _string;
		auto font = uf::io::root+"/fonts/" + metadata.font;
		auto color = metadataGui.color;

		auto origin = uf::stl::string("top");
		auto align = uf::stl::string("center");
		auto direction = uf::stl::string("down");

		if ( glyphs.cache[font].empty() ) ext::freetype::initialize( glyphs.glyph, font );

		struct {
			struct {
				float x = 0;
				float y = 0;
			} origin;
			
			struct {
				float x = 0;
				float y = 0;
			} cursor;

			struct {
				float sum = 0;
				float len = 0;
				float proc = 0;
				float tab = 4;
			} average;
			
			struct {
				float x = 0;
				float y = 0;
			} biggest;

			struct {
				float w = 0;
				float h = 0;
			} box;

			struct {
				uf::stl::vector<pod::Vector3f> container;
				size_t index = 0;
			} colors;
		} stat;

		stat.colors.container.push_back(color);

		// grab escaped color markers: ${#RRGGBB}
		{
			uf::stl::unordered_map<size_t, pod::Vector3f> colors;
			uf::stl::string text = string;

			std::regex regex("\\$\\{\\#([0-9A-Fa-f]{2})([0-9A-Fa-f]{2})([0-9A-Fa-f]{2})\\}");
			std::smatch match;

			bool matched = false;

			int maxTries = 128;
			while ( (matched = std::regex_search( text, match, regex )) && --maxTries > 0 ) {
				struct {
					uf::stl::string str;
					int dec;
				} r, g, b;
				r.str = match[1].str();
				g.str = match[2].str();
				b.str = match[3].str();
				
				{ uf::stl::stringstream stream; stream << r.str; stream >> std::hex >> r.dec; } 
				{ uf::stl::stringstream stream; stream << g.str; stream >> std::hex >> g.dec; } 
				{ uf::stl::stringstream stream; stream << b.str; stream >> std::hex >> b.dec; } 

				pod::Vector3f color = { r.dec / 255.0f, g.dec / 255.0f, b.dec / 255.0f };

				stat.colors.container.push_back(color);
				text = uf::string::replace( text, "${" + r.str + g.str + b.str + "}", "\x7F" );
			}
			if ( maxTries == 0 ) text += "\n(error formatting)";
			string = text;
		}

		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
		std::wstring str = convert.from_bytes(string);
		if ( str.size() == 0 ) return gs;

		// Calculate statistics
		{
			// Find tallest glyph for new line
			for ( auto it = str.begin(); it != str.end(); ++it ) {
				uint64_t c = *it; if ( c == '\n' ) continue; if ( c == '\t' ) continue; if ( c == 0x01 ) continue;
				auto key = hashGlyphSettings( c, metadata );
				auto& glyph = glyphs.cache[font][key];

				if ( !glyph.generated() ) {
					glyph.setPadding( { metadata.padding[0], metadata.padding[1] } );
					glyph.setSpread( metadata.spread );
				#if UF_USE_VULKAN
					if ( metadata.sdf ) glyph.useSdf(true);
				#else
					glyph.useSdf(false);
				#endif
					glyph.generate( glyphs.glyph, c, metadata.size );
				}

				
				stat.biggest.x = std::max( (float) stat.biggest.x, (float) glyph.getSize().x);
				stat.biggest.y = std::max( (float) stat.biggest.y, (float) glyph.getSize().y);

				stat.average.sum += glyph.getSize().x;
				++stat.average.len;
			}
			stat.average.proc = stat.average.sum / stat.average.len;
			stat.average.tab *= stat.average.proc;

			// Calculate box: Second pass required because of tab
			stat.cursor.x = 0;
			stat.cursor.y = 0;
			stat.origin.x = 0;
			stat.origin.y = 0;

			for ( auto it = str.begin(); it != str.end(); ++it ) {
				uint64_t c = *it; if ( c == '\n' ) {
					stat.cursor.y += stat.biggest.y;
					stat.cursor.x = 0;
					continue;
				} else if ( c == '\t' ) {
					// Fixed movement vs Real Tabbing
					if ( false ) {
						stat.cursor.x += stat.average.tab;
					} else {
						stat.cursor.x = ((stat.cursor.x / stat.average.tab) + 1) * stat.average.tab;
					}
					continue;
				} else if ( c == COLOR_CTRL ) {
					continue;
				}
				auto key = hashGlyphSettings( c, metadata );
				auto& glyph = glyphs.cache[font][key];
				::GlyphBox g;

				g.box.w = glyph.getSize().x;
				g.box.h = glyph.getSize().y;
					
				g.box.x = stat.cursor.x + glyph.getBearing().x;
				g.box.y = stat.cursor.y - glyph.getBearing().y; // - (glyph.getSize().y - glyph.getBearing().y);

				stat.cursor.x += (glyph.getAdvance().x);
				
			}

			stat.origin.x = transform.position.x * ::defaults.size.x; // ( !metadataGui.world && transform.position.x != (int) transform.position.x ) ? transform.position.x * ::defaults.size.x : transform.position.x;
			stat.origin.y = transform.position.y * ::defaults.size.y; // ( !metadataGui.world && transform.position.y != (int) transform.position.y ) ? transform.position.y * ::defaults.size.y : transform.position.y;

			if ( origin == "top" ) stat.origin.y = ::defaults.size.y - stat.origin.y - stat.biggest.y;// else stat.origin.y = stat.origin.y;
			if ( align == "right" ) stat.origin.x = ::defaults.size.x - stat.origin.x - stat.box.w;// else stat.origin.x = stat.origin.x;
			else if ( align == "center" )
				stat.origin.x -= stat.box.w * 0.5f;
		}

		// Render Glyphs
		stat.cursor.x = 0;
		stat.cursor.y = stat.biggest.y;
		for ( auto it = str.begin(); it != str.end(); ++it ) {
			uint64_t c = *it; if ( c == '\n' ) {
				if ( direction == "down" ) stat.cursor.y += stat.biggest.y; else stat.cursor.y -= stat.biggest.y;
				stat.cursor.x = 0;
				continue;
			} else if ( c == '\t' ) {
				// Fixed movement vs Real Tabbing
				if ( false ) {
					stat.cursor.x += stat.average.tab;
				} else {
					stat.cursor.x = ((stat.cursor.x / stat.average.tab) + 1) * stat.average.tab;
				}
				continue;
			} else if ( c == ' ' ) {
				stat.cursor.x += stat.average.tab / 4.0f;
				continue;
			} else if ( c == COLOR_CTRL ) {
				++stat.colors.index;
				continue;
			}
			auto key = hashGlyphSettings( c, metadata );
			auto& glyph = glyphs.cache[font][key];

			::GlyphBox g;
			g.code = c;

			g.box.w = glyph.getSize().x;
			g.box.h = glyph.getSize().y;
				
			g.box.x = stat.cursor.x + (glyph.getBearing().x);
			g.box.y = stat.cursor.y - glyph.getBearing().y; // - (glyph.getSize().y - glyph.getBearing().y);

			stat.cursor.x += (glyph.getAdvance().x);

			if ( stat.colors.index < stat.colors.container.size() ) {
				g.color = stat.colors.container.at(stat.colors.index);
			} else {
				g.color = metadataGui.color;
			}

			g.box.x /= ::defaults.size.x;
			g.box.w /= ::defaults.size.x;
			g.box.y /= ::defaults.size.y;
			g.box.h /= ::defaults.size.y;


			gs.push_back(g);
		}
	#endif

		return gs;
	}
}

void ext::GuiGlyphBehavior::initialize( uf::Object& self ) {
	auto& metadata = this->getComponent<ext::GuiGlyphBehavior::Metadata>();
	auto& metadataGui = this->getComponent<ext::GuiBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();

/*
	if ( ext::json::isNull( ::defaults.settings["metadata"] ) ) ::defaults.settings.readFromFile(uf::io::root+"/entities/gui/text/string.json");
	// set defaults
	if ( metadataJson["string"].is<uf::stl::string>() ) {
		auto copyMetadataJson = metadataJson;
		ext::json::forEach(::defaults.settings["metadata"], [&]( const uf::stl::string& key, ext::json::Value& value ){
			if ( ext::json::isNull( copyMetadataJson[key] ) ) {
				metadataJson[key] = value;
			}
		});
	}
*/

//	if ( metadataJson["scaling"] == "auto" ) metadataJson["scaling"] = "none";

	this->addHook( "gui:UpdateText.%UID%", [&](ext::json::Value& payload){
		auto string = payload["string"].as(metadata.string);
		auto font = uf::io::root+"/fonts/" + payload["font"].as(metadata.font);
		bool forced = payload["force"].as(false);

		auto glyphs = ::generateGlyphs( self, string );

		auto& scene = uf::scene::getCurrentScene();
		auto& mesh = this->getComponent<uf::Mesh>();
		auto& atlas = this->getComponent<uf::Atlas>();
		auto& images = atlas.getImages();
		auto& glyphs_cache = ::glyphs.cache;

		uf::stl::unordered_map<size_t, uf::stl::string> glyph_atlas_map;

		metadataGui.scaleMode = "none";

		// generate texture
		#if UF_USE_FREETYPE
		{
			atlas.clear();

			for ( auto& g : glyphs ) {
				auto key = ::hashGlyphSettings( g.code, metadata );
				auto& glyph = glyphs_cache[font][key];

				const uint8_t* buffer = glyph.getBuffer();
				uf::Image::container_t pixels;
				size_t len = glyph.getSize().x * glyph.getSize().y;
			
				if ( metadata.sdf ) {
					glyph_atlas_map[key] = atlas.addImage( glyph.getBuffer(), glyph.getSize(), 8, 1, true );
				} else {
				#if 1 || UF_ENV_DREAMCAST
					pixels.reserve( len * 4 );
					for ( size_t i = 0; i < len; ++i ) {
						pixels.emplace_back( buffer[i] );
						pixels.emplace_back( buffer[i] );
						pixels.emplace_back( buffer[i] );
						pixels.emplace_back( buffer[i] );
					//	pixels.emplace_back( buffer[i] > 127 ? 255 : 0 );
					}
					glyph_atlas_map[key] = atlas.addImage( &pixels[0], glyph.getSize(), 8, 4, true );
				#else
					pixels.reserve( len * 2 );
					for ( size_t i = 0; i < len; ++i ) {
						pixels.emplace_back( buffer[i] );
						pixels.emplace_back( buffer[i] );
					//	pixels.emplace_back( buffer[i] > 127 ? 255 : 0 );
					}
					glyph_atlas_map[key] = atlas.addImage( &pixels[0], glyph.getSize(), 8, 2, true );
				#endif
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

			uf::stl::vector<::Mesh> vertices; vertices.reserve( glyphs.size() * 6 );
			uf::stl::vector<uint16_t> indices; indices.reserve( glyphs.size() * 6 );


		//	pod::Vector2f min = { 0, 0 };
		//	pod::Vector2f max = { 0, 0 };

			for ( auto& g : glyphs ) {
				auto key = ::hashGlyphSettings( g.code, metadata );
				auto& glyph = glyphs_cache[font][key];
				auto hash = glyph_atlas_map[key];

			#if EXT_COLOR_FLOATS
				auto& color = g.color;
			#else
				pod::ColorRgba color = { g.color[0] * 255, g.color[1] * 255, g.color[2] * 255, g.color[3] * 255 }; 
			#endif
				// add vertices
				vertices.emplace_back( ::Mesh{pod::Vector3f{ g.box.x,           g.box.y + g.box.h, 0 }, atlas.mapUv( pod::Vector2f{ 0.0f, 0.0f }, hash ), color}); indices.emplace_back( indices.size() );
				vertices.emplace_back( ::Mesh{pod::Vector3f{ g.box.x,           g.box.y          , 0 }, atlas.mapUv( pod::Vector2f{ 0.0f, 1.0f }, hash ), color}); indices.emplace_back( indices.size() );
				vertices.emplace_back( ::Mesh{pod::Vector3f{ g.box.x + g.box.w, g.box.y          , 0 }, atlas.mapUv( pod::Vector2f{ 1.0f, 1.0f }, hash ), color}); indices.emplace_back( indices.size() );
				vertices.emplace_back( ::Mesh{pod::Vector3f{ g.box.x,           g.box.y + g.box.h, 0 }, atlas.mapUv( pod::Vector2f{ 0.0f, 0.0f }, hash ), color}); indices.emplace_back( indices.size() );
				vertices.emplace_back( ::Mesh{pod::Vector3f{ g.box.x + g.box.w, g.box.y          , 0 }, atlas.mapUv( pod::Vector2f{ 1.0f, 1.0f }, hash ), color}); indices.emplace_back( indices.size() );
				vertices.emplace_back( ::Mesh{pod::Vector3f{ g.box.x + g.box.w, g.box.y + g.box.h, 0 }, atlas.mapUv( pod::Vector2f{ 1.0f, 0.0f }, hash ), color}); indices.emplace_back( indices.size() );
			}

		/*
			pod::Vector2f min = {  std::numeric_limits<float>::max(),  std::numeric_limits<float>::max() };
			pod::Vector2f max = { -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max() };

			for ( auto& vertex : vertices ) {
				min.x = std::min( min.x, vertex.position.x * ::defaults.size.x * transform.scale.x );
				min.y = std::min( min.y, vertex.position.y * ::defaults.size.y * transform.scale.y );
				max.x = std::max( max.x, vertex.position.x * ::defaults.size.x * transform.scale.x );
				max.y = std::max( max.y, vertex.position.y * ::defaults.size.y * transform.scale.y );

			#if UF_USE_OPENGL
				vertex.position.y = -vertex.position.y;
			#endif
			}

			metadataGui.size = {
				max.x - min.x,
				max.y - min.y,
			};
		*/
			
			mesh.insertVertices( vertices );
			mesh.insertIndices( indices );

		//	mesh.resizeVertices( metadata.reserve * 6 );
		//	mesh.resizeIndices( metadata.reserve * 6 );
		}

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

				/*alignas(4)*/ float scale;
				/*alignas(4)*/ float padding1;
				/*alignas(4)*/ float padding2;
				/*alignas(4)*/ float padding3;
			} ubo = {
				.stroke = metadata.shader.stroke,
				.range = metadata.shader.range,
				
				.spread = metadata.spread,
				.weight = metadata.shader.weight,
				.scale = metadata.shader.scale,
			};			

		//	if ( metadataGlyph.sdf ) uniforms.gui.mode &= 1 << 1;
		//	if ( metadataGlyph.shadowbox ) uniforms.gui.mode &= 1 << 2;


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