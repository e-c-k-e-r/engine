#include "behavior.h"
#include "gui.h"

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

#include <sys/stat.h>
#include <fstream>
#include <regex>
#include <locale>
#include <codecvt>

#define EXT_COLOR_FLOATS 1

namespace {
#if UF_USE_FREETYPE
	struct {
		ext::freetype::Glyph glyph;
		uf::stl::unordered_map<uf::stl::string, uf::stl::unordered_map<uf::stl::string, uf::Glyph>> cache;
	} glyphs;
#endif
	uf::stl::string defaultRenderMode = "Gui";
	uf::Serializer defaultSettings;

	uf::stl::vector<uf::stl::string> metadataKeys = {
		"alpha",
		"color",
		"depth",
		"font",
		"only model",
		"projection",
		"scale",
		"sdf",
		"shader",
		"shadowbox",
		"size",
		"spread",
		"string",
		"stroke",
		"uv",
		"weight",
		"world",
		"clickable",
		"hoverable"
	};

	struct /*UF_API*/ GuiMesh {
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

UF_VERTEX_DESCRIPTOR(GuiMesh,
	UF_VERTEX_DESCRIPTION(GuiMesh, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(GuiMesh, R32G32_SFLOAT, uv)
#if EXT_COLOR_FLOATS
	UF_VERTEX_DESCRIPTION(GuiMesh, R32G32B32A32_SFLOAT, color)
#else
	UF_VERTEX_DESCRIPTION(GuiMesh, R8G8B8A8_UNORM, color)
#endif
)

uf::stl::vector<pod::GlyphBox> ext::Gui::generateGlyphs( const uf::stl::string& _string ) {
	uf::stl::vector<pod::GlyphBox> gs;
#if UF_USE_FREETYPE
	uf::Object& gui = *this;
	uf::stl::string string = _string;
	auto& metadata = gui.getComponent<ext::GuiBehavior::Metadata>();
	auto& metadataGlyph = gui.getComponent<ext::GuiBehavior::GlyphMetadata>();
	auto& metadataJson = gui.getComponent<uf::Serializer>();

	metadata.deserialize(gui, metadataJson);
	metadataGlyph.deserialize(gui, metadataJson);

//	uf::stl::string font = uf::io::root+"/fonts/" + metadataJson["text settings"]["font"].as<uf::stl::string>();
	uf::stl::string font = uf::io::root+"/fonts/" + metadataGlyph.font;
	if ( ::glyphs.cache[font].empty() ) {
		ext::freetype::initialize( ::glyphs.glyph, font );
	}
	if ( string == "" ) {
		string = metadataGlyph.string;
	}
	pod::Transform<>& transform = gui.getComponent<pod::Transform<>>();
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
			std::size_t index = 0;
		} colors;
	} stat;

	float scale = metadataGlyph.scale;
	pod::Vector3f color = metadata.color;
	stat.colors.container.push_back(color);
	unsigned long COLORCONTROL = 0x7F;

	{
		uf::stl::string text = string;
		std::regex regex("\\%\\#([0-9A-Fa-f]{2})([0-9A-Fa-f]{2})([0-9A-Fa-f]{2})\\%");
		uf::stl::unordered_map<size_t, pod::Vector3f> colors;
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
			text = uf::string::replace( text, "%#" + r.str + g.str + b.str + "%", "\x7F" );
		}
		if ( maxTries == 0 ) {
			text += "\n(error formatting)";
		}
		string = text;
	}

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
	std::wstring str = convert.from_bytes(string);
	if ( str.size() == 0 ) return gs;

	// Calculate statistics
	{
		// Find tallest glyph for new line
		for ( auto it = str.begin(); it != str.end(); ++it ) {
			unsigned long c = *it; if ( c == '\n' ) continue; if ( c == '\t' ) continue; if ( c == 0x01 ) continue;
			uf::stl::string key = ""; {
				key += std::to_string(c) + ";";
				key += std::to_string(metadataGlyph.padding[0]) + ",";
				key += std::to_string(metadataGlyph.padding[1]) + ";";
				key += std::to_string(metadataGlyph.spread) + ";";
				key += std::to_string(metadataGlyph.size) + ";";
				key += metadataGlyph.font + ";";
				key += std::to_string(metadataGlyph.sdf);
			}
			uf::Glyph& glyph = ::glyphs.cache[font][key];

			if ( !glyph.generated() ) {
				glyph.setPadding( { metadataGlyph.padding[0], metadataGlyph.padding[1] } );
				glyph.setSpread( metadataGlyph.spread );
			#if UF_USE_VULKAN
				if ( metadataGlyph.sdf ) glyph.useSdf(true);
			#else
				glyph.useSdf(false);
			#endif
				glyph.generate( ::glyphs.glyph, c, metadataGlyph.size );
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
			unsigned long c = *it; if ( c == '\n' ) {
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
			} else if ( c == COLORCONTROL ) {
				continue;
			}
			uf::stl::string key = ""; {
				key += std::to_string(c) + ";";
				key += std::to_string(metadataGlyph.padding[0]) + ",";
				key += std::to_string(metadataGlyph.padding[1]) + ";";
				key += std::to_string(metadataGlyph.spread) + ";";
				key += std::to_string(metadataGlyph.size) + ";";
				key += metadataGlyph.font + ";";
				key += std::to_string(metadataGlyph.sdf);
			}
			uf::Glyph& glyph = ::glyphs.cache[font][key];
			pod::GlyphBox g;

			g.box.w = glyph.getSize().x;
			g.box.h = glyph.getSize().y;
				
			g.box.x = stat.cursor.x + glyph.getBearing().x;
			g.box.y = stat.cursor.y - glyph.getBearing().y; // - (glyph.getSize().y - glyph.getBearing().y);

			stat.cursor.x += (glyph.getAdvance().x);
		}

		stat.origin.x = ( !metadata.world && transform.position.x != (int) transform.position.x ) ? transform.position.x * ext::gui::size.current.x : transform.position.x;
		stat.origin.y = ( !metadata.world && transform.position.y != (int) transform.position.y ) ? transform.position.y * ext::gui::size.current.y : transform.position.y;
	/*
		if ( ext::json::isArray( metadataJson["text settings"]["origin"] ) ) {
			stat.origin.x = metadataJson["text settings"]["origin"][0].as<int>();
			stat.origin.y = metadataJson["text settings"]["origin"][1].as<int>();
		}
	*/
		if ( metadataGlyph.origin == "top" ) stat.origin.y = ext::gui::size.current.y - stat.origin.y - stat.biggest.y;// else stat.origin.y = stat.origin.y;
		if ( metadataGlyph.align == "right" ) stat.origin.x = ext::gui::size.current.x - stat.origin.x - stat.box.w;// else stat.origin.x = stat.origin.x;
		else if ( metadataGlyph.align == "center" )
			stat.origin.x -= stat.box.w * 0.5f;
	}

	// Render Glyphs
	stat.cursor.x = 0;
	stat.cursor.y = stat.biggest.y;
	for ( auto it = str.begin(); it != str.end(); ++it ) {
		unsigned long c = *it; if ( c == '\n' ) {
			if ( metadataGlyph.direction == "down" ) stat.cursor.y -= stat.biggest.y; else stat.cursor.y += stat.biggest.y;
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
		} else if ( c == COLORCONTROL ) {
			++stat.colors.index;
			continue;
		}
		uf::stl::string key = ""; {
			key += std::to_string(c) + ";";
			key += std::to_string(metadataGlyph.padding[0]) + ",";
			key += std::to_string(metadataGlyph.padding[1]) + ";";
			key += std::to_string(metadataGlyph.spread) + ";";
			key += std::to_string(metadataGlyph.size) + ";";
			key += metadataGlyph.font + ";";
			key += std::to_string(metadataGlyph.sdf);
		}
		uf::Glyph& glyph = ::glyphs.cache[font][key];

		pod::GlyphBox g;
		g.code = c;

		g.box.w = glyph.getSize().x;
		g.box.h = glyph.getSize().y;
			
		g.box.x = stat.cursor.x + (glyph.getBearing().x);
		g.box.y = stat.cursor.y - glyph.getBearing().y; // - (glyph.getSize().y - glyph.getBearing().y);

		stat.cursor.x += (glyph.getAdvance().x);

		if ( stat.colors.index < stat.colors.container.size() ) {
			g.color = stat.colors.container.at(stat.colors.index);
		} else {
		//	std::cout << "Invalid color index `" << stat.colors.index <<  "` for string: " <<  string << ": (" << stat.colors.container.size() << ")" << std::endl;
			g.color = metadata.color;
		}
		gs.push_back(g);
	}
#endif
	return gs;
}

void ext::Gui::load( const uf::Image& image ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& assetLoader = scene.getComponent<uf::Asset>();
/*
	image.open( payload.filename );
	auto& image = this->getComponent<uf::Image>();
	image.copy( _image );
*/
	auto& metadata = this->getComponent<ext::GuiBehavior::Metadata>();
	auto& metadataGlyph = this->getComponent<ext::GuiBehavior::GlyphMetadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();
	metadata.size = image.getDimensions();

	auto& graphic = this->getComponent<uf::Graphic>();
	auto& texture = graphic.material.textures.emplace_back();
	texture.loadFromImage( image );

	auto& transform = this->getComponent<pod::Transform<>>();
#if EXT_COLOR_FLOATS
	auto& color = metadata.color;
#else
	pod::ColorRgba color = { metadata.color[0] * 255, metadata.color[1] * 255, metadata.color[2] * 255, metadata.color[3] * 255 };
#endif

#if 0
	uf::stl::vector<::GuiMesh> vertices = {
#else
	auto& vertices = this->getComponent<uf::stl::vector<::GuiMesh>>();
	vertices = {
#endif
		{ pod::Vector3f{ 1.0f, -1.0f, 0.0f}, pod::Vector2f{1.0f, 0.0f}, color },
		{ pod::Vector3f{-1.0f, -1.0f, 0.0f}, pod::Vector2f{0.0f, 0.0f}, color },
		{ pod::Vector3f{-1.0f,  1.0f, 0.0f}, pod::Vector2f{0.0f, 1.0f}, color },
	
		{ pod::Vector3f{-1.0f,  1.0f, 0.0f}, pod::Vector2f{0.0f, 1.0f}, color },
		{ pod::Vector3f{ 1.0f,  1.0f, 0.0f}, pod::Vector2f{1.0f, 1.0f}, color },
		{ pod::Vector3f{ 1.0f, -1.0f, 0.0f}, pod::Vector2f{1.0f, 0.0f}, color },
	};
	uf::stl::vector<size_t> indices = {
		0, 1, 2, 3, 4, 5
	};

	pod::Vector2f raidou = { 1, 1 };
	bool modified = false;

	if ( metadataJson["mode"].as<uf::stl::string>() == "flat" ) {
		if ( ext::json::isNull(metadataJson["projection"]) ) metadataJson["projection"] = false;
		if ( ext::json::isNull(metadataJson["flip uv"]) ) metadataJson["flip uv"] = true;
		if ( ext::json::isNull(metadataJson["front face"]) ) metadataJson["front face"] = "ccw";
	} else {
		if ( ext::json::isNull(metadataJson["projection"]) ) metadataJson["projection"] = true;
		if ( ext::json::isNull(metadataJson["flip uv"]) ) metadataJson["flip uv"] = false;
		if ( ext::json::isNull(metadataJson["front face"]) ) metadataJson["front face"] = "cw";
	}

	if ( metadataJson["world"].as<bool>() ) {
	//	metadataJson["gui layer"] = false;
	} else {
	#if UF_USE_OPENGL
		if ( ext::json::isNull(metadataJson["cull mode"]) ) metadataJson["cull mode"] = "front";
		transform.position.z = metadata.depth;
	#else
	//	if ( uf::matrix::reverseInfiniteProjection ) metadata.depth = -metadata.depth;
		if ( metadataJson["flip uv"].as<bool>() ) for ( auto& v : vertices ) v.uv.y = 1 - v.uv.y;
	#endif
		for ( auto& v : vertices ) v.position.z = metadata.depth;
	}

	graphic.descriptor.parse( metadataJson );


	if ( metadataJson["scaling"].is<uf::stl::string>() ) {
		uf::stl::string mode = metadataJson["scaling"].as<uf::stl::string>();
		if ( mode == "mesh" ) {
		//	raidou.x = (float) image.getDimensions().x / image.getDimensions().y;
			raidou.y = (float) image.getDimensions().y / image.getDimensions().x;
			modified = true;
		}
	} else if ( ext::json::isArray(metadataJson["scaling"]) ) {
		raidou = uf::vector::decode( metadataJson["scaling"], raidou );
	#if 1
		float factor = 1;
		if ( raidou.x == 0 && raidou.y != 0 ) {
			factor = raidou.y;
		} else if ( raidou.y == 0 && raidou.x != 0 ) {
			factor = raidou.x;
		}
		raidou.x /= factor;
		raidou.y /= factor;
	#endif
		modified = true;
	}
	if ( modified ) {
		transform.scale.x = raidou.x;
		transform.scale.y = raidou.y;
	}
#if 1
	if ( metadataJson["layer"].is<uf::stl::string>() ) {
		graphic.initialize( metadataJson["layer"].as<uf::stl::string>() );
	} else if ( !ext::json::isNull( metadataJson["gui layer"] ) && !metadataJson["gui layer"].as<bool>() ) {
		graphic.initialize();
	} else if ( metadataJson["gui layer"].is<uf::stl::string>() ) {
		graphic.initialize( metadataJson["gui layer"].as<uf::stl::string>() );
	} else {
		graphic.initialize( ::defaultRenderMode );
	}
#endif
//	graphic.initialize();

	auto& mesh = this->getComponent<uf::Mesh>();
	mesh.bind<::GuiMesh>();
	mesh.insertVertices( vertices );
//	mesh.insertIndices( indices );
	graphic.initializeMesh( mesh );

	struct {
		uf::stl::string vertex = uf::io::root+"/shaders/gui/base.vert.spv";
		uf::stl::string fragment = uf::io::root+"/shaders/gui/base.frag.spv";
	} filenames;
	uf::stl::string suffix = ""; {
		uf::stl::string _ = scene.getComponent<uf::Serializer>()["shaders"]["gui"]["suffix"].as<uf::stl::string>();
		if ( _ != "" ) suffix = _ + ".";
	}
	if ( metadataJson["shaders"]["vertex"].is<uf::stl::string>() ) filenames.vertex = metadataJson["shaders"]["vertex"].as<uf::stl::string>();
	if ( metadataJson["shaders"]["fragment"].is<uf::stl::string>() ) filenames.fragment = metadataJson["shaders"]["fragment"].as<uf::stl::string>();
	else if ( suffix != "" ) filenames.fragment = uf::io::root+"/shaders/gui/"+suffix+"base.frag.spv";

	graphic.material.initializeShaders({
		{filenames.vertex, uf::renderer::enums::Shader::VERTEX},
		{filenames.fragment, uf::renderer::enums::Shader::FRAGMENT},
	});
#if UF_USE_VULKAN
	{
		auto& shader = graphic.material.getShader("vertex");
		struct SpecializationConstant {
			uint32_t passes = 6;
		};
		auto& specializationConstants = shader.specializationConstants.get<SpecializationConstant>();
		specializationConstants.passes = uf::renderer::settings::maxViews;
	}
#endif
}

UF_OBJECT_REGISTER_BEGIN(ext::Gui)
	UF_OBJECT_REGISTER_BEHAVIOR(uf::EntityBehavior)
	UF_OBJECT_REGISTER_BEHAVIOR(uf::ObjectBehavior)
	UF_OBJECT_REGISTER_BEHAVIOR(ext::GuiBehavior)
UF_OBJECT_REGISTER_END()
UF_BEHAVIOR_TRAITS_CPP(ext::GuiBehavior, ticks = true, renders = false, multithread = false) // segfaults when destroying, keep off to make smooth transitions
#define this (&self)
void ext::GuiBehavior::initialize( uf::Object& self ) {	
	auto& metadata = this->getComponent<ext::GuiBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();

	UF_BEHAVIOR_METADATA_BIND_SERIALIZER_HOOKS(metadata, metadataJson);

	this->addHook( "asset:Load.%UID%", [&](pod::payloads::assetLoad& payload){
		if ( !uf::Asset::isExpected( payload, uf::Asset::Type::IMAGE ) ) return;

		auto& scene = uf::scene::getCurrentScene();
		auto& assetLoader = scene.getComponent<uf::Asset>();
		if ( !assetLoader.has<uf::Image>(payload.filename) ) return;
		auto& image = assetLoader.get<uf::Image>(payload.filename);
		this->as<ext::Gui>().load( image );
	});

	if ( metadata.clickable ) {
		uf::Timer<long long> clickTimer(false);
		clickTimer.start( uf::Time<>(-1000000) );
		if ( !clickTimer.running() ) clickTimer.start();

		this->addHook( "window:Mouse.Click", [&](pod::payloads::windowMouseClick& payload){
			if ( metadata.world ) return;
			if ( !metadata.box.min && !metadata.box.max ) return;
			
			bool clicked = false;
			if ( payload.mouse.state == -1 ) {
				pod::Vector2f click;
				click.x = (float) payload.mouse.position.x / (float) ext::gui::size.current.x;
				click.y = (float) payload.mouse.position.y / (float) ext::gui::size.current.y;

				click.x = (click.x * 2.0f) - 1.0f;
				click.y = (click.y * 2.0f) - 1.0f;

				float x = click.x;
				float y = click.y;


				if (payload.invoker == "vr" ) {
					x = payload.mouse.position.x;
					y = payload.mouse.position.y;
				}
				clicked = ( metadata.box.min.x <= x && metadata.box.min.y <= y && metadata.box.max.x >= x && metadata.box.max.y >= y );
			
				int minX = (metadata.box.min.x * 0.5f + 0.5f) * ext::gui::size.current.x;
				int minY = (metadata.box.min.y * 0.5f + 0.5f) * ext::gui::size.current.y;

				int maxX = (metadata.box.max.x * 0.5f + 0.5f) * ext::gui::size.current.x;
				int maxY = (metadata.box.max.y * 0.5f + 0.5f) * ext::gui::size.current.y;

				int mouseX = payload.mouse.position.x;
				int mouseY = payload.mouse.position.y;
			}
			
			metadata.clicked = clicked;


			if ( clicked ) {
				UF_MSG_DEBUG("{} clicked", this->getName());
				this->callHook("gui:Clicked.%UID%", payload);
			}
			this->callHook("gui:Mouse.Clicked.%UID%", payload);
		} );


		this->addHook( "gui:Clicked.%UID%", [&](pod::payloads::windowMouseClick& payload){
			if ( ext::json::isObject( metadataJson["events"]["click"] ) ) {
				ext::json::Value event = metadataJson["events"]["click"];
				metadataJson["events"]["click"] = ext::json::array();
				metadataJson["events"]["click"][0] = event;
			} else if ( !ext::json::isArray( metadataJson["events"]["click"] ) ) {
				this->getParent().as<uf::Object>().callHook("gui:Clicked.%UID%", payload);
				return;
			}
			for ( int i = 0; i < metadataJson["events"]["click"].size(); ++i ) {
				ext::json::Value event = metadataJson["events"]["click"][i];
				ext::json::Value payload = event["payload"];
				if ( event["delay"].is<float>() ) {
					this->queueHook(event["name"].as<uf::stl::string>(), payload, event["delay"].as<float>());
				} else {
					this->callHook(event["name"].as<uf::stl::string>(), payload );
				}
			}
		});
	}
	if ( metadata.hoverable ) {
		uf::Timer<long long> hoverTimer(false);
		hoverTimer.start( uf::Time<>(-1000000) );

		this->addHook( "window:Mouse.Moved", [&](pod::payloads::windowMouseMoved& payload){
			if ( metadata.world ) return;
			if ( !metadata.box.min && !metadata.box.max ) return;

			bool hovered = false;
			pod::Vector2f click;
			click.x = (float) payload.mouse.position.x / (float) ext::gui::size.current.x;
			click.y = (float) payload.mouse.position.y / (float) ext::gui::size.current.y;

			click.x = (click.x * 2.0f) - 1.0f;
			click.y = (click.y * 2.0f) - 1.0f;
			float x = click.x;
			float y = click.y;

			hovered = ( metadata.box.min.x <= x && metadata.box.min.y <= y && metadata.box.max.x >= x && metadata.box.max.y >= y );

			if ( hovered ) {
				int minX = (metadata.box.min.x * 0.5f + 0.5f) * ext::gui::size.current.x;
				int minY = (metadata.box.min.y * 0.5f + 0.5f) * ext::gui::size.current.y;

				int maxX = (metadata.box.max.x * 0.5f + 0.5f) * ext::gui::size.current.x;
				int maxY = (metadata.box.max.y * 0.5f + 0.5f) * ext::gui::size.current.y;

				int mouseX = payload.mouse.position.x;
				int mouseY = payload.mouse.position.y;
			}
			
			metadata.hovered = hovered;

			if ( hovered && hoverTimer.elapsed().asDouble() >= 1 ) {
				hoverTimer.reset();
				this->callHook("gui:Hovered.%UID%", payload);
			}
			this->callHook("gui:Mouse.Moved.%UID%", payload);
		});

		this->addHook( "gui:Hovered.%UID%", [&](ext::json::Value& json){
			if ( ext::json::isObject( metadataJson["events"]["hover"] ) ) {
				ext::json::Value event = metadataJson["events"]["hover"];
				metadataJson["events"]["hover"] = ext::json::array(); //Json::arrayValue;
				metadataJson["events"]["hover"][0] = event;
			} else if ( !ext::json::isArray( metadataJson["events"]["hover"] ) ) {
				this->getParent().as<uf::Object>().callHook("gui:Clicked.%UID%", json);
				return;
			}
			for ( int i = 0; i < metadataJson["events"]["hover"].size(); ++i ) {
				ext::json::Value event = metadataJson["events"]["hover"][i];
				ext::json::Value payload = event["payload"];
				float delay = event["delay"].as<float>();
				if ( event["delay"].is<double>() ) {
					this->queueHook(event["name"].as<uf::stl::string>(), payload, event["delay"].as<float>());
				} else {
					this->callHook(event["name"].as<uf::stl::string>(), payload );
				}
			}
			return;
		});
	}

#if UF_USE_FREETYPE
	if ( metadataJson["text settings"]["string"].is<uf::stl::string>() ) {
		if ( ext::json::isNull( ::defaultSettings["metadata"] ) ) ::defaultSettings.readFromFile(uf::io::root+"/entities/gui/text/string.json");

		ext::json::forEach(::defaultSettings["metadata"]["text settings"], [&]( const uf::stl::string& key, ext::json::Value& value ){
			if ( ext::json::isNull( metadataJson["text settings"][key] ) )
				metadataJson["text settings"][key] = value;
		});
		auto& metadataGlyph = this->getComponent<ext::GuiBehavior::GlyphMetadata>();
	//	UF_BEHAVIOR_METADATA_BIND_SERIALIZER_HOOKS(metadataGlyph, metadataJson);

		this->addHook( "object:Serialize.%UID%", [&](){ metadataGlyph.serialize(self, metadataJson); });
		this->addHook( "object:Serialize.%UID%", [&](ext::json::Value& json){ metadataGlyph.serialize(self, (uf::Serializer&) json); });
		this->addHook( "object:Deserialize.%UID%", [&](){ metadataGlyph.deserialize(self, metadataJson); });
		this->addHook( "object:Deserialize.%UID%", [&](ext::json::Value& json){	 metadataGlyph.deserialize(self, (uf::Serializer&) json); });
	//	metadataGlyph.deserialize(self, metadataJson);
		
		this->addHook( "object:Reload.%UID%", [&](ext::json::Value& json){
			if ( json["old"]["text settings"]["string"] == json["new"]["text settings"]["string"] ) return;
			this->queueHook( "gui:UpdateString.%UID%", ext::json::Value{});
		});
		this->addHook( "gui:UpdateString.%UID%", [&](ext::json::Value& json){
			ext::json::forEach(::defaultSettings["metadata"]["text settings"], [&]( const uf::stl::string& key, ext::json::Value& value ){
				if ( ext::json::isNull( metadataJson["text settings"][key] ) )
					metadataJson["text settings"][key] = value;
			});
			if ( json["string"].is<uf::stl::string>() ) metadataJson["text settings"]["string"] = json["string"];
			uf::stl::string string = metadataJson["text settings"]["string"].as<uf::stl::string>();


			auto& metadataGlyph = this->getComponent<ext::GuiBehavior::GlyphMetadata>();
			bool first = metadataGlyph.string == "" || json["first"].as<bool>();
			metadataGlyph.deserialize(self, metadataJson);

			uf::stl::vector<pod::GlyphBox> glyphs = this->as<ext::Gui>().generateGlyphs( string );
			uf::stl::string font = uf::io::root+"/fonts/" + metadataGlyph.font;
			uf::stl::string key = ""; {
				key += std::to_string(metadataGlyph.padding[0]) + ",";
				key += std::to_string(metadataGlyph.padding[1]) + ";";
				key += std::to_string(metadataGlyph.spread) + ";";
				key += std::to_string(metadataGlyph.size) + ";";
				key += metadataGlyph.font + ";";
				key += std::to_string(metadataGlyph.sdf);
			}
			auto& scene = uf::scene::getCurrentScene();
			auto& mesh = this->getComponent<uf::Mesh>();
			auto& graphic = this->getComponent<uf::Graphic>();
			auto& atlas = this->getComponent<uf::Atlas>();
			auto& images = atlas.getImages();
			if ( !first ) {
				atlas.clear();
				mesh.destroy();
				
				for ( auto& texture : graphic.material.textures ) texture.destroy();
				graphic.material.textures.clear();
			}

			uf::stl::unordered_map<uf::stl::string, uf::stl::string> glyphHashMap;
			for ( auto& g : glyphs ) {
				auto glyphKey = std::to_string((uint64_t) g.code) + ";"+key;
				auto& glyph = ::glyphs.cache[font][glyphKey];
			#if UF_USE_OPENGL
				const uint8_t* buffer = glyph.getBuffer();
				uf::Image::container_t pixels;
				std::size_t len = glyph.getSize().x * glyph.getSize().y;
				pixels.reserve( len * 2 );
				for ( size_t i = 0; i < len; ++i ) {
					pixels.emplace_back( buffer[i] );
					pixels.emplace_back( buffer[i] );
				}
				glyphHashMap[glyphKey] = atlas.addImage( &pixels[0], glyph.getSize(), 8, 2, true );
			#else
				glyphHashMap[glyphKey] = atlas.addImage( glyph.getBuffer(), glyph.getSize(), 8, 1, true );
			#endif
			}

			atlas.generate();
			atlas.clear(false);

			float scale = metadataGlyph.scale;
			auto& transform = this->getComponent<pod::Transform<>>();
			transform.scale.x = scale;
			transform.scale.y = scale;
		#if 0
			uf::stl::vector<::GuiMesh> vertices;
		#else
			auto& vertices = this->getComponent<uf::stl::vector<::GuiMesh>>();
		#endif
			uf::stl::vector<size_t> indices;
			vertices.reserve( glyphs.size() * 6 );
			indices.reserve( glyphs.size() * 6 );

			for ( auto& g : glyphs ) {
				auto glyphKey = std::to_string((uint64_t) g.code) + ";"+key;
				auto& glyph = ::glyphs.cache[font][glyphKey];
				auto hash = glyphHashMap[glyphKey];
			#if EXT_COLOR_FLOATS
				auto& color = g.color;
			#else
				pod::ColorRgba color = { g.color[0] * 255, g.color[1] * 255, g.color[2] * 255, g.color[3] * 255 }; 
			#endif
				// add vertices
				vertices.emplace_back(::GuiMesh{pod::Vector3f{ g.box.x,           g.box.y + g.box.h, 0 }, atlas.mapUv( pod::Vector2f{ 0.0f, 0.0f }, hash ), color}); indices.emplace_back( indices.size() );
				vertices.emplace_back(::GuiMesh{pod::Vector3f{ g.box.x,           g.box.y          , 0 }, atlas.mapUv( pod::Vector2f{ 0.0f, 1.0f }, hash ), color}); indices.emplace_back( indices.size() );
				vertices.emplace_back(::GuiMesh{pod::Vector3f{ g.box.x + g.box.w, g.box.y          , 0 }, atlas.mapUv( pod::Vector2f{ 1.0f, 1.0f }, hash ), color}); indices.emplace_back( indices.size() );
				vertices.emplace_back(::GuiMesh{pod::Vector3f{ g.box.x,           g.box.y + g.box.h, 0 }, atlas.mapUv( pod::Vector2f{ 0.0f, 0.0f }, hash ), color}); indices.emplace_back( indices.size() );
				vertices.emplace_back(::GuiMesh{pod::Vector3f{ g.box.x + g.box.w, g.box.y          , 0 }, atlas.mapUv( pod::Vector2f{ 1.0f, 1.0f }, hash ), color}); indices.emplace_back( indices.size() );
				vertices.emplace_back(::GuiMesh{pod::Vector3f{ g.box.x + g.box.w, g.box.y + g.box.h, 0 }, atlas.mapUv( pod::Vector2f{ 1.0f, 0.0f }, hash ), color}); indices.emplace_back( indices.size() );
			}
			for ( size_t i = 0; i < vertices.size(); i += 6 ) {
				for ( size_t j = 0; j < 6; ++j ) {
					auto& vertex = vertices[i+j];
					vertex.position.x /= ext::gui::size.reference.x;
					vertex.position.y /= ext::gui::size.reference.y;
					vertex.position.z = metadata.depth;
				}
			}
			mesh.bind<::GuiMesh>();
			mesh.insertVertices( vertices );
		//	mesh.insertIndices( indices );

			auto& texture = graphic.material.textures.emplace_back();
			texture.loadFromImage( atlas.getAtlas() );

			if ( first ) {
				graphic.initialize( ::defaultRenderMode );
				graphic.initializeMesh( mesh );

				uf::stl::string suffix = ""; {
					uf::stl::string _ = scene.getComponent<uf::Serializer>()["shaders"]["gui"]["suffix"].as<uf::stl::string>();
					if ( _ != "" ) suffix = _ + ".";
				}
				struct {
					uf::stl::string vertex = uf::io::root+"/shaders/gui/text.vert.spv";
					uf::stl::string fragment = uf::io::root+"/shaders/gui/text.frag.spv";
				} filenames;
				if ( metadataJson["shaders"]["vertex"].is<uf::stl::string>() ) filenames.vertex = metadataJson["shaders"]["vertex"].as<uf::stl::string>();
				if ( metadataJson["shaders"]["fragment"].is<uf::stl::string>() ) filenames.fragment = metadataJson["shaders"]["fragment"].as<uf::stl::string>();
				else if ( suffix != "" ) filenames.fragment = uf::io::root+"/shaders/gui/text."+suffix+"frag.spv";

				graphic.material.initializeShaders({
					{filenames.vertex, uf::renderer::enums::Shader::VERTEX},
					{filenames.fragment, uf::renderer::enums::Shader::FRAGMENT},
				});
			} else {
				graphic.initializeMesh( mesh );
				graphic.getPipeline().update( graphic );
			}
		});
		this->callHook("gui:UpdateString.%UID%", ext::json::Value{});
	}
#endif
}
template<size_t N = uf::renderer::settings::maxViews>
struct UniformDescriptor {
	struct Matrices {
		/*alignas(16)*/ pod::Matrix4f model;
	} matrices[N];
	struct Gui {
		/*alignas(16)*/ pod::Vector4f offset;
		/*alignas(16)*/ pod::Vector4f color;

		/*alignas(4)*/ int32_t mode = 0;
		/*alignas(4)*/ float depth = 0.0f;
		/*alignas(8)*/ float padding1;
		/*alignas(8)*/ float padding2;
	} gui;
};
template<size_t N = uf::renderer::settings::maxViews>
struct GlyphUniformDescriptor : public ::UniformDescriptor<N> {
	struct Glyph {
		/*alignas(16)*/ pod::Vector4f stroke;

		/*alignas(4)*/ int32_t spread;
		/*alignas(4)*/ float weight;
		/*alignas(4)*/ float scale;
		/*alignas(4)*/ float padding;
	} glyph;
};
void ext::GuiBehavior::tick( uf::Object& self ) {
	auto& metadata = this->getComponent<ext::GuiBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();
	/* Update uniforms */ if ( this->hasComponent<uf::Graphic>() ) {
		auto& scene = uf::scene::getCurrentScene();
		auto& graphic = this->getComponent<uf::Graphic>();
		auto& controller = scene.getController();
		auto& camera = controller.getComponent<uf::Camera>();
		auto transform = this->getComponent<pod::Transform<>>();
		if ( !graphic.initialized ) return;

		bool isGlyph = this->hasComponent<ext::GuiBehavior::GlyphMetadata>();
	#if UF_USE_OPENGL
		if ( metadata.mode == 0 ) {
			transform.position.y = -transform.position.y;
		}
		auto model = uf::transform::model( transform );
		auto& shader = graphic.material.getShader("vertex");
		pod::Uniform uniform;

		if ( metadata.mode == 1 ) {
			uniform.modelView = model; 
			uniform.projection = uf::matrix::identity();
		} else if ( metadata.mode == 2 ) {
			auto& scene = uf::scene::getCurrentScene();
			auto& controller = scene.getController();
			auto& camera = controller.getComponent<uf::Camera>();
			uniform.modelView = camera.getView() * uf::transform::model( transform );
			uniform.projection = camera.getProjection();
		} else if ( metadata.mode == 3 ) {
			pod::Transform<> flatten = uf::transform::flatten( transform );
			uniform.modelView = 
				uf::matrix::translate( uf::matrix::identity(), flatten.position ) *
				uf::matrix::scale( uf::matrix::identity(), flatten.scale ) *
				uf::quaternion::matrix( flatten.orientation ) *
				flatten.model;
			uniform.projection = camera.getProjection();
		} else {
			pod::Transform<> flatten = uf::transform::flatten( transform );
			uniform.modelView = 
				uf::matrix::translate( uf::matrix::identity(), flatten.position ) *
				uf::matrix::scale( uf::matrix::identity(), flatten.scale ) *
				uf::quaternion::matrix( flatten.orientation ) *
				flatten.model;
			uniform.projection = uf::matrix::identity();
		}

		shader.updateUniform( "UBO", (const void*) &uniform, sizeof(uniform) );
		pod::Uniform* uniformBuffer = (pod::Uniform*) shader.device->getBuffer(shader.getUniformBuffer("UBO").descriptor.buffer);
	#elif UF_USE_VULKAN
		if ( !graphic.material.hasShader("vertex") ) return;

		auto& shader = graphic.material.getShader("vertex");
	#if 1
		auto model = uf::matrix::identity();
		auto& uniformBuffer = shader.getUniformBuffer("UBO");
		isGlyph = isGlyph && uniformBuffer.allocationInfo.size == sizeof(::GlyphUniformDescriptor<>);
		if ( isGlyph ) {
			auto& metadataGlyph = this->getComponent<ext::GuiBehavior::GlyphMetadata>();
			::GlyphUniformDescriptor<> uniforms;

			for ( uint_fast8_t i = 0; i < uf::renderer::settings::maxViews; ++i ) {
				if ( metadata.mode == 1 ) {
					uniforms.matrices[i].model = transform.model; 
				} else if ( metadata.mode == 2 ) {
					auto& scene = uf::scene::getCurrentScene();
					auto& controller = scene.getController();
					auto& camera = controller.getComponent<uf::Camera>();
					uniforms.matrices[i].model = camera.getProjection(i) * camera.getView(i) * uf::transform::model( transform );
				} else if ( metadata.mode == 3 ) {
					pod::Transform<> flatten = uf::transform::flatten( transform );
					uniforms.matrices[i].model = 
						camera.getProjection(i) *
						uf::matrix::translate( uf::matrix::identity(), flatten.position ) *
						uf::matrix::scale( uf::matrix::identity(), flatten.scale ) *
						uf::quaternion::matrix( flatten.orientation ) *
						flatten.model;
				} else {
					pod::Transform<> flatten = uf::transform::flatten( transform );
					uniforms.matrices[i].model = 
						uf::matrix::translate( uf::matrix::identity(), flatten.position ) *
						uf::matrix::scale( uf::matrix::identity(), flatten.scale ) *
						uf::quaternion::matrix( flatten.orientation ) *
						flatten.model;
				}
			}

			uniforms.gui = ::UniformDescriptor<>::Gui{
				.offset = metadata.uv,
				.color = metadata.color,

				.mode = metadata.shader,
			//	.depth = 1 - metadata.depth,
				.depth = uf::matrix::reverseInfiniteProjection ? 1 - metadata.depth : metadata.depth,
			};			

			if ( metadataGlyph.sdf ) uniforms.gui.mode &= 1 << 1;
			if ( metadataGlyph.shadowbox ) uniforms.gui.mode &= 1 << 2;

			uniforms.glyph.stroke = metadataGlyph.stroke;
			uniforms.glyph.spread = metadataGlyph.spread;
			uniforms.glyph.weight = metadataGlyph.weight;
			uniforms.glyph.scale = metadataGlyph.scale;

			shader.updateBuffer( (const void*) &uniforms, sizeof(uniforms), uniformBuffer );
			model = uniforms.matrices[0].model;
		} else {
			UniformDescriptor<> uniforms{};
			for ( uint_fast8_t i = 0; i < uf::renderer::settings::maxViews; ++i ) {
				if ( metadata.mode == 1 ) {
					uniforms.matrices[i].model = transform.model; 
				} else if ( metadata.mode == 2 ) {
					auto& scene = uf::scene::getCurrentScene();
					auto& controller = scene.getController();
					auto& camera = controller.getComponent<uf::Camera>();
					uniforms.matrices[i].model = camera.getProjection(i) * camera.getView(i) * uf::transform::model( transform );
				} else if ( metadata.mode == 3 ) {
					pod::Transform<> flatten = uf::transform::flatten( transform );
					uniforms.matrices[i].model = 
						camera.getProjection(i) *
						uf::matrix::translate( uf::matrix::identity(), flatten.position ) *
						uf::matrix::scale( uf::matrix::identity(), flatten.scale ) *
						uf::quaternion::matrix( flatten.orientation ) *
						flatten.model;
				} else {
					pod::Transform<> flatten = uf::transform::flatten( transform );
					uniforms.matrices[i].model = 
						uf::matrix::translate( uf::matrix::identity(), flatten.position ) *
						uf::matrix::scale( uf::matrix::identity(), flatten.scale ) *
						uf::quaternion::matrix( flatten.orientation ) *
						flatten.model;
				}
			}

			uniforms.gui = ::UniformDescriptor<>::Gui{
				.offset = metadata.uv,
				.color = metadata.color,

				.mode = metadata.shader,
			//	.depth = 1 - metadata.depth,
				.depth = uf::matrix::reverseInfiniteProjection ? 1 - metadata.depth : metadata.depth,
			};
			shader.updateBuffer( (const void*) &uniforms, sizeof(uniforms), uniformBuffer );
			model = uniforms.matrices[0].model;
		}
	#endif

	#endif
		pod::Vector2f min = {  1,  1 };
		pod::Vector2f max = { -1, -1 };

		if ( this->hasComponent<uf::Mesh>() ) {
			auto& mesh = this->getComponent<uf::Mesh>();
			uf::Mesh::Attribute vertexAttribute;
			for ( auto& attribute : mesh.vertex.attributes ) if ( attribute.descriptor.name == "position" ) { vertexAttribute = attribute; break; }
			UF_ASSERT( vertexAttribute.descriptor.name == "position" );

			for ( auto i = 0; i < mesh.vertex.count; ++i ) {
				float* p = (float*) (static_cast<uint8_t*>(vertexAttribute.pointer) + i * vertexAttribute.stride );
				pod::Vector4f position = { p[0], p[1], 0, 1 };
				pod::Vector4f translated = uf::matrix::multiply<float>( model, position );
				min.x = std::min( min.x, translated.x );
				max.x = std::max( max.x, translated.x );
				
				min.y = std::min( min.y, translated.y );
				max.y = std::max( max.y, translated.y );
			}
		}
		
		metadata.box.min.x = min.x;
		metadata.box.min.y = min.y;
		metadata.box.max.x = max.x;
		metadata.box.max.y = max.y;
	}
}
void ext::GuiBehavior::render( uf::Object& self ){}
void ext::GuiBehavior::destroy( uf::Object& self ){}
void ext::GuiBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ){
	serializer["gui"]["color"] = uf::vector::encode( color );
	serializer["gui"]["world"] = world;
	if ( mode == 1 ) serializer["gui"]["only model"] = true;
	else if ( mode == 2 ) serializer["gui"]["world"] = true;
	else if ( mode == 3 ) serializer["gui"]["projection"] = true;
	serializer["box"]["min"] = uf::vector::encode( box.min );
	serializer["box"]["max"] = uf::vector::encode( box.max );
	serializer["gui"]["clickable"] = clickable;
	serializer["gui"]["clicked"] = clicked;
	serializer["gui"]["hoverable"] = hoverable;
	serializer["gui"]["hovered"] = hovered;
	serializer["gui"]["uv"] = uf::vector::encode( uv );
	serializer["gui"]["shader"] = shader;
	serializer["gui"]["depth"] = depth;
	serializer["gui"]["alpha"] = alpha;

	for ( auto& key : metadataKeys ) {
		if ( !ext::json::isNull( serializer[key] ) )
			serializer["gui"][key] = serializer[key];
		if ( !ext::json::isNull( serializer["text settings"][key] ) )
			serializer["gui"][key] = serializer["text settings"][key];
	}
}
void ext::GuiBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ){
	for ( auto& key : metadataKeys ) {
		if ( !ext::json::isNull( serializer[key] ) )
			serializer["gui"][key] = serializer[key];
		if ( !ext::json::isNull( serializer["text settings"][key] ) )
			serializer["gui"][key] = serializer["text settings"][key];
	}
	
	/*this->*/color = uf::vector::decode( serializer["gui"]["color"], /*this->*/color );
	/*this->*/world = serializer["gui"]["world"].as(/*this->*/world);

	if ( serializer["gui"]["only model"].as<bool>() ) mode = 1;
	else if ( serializer["gui"]["world"].as<bool>() ) mode = 2;
	else if ( serializer["gui"]["projection"].as<bool>() ) mode = 3;

	/*this->*/box.min = uf::vector::decode( serializer["box"]["min"], /*this->*/box.min );
	/*this->*/box.max = uf::vector::decode( serializer["box"]["max"], /*this->*/box.max );

	/*this->*/clickable = serializer["gui"]["clickable"].as(/*this->*/clickable);
	/*this->*/clicked = serializer["gui"]["clicked"].as(/*this->*/clicked);
	/*this->*/hoverable = serializer["gui"]["hoverable"].as(/*this->*/hoverable);
	/*this->*/hovered = serializer["gui"]["hovered"].as(/*this->*/hovered);
	/*this->*/uv = uf::vector::decode( serializer["gui"]["uv"], /*this->*/uv );
	
	/*this->*/shader = serializer["gui"]["shader"].as(/*this->*/shader);
	/*this->*/depth = serializer["gui"]["depth"].as(/*this->*/depth);
	/*this->*/alpha = serializer["gui"]["alpha"].as(/*this->*/alpha);
	
	if ( serializer["gui"]["alpha"].is<float>() ) color[3] *= alpha;
}

void ext::GuiBehavior::GlyphMetadata::serialize( uf::Object& self, uf::Serializer& serializer ){
	serializer["text settings"]["font"] = /*this->*/font;
	serializer["text settings"]["string"] = /*this->*/string;

	serializer["text settings"]["scale"] = /*this->*/scale;
	serializer["text settings"]["padding"] = uf::vector::encode( /*this->*/padding );
	serializer["text settings"]["spread"] = /*this->*/spread;
	serializer["text settings"]["size"] = /*this->*/size;
	serializer["text settings"]["weight"] = /*this->*/weight;
	serializer["text settings"]["sdf"] = /*this->*/sdf;
	serializer["text settings"]["shadowbox"] = /*this->*/shadowbox;
	serializer["text settings"]["stroke"] = uf::vector::encode( /*this->*/stroke );

	serializer["text settings"]["origin"] = /*this->*/origin;
	serializer["text settings"]["align"] = /*this->*/align;
	serializer["text settings"]["direction"] = /*this->*/direction;

	for ( auto& key : ::metadataKeys ) {
		if ( !ext::json::isNull( serializer[key] ) )
			serializer["gui"][key] = serializer[key];
		if ( !ext::json::isNull( serializer["text settings"][key] ) )
			serializer["gui"][key] = serializer["text settings"][key];
	}
}
void ext::GuiBehavior::GlyphMetadata::deserialize( uf::Object& self, uf::Serializer& serializer ){
	for ( auto& key : ::metadataKeys ) {
		if ( !ext::json::isNull( serializer[key] ) ) serializer["gui"][key] = serializer[key];
		if ( !ext::json::isNull( serializer["text settings"][key] ) ) serializer["gui"][key] = serializer["text settings"][key];
	}
	/*this->*/font = serializer["gui"]["font"].as(/*this->*/font);
	/*this->*/string = serializer["gui"]["string"].as(/*this->*/string);
	
	/*this->*/scale = serializer["gui"]["scale"].as(/*this->*/scale);
	/*this->*/padding = uf::vector::decode( serializer["gui"]["padding"], /*this->*/padding );
	/*this->*/spread = serializer["gui"]["spread"].as(/*this->*/spread);
	/*this->*/size = serializer["gui"]["size"].as(/*this->*/size);
	/*this->*/weight = serializer["gui"]["weight"].as(/*this->*/weight);
#if UF_USE_OPENGL
	/*this->*/sdf = false;
#else
	/*this->*/sdf = serializer["gui"]["sdf"].as(/*this->*/sdf);
#endif
	/*this->*/shadowbox = serializer["gui"]["shadowbox"].as(/*this->*/shadowbox);
	/*this->*/stroke = uf::vector::decode( serializer["gui"]["stroke"], /*this->*/stroke );
	
	/*this->*/origin = serializer["gui"]["origin"].as(/*this->*/origin);
	/*this->*/align = serializer["gui"]["align"].as(/*this->*/align);
	/*this->*/direction = serializer["gui"]["direction"].as(/*this->*/direction);
}
#undef this