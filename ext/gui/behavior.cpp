#include "behavior.h"
#include "gui.h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/image/atlas.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/window/window.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/graphic/mesh.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/math/physics.h>

#include <uf/utils/text/glyph.h>
#include <uf/engine/asset/asset.h>
#include <uf/engine/scene/scene.h>

#include <unordered_map>
#include <locale>
#include <codecvt>

#include <uf/utils/renderer/renderer.h>
#include <uf/ext/openvr/openvr.h>

#include <uf/utils/http/http.h>
#include <uf/utils/audio/audio.h>
#include <sys/stat.h>
#include <fstream>

#include <regex>

namespace {
	struct {
		ext::freetype::Glyph glyph;
		std::unordered_map<std::string, std::unordered_map<std::string, uf::Glyph>> cache;
	} glyphs;

	uf::Serializer defaultSettings;
}

ext::Gui::Gui(){
	this->addBehavior<ext::GuiBehavior>();
}

std::vector<pod::GlyphBox> ext::Gui::generateGlyphs( const std::string& _string ) {
	uf::Object& gui = *this;
	std::string string = _string;
	uf::Serializer& metadata = gui.getComponent<uf::Serializer>();
	std::string font = "./data/fonts/" + metadata["text settings"]["font"].as<std::string>();
	if ( ::glyphs.cache[font].empty() ) {
		ext::freetype::initialize( ::glyphs.glyph, font );
	}
	if ( string == "" ) {
		string = metadata["text settings"]["string"].as<std::string>();
	}
	pod::Transform<>& transform = gui.getComponent<pod::Transform<>>();
	std::vector<pod::GlyphBox> gs;
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
			std::vector<pod::Vector3f> container;
			std::size_t index = 0;
		} colors;
	} stat;

	float scale = metadata["text settings"]["scale"].as<float>();

	{
		pod::Vector3f color = {
			metadata["text settings"]["color"][0].as<float>(),
			metadata["text settings"]["color"][1].as<float>(),
			metadata["text settings"]["color"][2].as<float>(),
		};
		stat.colors.container.push_back(color);
	}

	unsigned long COLORCONTROL = 0x7F;

	{
		std::string text = string;
		std::regex regex("\\%\\#([0-9A-Fa-f]{2})([0-9A-Fa-f]{2})([0-9A-Fa-f]{2})\\%");
		std::unordered_map<size_t, pod::Vector3f> colors;
		std::smatch match;
		bool matched = false;
		int maxTries = 128;
		while ( (matched = std::regex_search( text, match, regex )) && --maxTries > 0 ) {
			struct {
				std::string str;
				int dec;
			} r, g, b;
			r.str = match[1].str();
			g.str = match[2].str();
			b.str = match[3].str();
			
			{ std::stringstream stream; stream << r.str; stream >> std::hex >> r.dec; } 
			{ std::stringstream stream; stream << g.str; stream >> std::hex >> g.dec; } 
			{ std::stringstream stream; stream << b.str; stream >> std::hex >> b.dec; } 

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
			std::string key = ""; {
				key += std::to_string(c) + ";";
				key += metadata["text settings"]["padding"][0].as<std::string>() + ",";
				key += metadata["text settings"]["padding"][1].as<std::string>() + ";";
				key += metadata["text settings"]["spread"].as<std::string>() + ";";
				key += metadata["text settings"]["size"].as<std::string>() + ";";
				key += metadata["text settings"]["font"].as<std::string>() + ";";
				key += metadata["text settings"]["sdf"].as<std::string>();
			}
			uf::Glyph& glyph = ::glyphs.cache[font][key];

			if ( !glyph.generated() ) {
				glyph.setPadding( { metadata["text settings"]["padding"][0].as<size_t>(), metadata["text settings"]["padding"][1].as<size_t>() } );
				glyph.setSpread( metadata["text settings"]["spread"].as<size_t>() );
				if ( metadata["text settings"]["sdf"].as<bool>() ) glyph.useSdf(true);
				glyph.generate( ::glyphs.glyph, c, metadata["text settings"]["size"].as<int>() );
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
			std::string key = ""; {
				key += std::to_string(c) + ";";
				key += metadata["text settings"]["padding"][0].as<std::string>() + ",";
				key += metadata["text settings"]["padding"][1].as<std::string>() + ";";
				key += metadata["text settings"]["spread"].as<std::string>() + ";";
				key += metadata["text settings"]["size"].as<std::string>() + ";";
				key += metadata["text settings"]["font"].as<std::string>() + ";";
				key += metadata["text settings"]["sdf"].as<std::string>();
			}
			uf::Glyph& glyph = ::glyphs.cache[font][key];
			pod::GlyphBox g;

			g.box.w = glyph.getSize().x;
			g.box.h = glyph.getSize().y;
				
			g.box.x = stat.cursor.x + glyph.getBearing().x;
			g.box.y = stat.cursor.y - glyph.getBearing().y; // - (glyph.getSize().y - glyph.getBearing().y);

			stat.cursor.x += (glyph.getAdvance().x);
		}

		stat.origin.x = ( !metadata["text settings"]["world"].as<bool>() && transform.position.x != (int) transform.position.x ) ? transform.position.x * ext::gui::size.current.x : transform.position.x;
		stat.origin.y = ( !metadata["text settings"]["world"].as<bool>() && transform.position.y != (int) transform.position.y ) ? transform.position.y * ext::gui::size.current.y : transform.position.y;

	
		if ( ext::json::isArray( metadata["text settings"]["origin"] ) ) {
			stat.origin.x = metadata["text settings"]["origin"][0].as<int>();
			stat.origin.y = metadata["text settings"]["origin"][1].as<int>();
		}
		else if ( metadata["text settings"]["origin"] == "top" ) stat.origin.y = ext::gui::size.current.y - stat.origin.y - stat.biggest.y;// else stat.origin.y = stat.origin.y;
		if ( metadata["text settings"]["align"] == "right" ) stat.origin.x = ext::gui::size.current.x - stat.origin.x - stat.box.w;// else stat.origin.x = stat.origin.x;
		else if ( metadata["text settings"]["align"] == "center" )
			stat.origin.x -= stat.box.w * 0.5f;
	}

	// Render Glyphs
	stat.cursor.x = 0;
	stat.cursor.y = stat.biggest.y;
	for ( auto it = str.begin(); it != str.end(); ++it ) {
		unsigned long c = *it; if ( c == '\n' ) {
			if ( metadata["text settings"]["direction"] == "down" ) stat.cursor.y -= stat.biggest.y; else stat.cursor.y += stat.biggest.y;
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
		std::string key = ""; {
			key += std::to_string(c) + ";";
			key += metadata["text settings"]["padding"][0].as<std::string>() + ",";
			key += metadata["text settings"]["padding"][1].as<std::string>() + ";";
			key += metadata["text settings"]["spread"].as<std::string>() + ";";
			key += metadata["text settings"]["size"].as<std::string>() + ";";
			key += metadata["text settings"]["font"].as<std::string>() + ";";
			key += metadata["text settings"]["sdf"].as<std::string>();
		}
		uf::Glyph& glyph = ::glyphs.cache[font][key];

		pod::GlyphBox g;
		g.code = c;

		g.box.w = glyph.getSize().x;
		g.box.h = glyph.getSize().y;
			
		g.box.x = stat.cursor.x + (glyph.getBearing().x);
		g.box.y = stat.cursor.y - glyph.getBearing().y; // - (glyph.getSize().y - glyph.getBearing().y);

		stat.cursor.x += (glyph.getAdvance().x);

		try {
			g.color = stat.colors.container.at(stat.colors.index);
		} catch ( ... ) {
			std::cout << "Invalid color index `" << stat.colors.index <<  "` for string: " <<  string << ": (" << stat.colors.container.size() << ")" << std::endl;
			g.color = {
				metadata["text settings"]["color"][0].as<float>(),
				metadata["text settings"]["color"][1].as<float>(),
				metadata["text settings"]["color"][2].as<float>(),
			};
		}

		gs.push_back(g);
	}
	return gs;
}

void ext::Gui::load( uf::Image& image ) {
	uf::Object& gui = *this;
	uf::Serializer& metadata = gui.getComponent<uf::Serializer>();
	{
	//	this->addAlias<uf::GuiMesh, uf::MeshBase>();
	//	gui.addAlias<uf::GuiMesh, uf::Mesh>();
	}
	uf::GuiMesh& mesh = gui.getComponent<uf::GuiMesh>();
	uf::Graphic& graphic = gui.getComponent<uf::Graphic>();
	/* get original image size (before padding) */ {
		metadata["original size"]["x"] = image.getDimensions().x;
		metadata["original size"]["y"] = image.getDimensions().y;

		// image.padToPowerOfTwo();

		metadata["current size"]["x"] = image.getDimensions().x;
		metadata["current size"]["y"] = image.getDimensions().y;
	}

	std::string suffix = ""; {
		std::string _ = gui.getRootParent<uf::Scene>().getComponent<uf::Serializer>()["shaders"]["gui"]["suffix"].as<std::string>();
		if ( _ != "" ) suffix = _ + ".";
	}
	if ( gui.getName() == "Gui: Text" ) {
		pod::GlyphBox g;
		g.box.x = metadata["text settings"]["box"][0].as<float>();
		g.box.y = metadata["text settings"]["box"][1].as<float>();
		g.box.w = metadata["text settings"]["box"][2].as<float>();
		g.box.h = metadata["text settings"]["box"][3].as<float>();

		mesh.vertices = {
			{{ g.box.x,           g.box.y + g.box.h }, { 0.0f, 0.0f }},
			{{ g.box.x,           g.box.y           }, { 0.0f, 1.0f }},
			{{ g.box.x + g.box.w, g.box.y           }, { 1.0f, 1.0f }},

			{{ g.box.x,           g.box.y + g.box.h }, { 0.0f, 0.0f }},
			{{ g.box.x + g.box.w, g.box.y           }, { 1.0f, 1.0f }},
			{{ g.box.x + g.box.w, g.box.y + g.box.h }, { 1.0f, 0.0f }},
		};
		for ( auto& vertex : mesh.vertices ) {
			vertex.position.x /= ext::gui::size.reference.x;
			vertex.position.y /= ext::gui::size.reference.y;
		}
		graphic.initialize( "Gui" );
		graphic.initializeGeometry( mesh );

		struct {
			std::string vertex = "./data/shaders/gui.text.vert.spv";
			std::string fragment = "./data/shaders/gui.text.frag.spv";
		} filenames;
		if ( metadata["shaders"]["vertex"].is<std::string>() ) filenames.vertex = metadata["shaders"]["vertex"].as<std::string>();
		if ( metadata["shaders"]["fragment"].is<std::string>() ) filenames.fragment = metadata["shaders"]["fragment"].as<std::string>();
		else if ( suffix != "" ) filenames.fragment = "./data/shaders/gui.text."+suffix+"frag.spv";

		graphic.material.initializeShaders({
			{filenames.vertex, VK_SHADER_STAGE_VERTEX_BIT},
			{filenames.fragment, VK_SHADER_STAGE_FRAGMENT_BIT},
		});
	} else {
		mesh.vertices = {
			{ {-1.0f, 1.0f}, {0.0f, 0.0f}, },
			{ {-1.0f, -1.0f}, {0.0f, 1.0f}, },
			{ {1.0f, -1.0f}, {1.0f, 1.0f}, },

			{ {-1.0f, 1.0f}, {0.0f, 0.0f}, },
			{ {1.0f, -1.0f}, {1.0f, 1.0f}, },
			{ {1.0f, 1.0f}, {1.0f, 0.0f}, }
		};
		graphic.initialize( "Gui" );
		graphic.initializeGeometry( mesh );
		struct {
			std::string vertex = "./data/shaders/gui.vert.spv";
			std::string fragment = "./data/shaders/gui.frag.spv";
		} filenames;
		if ( metadata["shaders"]["vertex"].is<std::string>() ) filenames.vertex = metadata["shaders"]["vertex"].as<std::string>();
		if ( metadata["shaders"]["fragment"].is<std::string>() ) filenames.fragment = metadata["shaders"]["fragment"].as<std::string>();
		else if ( suffix != "" ) filenames.fragment = "./data/shaders/gui."+suffix+"frag.spv";
	
		graphic.material.initializeShaders({
			{filenames.vertex, VK_SHADER_STAGE_VERTEX_BIT},
			{filenames.fragment, VK_SHADER_STAGE_FRAGMENT_BIT},
		});
	}


	auto& texture = graphic.material.textures.emplace_back();
	texture.loadFromImage( image );

	{
		pod::Transform<>& transform = gui.getComponent<pod::Transform<>>();
		uf::GuiMesh& mesh = gui.getComponent<uf::GuiMesh>();
		auto& texture = graphic.material.textures.front();

		pod::Vector2f textureSize = {
			metadata["original size"]["x"].as<float>(),
			metadata["original size"]["y"].as<float>()
		};

		if ( metadata["scaling"].as<std::string>() == "fixed" ) {
			transform.scale = pod::Vector3{ (float) textureSize.x / ext::gui::size.current.x, (float) textureSize.y / ext::gui::size.current.y, 1 };
		} else if ( metadata["scaling"].as<std::string>() == "fixed-1080p" ) {
			transform.scale = pod::Vector3{ (float) textureSize.x / 1920, (float) textureSize.y / 1080, 1 };
		}
	}
}

UF_OBJECT_REGISTER_BEGIN(ext::Gui)
	UF_OBJECT_REGISTER_BEHAVIOR(uf::EntityBehavior)
	UF_OBJECT_REGISTER_BEHAVIOR(uf::ObjectBehavior)
	UF_OBJECT_REGISTER_BEHAVIOR(ext::GuiBehavior)
UF_OBJECT_REGISTER_END()
#define this (&self)
void ext::GuiBehavior::initialize( uf::Object& self ) {	
	auto& metadata = this->getComponent<uf::Serializer>();

	this->addHook( "glyph:Load.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		unsigned long c = json["glyph"].as<size_t>();
		std::string font = "./data/fonts/" + metadata["text settings"]["font"].as<std::string>();
		std::string key = ""; {
			key += std::to_string(c) + ";";
			key += metadata["text settings"]["padding"][0].as<std::string>() + ",";
			key += metadata["text settings"]["padding"][1].as<std::string>() + ";";
			key += metadata["text settings"]["spread"].as<std::string>() + ";";
			key += metadata["text settings"]["size"].as<std::string>() + ";";
			key += metadata["text settings"]["font"].as<std::string>() + ";";
			key += metadata["text settings"]["sdf"].as<std::string>();
		}
		uf::Glyph& glyph = ::glyphs.cache[font][key];
		
		uf::Image image; {
			const uint8_t* buffer = glyph.getBuffer();
			uf::Image::container_t pixels;
			std::size_t len = glyph.getSize().x * glyph.getSize().y;
			pixels.insert( pixels.end(), buffer, buffer + len );
			image.loadFromBuffer( pixels, glyph.getSize(), 8, 1, true );
		}
		this->as<ext::Gui>().load( image );
		return "true";
	});
	this->addHook( "asset:Load.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		std::string filename = json["filename"].as<std::string>();

		if ( uf::io::extension(filename) != "png" ) return "false";

		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
		const uf::Image* imagePointer = NULL;
		try { imagePointer = &assetLoader.get<uf::Image>(filename); } catch ( ... ) {}
		if ( !imagePointer ) return "false";
		
		uf::Image image = *imagePointer;

		this->as<ext::Gui>().load( image );
		return "true";
	});
	this->addHook( "window:Resized", [&](const std::string& event)->std::string{
		uf::Serializer json = event;
		if ( !this->hasComponent<uf::GuiMesh>() ) return "false";

		pod::Vector2ui size; {
			size.x = json["window"]["size"]["x"].as<size_t>();
			size.y = json["window"]["size"]["y"].as<size_t>();
		}
		pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
	//	uf::Graphic& graphic = this->getComponent<uf::Graphic>();
	//	auto& texture = graphic.material.textures.front();
		pod::Vector2f textureSize = {
			metadata["original size"]["x"].as<float>(),
			metadata["original size"]["y"].as<float>()
		};

		if ( ext::json::isObject( metadata["text settings"] ) ) {
		} else if ( metadata["scaling"].as<std::string>() == "fixed" ) {
			transform.scale = pod::Vector3{ (float) textureSize.x / size.x, (float) textureSize.y / size.y, 1 };
		} else if ( metadata["scaling"].as<std::string>() == "fixed-1080p" ) {
			transform.scale = pod::Vector3{ (float) textureSize.x / 1920, (float) textureSize.y / 1080, 1 };
		}

		return "true";
	} );
		
	if ( metadata["system"]["clickable"].as<bool>() ) {
		uf::Timer<long long> clickTimer(false);
	//	clickTimer.start( uf::Time<>(-1000000) );
		if ( !clickTimer.running() ) clickTimer.start();
		this->addHook( "gui:Clicked.%UID%", [&](const std::string& event)->std::string{
			uf::Serializer json = event;

			if ( ext::json::isObject( metadata["events"]["click"] ) ) {
				uf::Serializer event = metadata["events"]["click"];
				metadata["events"]["click"] = Json::arrayValue;
				metadata["events"]["click"][0] = event;
			} else if ( !ext::json::isArray( metadata["events"]["click"] ) ) {
				this->getParent().as<uf::Object>().callHook("gui:Clicked.%UID%", event);
				return "false";
			}
			for ( int i = 0; i < metadata["events"]["click"].size(); ++i ) {
				uf::Serializer event = metadata["events"]["click"][i];
				uf::Serializer payload = event["payload"];
				float delay = event["delay"].as<float>();
				if ( event["delay"].is<double>() ) {
					this->queueHook(event["name"].as<std::string>(), payload, event["delay"].as<float>());
				} else {
					this->callHook(event["name"].as<std::string>(), payload );
				}
			}
			return "true";
		});
		this->addHook( "window:Mouse.Click", [&](const std::string& event)->std::string{
			uf::Serializer json = event;
//			if ( !this->hasComponent<uf::GuiMesh>() ) return "false";
			uf::Serializer& metadata = this->getComponent<uf::Serializer>();

			if ( metadata["world"].as<bool>() ) return "true";
			if ( metadata["box"] == Json::nullValue ) return "true";

			bool down = json["mouse"]["state"].as<std::string>() == "Down";
			bool clicked = false;
			if ( down ) {
				pod::Vector2ui position; {
					position.x = json["mouse"]["position"]["x"].as<int>() > 0 ? json["mouse"]["position"]["x"].as<size_t>() : 0;
					position.y = json["mouse"]["position"]["y"].as<int>() > 0 ? json["mouse"]["position"]["y"].as<size_t>() : 0;
				}
				pod::Vector2f click; {
					click.x = (float) position.x / (float) ext::gui::size.current.x;
					click.y = (float) position.y / (float) ext::gui::size.current.y;

					click.x = (click.x * 2.0f) - 1.0f;
					click.y = (click.y * 2.0f) - 1.0f;

					float x = click.x;
					float y = click.y;


					if (json["invoker"] == "vr" ) {
						x = json["mouse"]["position"]["x"].as<float>();
						y = json["mouse"]["position"]["y"].as<float>();
					}

					pod::Vector2f min = { metadata["box"]["min"]["x"].as<float>(), metadata["box"]["min"]["y"].as<float>() };
					pod::Vector2f max = { metadata["box"]["max"]["x"].as<float>(), metadata["box"]["max"]["y"].as<float>() };
					clicked = ( min.x <= x && min.y <= y && max.x >= x && max.y >= y );
				}

			}
			metadata["clicked"] = clicked;
			if ( clicked ) {
				this->callHook("gui:Clicked.%UID%");
			}
			return "true";
		} );
	}
	if ( metadata["system"]["hoverable"].as<bool>() ) {
		uf::Timer<long long> hoverTimer(false);
		hoverTimer.start( uf::Time<>(-1000000) );
		this->addHook( "gui:Hovered.%UID%", [&](const std::string& event)->std::string{
			uf::Serializer json = event;

			if ( ext::json::isObject( metadata["events"]["hover"] ) ) {
				uf::Serializer event = metadata["events"]["hover"];
				metadata["events"]["hover"] = Json::arrayValue;
				metadata["events"]["hover"][0] = event;
			} else if ( !ext::json::isArray( metadata["events"]["hover"] ) ) {
				this->getParent().as<uf::Object>().callHook("gui:Clicked.%UID%", event);
				return "false";
			}
			for ( int i = 0; i < metadata["events"]["hover"].size(); ++i ) {
				uf::Serializer event = metadata["events"]["hover"][i];
				uf::Serializer payload = event["payload"];
				float delay = event["delay"].as<float>();
				if ( event["delay"].is<double>() ) {
					this->queueHook(event["name"].as<std::string>(), payload, event["delay"].as<float>());
				} else {
					this->callHook(event["name"].as<std::string>(), payload );
				}
			}
			return "true";
		});
		this->addHook( "window:Mouse.Moved", [&](const std::string& event)->std::string{
			uf::Serializer json = event;
			if ( this->getUid() == 0 ) return "false";
			if ( !this->hasComponent<uf::GuiMesh>() ) return "false";
			uf::Serializer& metadata = this->getComponent<uf::Serializer>();
			if ( metadata["world"].as<bool>() ) return "true";
			if ( metadata["box"] == Json::nullValue ) return "true";

			bool down = json["mouse"]["state"].as<std::string>() == "Down";
			bool clicked = false;
			pod::Vector2ui position; {
				position.x = json["mouse"]["position"]["x"].as<int>() > 0 ? json["mouse"]["position"]["x"].as<size_t>() : 0;
				position.y = json["mouse"]["position"]["y"].as<int>() > 0 ? json["mouse"]["position"]["y"].as<size_t>() : 0;
			}
			pod::Vector2f click; {
				click.x = (float) position.x / (float) ext::gui::size.current.x;
				click.y = (float) position.y / (float) ext::gui::size.current.y;

				click.x = (click.x * 2.0f) - 1.0f;
				click.y = (click.y * 2.0f) - 1.0f;
				float x = click.x;
				float y = click.y;

				pod::Vector2f min = { metadata["box"]["min"]["x"].as<float>(), metadata["box"]["min"]["y"].as<float>() };
				pod::Vector2f max = { metadata["box"]["max"]["x"].as<float>(), metadata["box"]["max"]["y"].as<float>() };
				clicked = ( min.x <= x && min.y <= y && max.x >= x && max.y >= y );
			}
			metadata["hovered"] = clicked;

			if ( clicked && hoverTimer.elapsed().asDouble() >= 1 ) {
				hoverTimer.reset();
				this->callHook("gui:Hovered.%UID%");
			}

			return "true";
		} );
	}
	if ( metadata["text settings"]["string"].is<std::string>() ) {
		if ( ext::json::isNull( ::defaultSettings["metadata"] ) ) {
			::defaultSettings.readFromFile("./data/entities/gui/text/string.json");
		}
		for ( auto it = ::defaultSettings["metadata"]["text settings"].begin(); it != ::defaultSettings["metadata"]["text settings"].end(); ++it ) {
			std::string key = it.key().as<std::string>();
			if ( ext::json::isNull( metadata["text settings"][key] ) ) {
				metadata["text settings"][key] = ::defaultSettings["metadata"]["text settings"][key];
			}
		}
		if ( metadata["text settings"]["legacy"].as<bool>() ) {
			float delay = 0.0f;
			float scale = metadata["text settings"]["scale"].as<float>();
			std::vector<pod::GlyphBox> glyphs = this->as<ext::Gui>().generateGlyphs();
			for ( auto& glyph : glyphs ) {
				uf::Object& glyphElement = this->loadChild("/gui/text/letter.json", false);

				uf::Serializer& pMetadata = glyphElement.getComponent<uf::Serializer>();
			
			//	pMetadata["events"] = metadata["events"];
				pMetadata["system"]["hot reload"] = metadata["system"]["hot reload"];
				pMetadata["text settings"] = metadata["text settings"];
				pMetadata["text settings"].removeMember("string");
				pMetadata["text settings"]["letter"] = (wchar_t) glyph.code;
				
				pMetadata["text settings"]["color"][0] = glyph.color[0];
				pMetadata["text settings"]["color"][1] = glyph.color[1];
				pMetadata["text settings"]["color"][2] = glyph.color[2];

				pMetadata["text settings"]["box"][0] = glyph.box.x;
				pMetadata["text settings"]["box"][1] = glyph.box.y;
				pMetadata["text settings"]["box"][2] = glyph.box.w;
				pMetadata["text settings"]["box"][3] = glyph.box.h;

				pMetadata["system"]["hoverable"] = metadata["system"]["hoverable"];
				pMetadata["system"]["clickable"] = metadata["system"]["clickable"];
				
				glyphElement.initialize();

				pod::Transform<>& pTransform = glyphElement.getComponent<pod::Transform<>>();
				pTransform.scale.x = scale;
				pTransform.scale.y = scale;
				pTransform.reference = this->getComponentPointer<pod::Transform<>>();
			
				uf::Serializer payload;
				payload["glyph"] = (uint64_t) glyph.code;
				if ( metadata["text settings"]["scroll speed"].is<double>() ) {
					glyphElement.queueHook("glyph:Load.%UID%", payload, delay);
					delay += metadata["text settings"]["scroll speed"].as<float>();
				} else {
					glyphElement.callHook("glyph:Load.%UID%", payload);
				}
			}
			return;
		}
		{
			float delay = 0.0f;
			float scale = metadata["text settings"]["scale"].as<float>();
			auto& transform = this->getComponent<pod::Transform<>>();
			transform.scale.x = scale;
			transform.scale.y = scale;

			std::vector<pod::GlyphBox> glyphs = this->as<ext::Gui>().generateGlyphs();
			
			std::string font = "./data/fonts/" + metadata["text settings"]["font"].as<std::string>();
			std::string key = ""; {
				key += metadata["text settings"]["padding"][0].as<std::string>() + ",";
				key += metadata["text settings"]["padding"][1].as<std::string>() + ";";
				key += metadata["text settings"]["spread"].as<std::string>() + ";";
				key += metadata["text settings"]["size"].as<std::string>() + ";";
				key += metadata["text settings"]["font"].as<std::string>() + ";";
				key += metadata["text settings"]["sdf"].as<std::string>();
			}
			auto& scene = uf::scene::getCurrentScene();
			auto& atlas = this->getComponent<uf::Atlas>();
			auto& images = atlas.getImages();
			auto& mesh = this->getComponent<ext::Gui::glyph_mesh_t>();

			auto& graphic = this->getComponent<uf::Graphic>();
			mesh.vertices.reserve( glyphs.size() * 6 );
			for ( auto& g : glyphs ) {
				auto& glyph = ::glyphs.cache[font][std::to_string((uint64_t) g.code) + ";"+key];
				// add bitmap to atlas
				uf::Image& image = images.emplace_back();
				const uint8_t* buffer = glyph.getBuffer();
				uf::Image::container_t pixels;
				std::size_t len = glyph.getSize().x * glyph.getSize().y;
				pixels.insert( pixels.end(), buffer, buffer + len );
				image.loadFromBuffer( pixels, glyph.getSize(), 8, 1, true );
				// add vertices
				mesh.vertices.push_back({pod::Vector3f{ g.box.x,           g.box.y + g.box.h, 0 }, pod::Vector2f{ 0.0f, 0.0f }, g.color});
				mesh.vertices.push_back({pod::Vector3f{ g.box.x,           g.box.y          , 0 }, pod::Vector2f{ 0.0f, 1.0f }, g.color});
				mesh.vertices.push_back({pod::Vector3f{ g.box.x + g.box.w, g.box.y          , 0 }, pod::Vector2f{ 1.0f, 1.0f }, g.color});
				mesh.vertices.push_back({pod::Vector3f{ g.box.x,           g.box.y + g.box.h, 0 }, pod::Vector2f{ 0.0f, 0.0f }, g.color});
				mesh.vertices.push_back({pod::Vector3f{ g.box.x + g.box.w, g.box.y          , 0 }, pod::Vector2f{ 1.0f, 1.0f }, g.color});
				mesh.vertices.push_back({pod::Vector3f{ g.box.x + g.box.w, g.box.y + g.box.h, 0 }, pod::Vector2f{ 1.0f, 0.0f }, g.color});
			}

			atlas.generate();
			
			for ( size_t i = 0, k = 0; i < mesh.vertices.size(); ++k, i += 6 ) {
				for ( size_t j = 0; j < 6; ++j ) {
					auto& vertex = mesh.vertices[i+j];
					vertex.position.x /= ext::gui::size.reference.x;
					vertex.position.y /= ext::gui::size.reference.y;

					vertex.uv = atlas.mapUv( vertex.uv, k );
					vertex.uv.y = 1 - vertex.uv.y;
				}
			}

			auto& texture = graphic.material.textures.emplace_back();
			texture.loadFromImage( atlas.getAtlas() );

			graphic.initialize( "Gui" );
			graphic.initializeGeometry( mesh );

			std::string suffix = ""; {
				std::string _ = scene.getComponent<uf::Serializer>()["shaders"]["gui"]["suffix"].as<std::string>();
				if ( _ != "" ) suffix = _ + ".";
			}
			struct {
				std::string vertex = "./data/shaders/gui.text.vert.spv";
				std::string fragment = "./data/shaders/gui.text.frag.spv";
			} filenames;
			if ( metadata["shaders"]["vertex"].is<std::string>() ) filenames.vertex = metadata["shaders"]["vertex"].as<std::string>();
			if ( metadata["shaders"]["fragment"].is<std::string>() ) filenames.fragment = metadata["shaders"]["fragment"].as<std::string>();
			else if ( suffix != "" ) filenames.fragment = "./data/shaders/gui.text."+suffix+"frag.spv";

			graphic.material.initializeShaders({
				{filenames.vertex, VK_SHADER_STAGE_VERTEX_BIT},
				{filenames.fragment, VK_SHADER_STAGE_FRAGMENT_BIT},
			});
		}

		this->addHook( "object:Reload.%UID%", [&](const std::string& event)->std::string{
			uf::Serializer json = event;

			if ( json["old"]["text settings"]["string"] == json["new"]["text settings"]["string"] ) return "false";
			this->queueHook( "gui:UpdateString.%UID%");
			return "true";
		});
		this->addHook( "gui:UpdateString.%UID%", [&](const std::string& event)->std::string{
			uf::Serializer json = event;
			for ( auto it = ::defaultSettings["metadata"]["text settings"].begin(); it != ::defaultSettings["metadata"]["text settings"].end(); ++it ) {
				std::string key = it.key().as<std::string>();
				if ( ext::json::isNull( metadata["text settings"][key] ) ) {
					metadata["text settings"][key] = ::defaultSettings["metadata"]["text settings"][key];
				}
			}
			if ( json["string"].is<std::string>() ) {
				metadata["text settings"]["string"] = json["string"];
			}
			std::string string = metadata["text settings"]["string"].as<std::string>();

			float delay = 0.0f;
			float scale = metadata["text settings"]["scale"].as<float>();
			std::vector<pod::GlyphBox> glyphs = this->as<ext::Gui>().generateGlyphs( string );
			
			std::string font = "./data/fonts/" + metadata["text settings"]["font"].as<std::string>();
			std::string key = ""; {
				key += metadata["text settings"]["padding"][0].as<std::string>() + ",";
				key += metadata["text settings"]["padding"][1].as<std::string>() + ";";
				key += metadata["text settings"]["spread"].as<std::string>() + ";";
				key += metadata["text settings"]["size"].as<std::string>() + ";";
				key += metadata["text settings"]["font"].as<std::string>() + ";";
				key += metadata["text settings"]["sdf"].as<std::string>();
			}
			auto& transform = this->getComponent<pod::Transform<>>();
			auto& scene = uf::scene::getCurrentScene();
			auto& atlas = this->getComponent<uf::Atlas>();
			auto& images = atlas.getImages();
			auto& mesh = this->getComponent<ext::Gui::glyph_mesh_t>();

			auto& graphic = this->getComponent<uf::Graphic>();

			atlas.clear();
			mesh.vertices.clear();
			mesh.indices.clear();
			mesh.vertices.reserve( glyphs.size() * 6 );
			for ( auto& g : glyphs ) {
				auto& glyph = ::glyphs.cache[font][std::to_string((uint64_t) g.code) + ";"+key];
				// add bitmap to atlas
				uf::Image& image = images.emplace_back();
				const uint8_t* buffer = glyph.getBuffer();
				uf::Image::container_t pixels;
				std::size_t len = glyph.getSize().x * glyph.getSize().y;
				pixels.insert( pixels.end(), buffer, buffer + len );
				image.loadFromBuffer( pixels, glyph.getSize(), 8, 1, true );
				// add vertices
				mesh.vertices.push_back({pod::Vector3f{ g.box.x,           g.box.y + g.box.h, 0 }, pod::Vector2f{ 0.0f, 0.0f }, g.color});
				mesh.vertices.push_back({pod::Vector3f{ g.box.x,           g.box.y          , 0 }, pod::Vector2f{ 0.0f, 1.0f }, g.color});
				mesh.vertices.push_back({pod::Vector3f{ g.box.x + g.box.w, g.box.y          , 0 }, pod::Vector2f{ 1.0f, 1.0f }, g.color});
				mesh.vertices.push_back({pod::Vector3f{ g.box.x,           g.box.y + g.box.h, 0 }, pod::Vector2f{ 0.0f, 0.0f }, g.color});
				mesh.vertices.push_back({pod::Vector3f{ g.box.x + g.box.w, g.box.y          , 0 }, pod::Vector2f{ 1.0f, 1.0f }, g.color});
				mesh.vertices.push_back({pod::Vector3f{ g.box.x + g.box.w, g.box.y + g.box.h, 0 }, pod::Vector2f{ 1.0f, 0.0f }, g.color});
			}

			atlas.generate();

			transform.scale.x = scale;
			transform.scale.y = scale;
			for ( size_t i = 0, k = 0; i < mesh.vertices.size(); ++k, i += 6 ) {
				for ( size_t j = 0; j < 6; ++j ) {
					auto& vertex = mesh.vertices[i+j];
					vertex.position.x /= ext::gui::size.reference.x;
					vertex.position.y /= ext::gui::size.reference.y;

					vertex.uv = atlas.mapUv( vertex.uv, k );
					vertex.uv.y = 1 - vertex.uv.y;
				}
			}

			for ( auto& texture : graphic.material.textures ) texture.destroy();
			graphic.material.textures.clear();

			auto& texture = graphic.material.textures.emplace_back();
			texture.loadFromImage( atlas.getAtlas() );
			// graphic.descriptor.indices = 0;
			graphic.initializeGeometry( mesh );
			graphic.getPipeline().update( graphic );
		//	graphic.updatePipelines();
			// uf::renderer::rebuild = true;

		/*
			graphic.destroy();

			auto& texture = graphic.material.textures.emplace_back();
			texture.loadFromImage( atlas.getAtlas() );

			graphic.initialize( "Gui" );
			graphic.initializeGeometry( mesh );

			std::string suffix = ""; {
				std::string _ = scene.getComponent<uf::Serializer>()["shaders"]["gui"]["suffix"].as<std::string>();
				if ( _ != "" ) suffix = _ + ".";
			}
			struct {
				std::string vertex = "./data/shaders/gui.text.vert.spv";
				std::string fragment = "./data/shaders/gui.text.frag.spv";
			} filenames;
			if ( metadata["shaders"]["vertex"].is<std::string>() ) filenames.vertex = metadata["shaders"]["vertex"].as<std::string>();
			if ( metadata["shaders"]["fragment"].is<std::string>() ) filenames.fragment = metadata["shaders"]["fragment"].as<std::string>();
			else if ( suffix != "" ) filenames.fragment = "./data/shaders/gui.text."+suffix+"frag.spv";

			graphic.material.initializeShaders({
				{filenames.vertex, VK_SHADER_STAGE_VERTEX_BIT},
				{filenames.fragment, VK_SHADER_STAGE_FRAGMENT_BIT},
			});
		*/

			return "true";
		});
	}
}
void ext::GuiBehavior::tick( uf::Object& self ) {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	if ( metadata["text settings"]["fade in speed"].is<double>() && !metadata["system"]["faded in"].as<bool>() ) {
		float speed = metadata["text settings"]["fade in speed"].as<float>();
		float alpha = metadata["text settings"]["color"][3].as<float>();
		speed *= uf::physics::time::delta;
		if ( alpha < 1 && alpha + speed > 1 ) {
			alpha = 1;
			speed = 0;

			metadata["system"]["faded in"] = true;
		}
		if ( alpha + speed <= 1 ) {
			metadata["text settings"]["color"][3] = alpha + speed;
		}
	}
}

void ext::GuiBehavior::render( uf::Object& self ){
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	/* Update uniforms */ if ( this->hasComponent<uf::Graphic>() ) {
		auto& scene = uf::scene::getCurrentScene();
		auto& graphic = this->getComponent<uf::Graphic>();
		auto& controller = scene.getController();
		auto& camera = controller.getComponent<uf::Camera>();
		auto& transform = this->getComponent<pod::Transform<>>();
		if ( !graphic.initialized ) return;

		pod::Vector4 offset = {
			metadata["uv"][0].as<float>(),
			metadata["uv"][1].as<float>(),
			metadata["uv"][2].as<float>(),
			metadata["uv"][3].as<float>()
		};
		int mode = 0;
		if ( !ext::json::isNull( metadata["shader"] ) ) mode = metadata["shader"].as<int>();
		
		if ( !ext::json::isNull( metadata["text settings"]["legacy"] ) && ((metadata["text settings"]["legacy"].as<bool>() && this->getName() == "Gui: Text") || (!metadata["text settings"]["legacy"].as<bool>() && metadata["text settings"]["string"].is<std::string>())) ) {
			struct GlyphDescriptor {
				struct {
					alignas(16) pod::Matrix4f model[2];
				} matrices;
				struct {
					alignas(16) pod::Vector4f offset;
					alignas(16) pod::Vector4f color;
					alignas(4) int32_t mode = 0;
					alignas(4) float depth = 0.0f;
					alignas(4) int32_t sdf = false;
					alignas(4) int32_t shadowbox = false;
					alignas(16) pod::Vector4f stroke;
					alignas(4) float weight;
					alignas(4) int32_t spread;
					alignas(4) float scale;
					alignas(4) float padding;
				} gui;
			};
			auto& uniforms = graphic.material.shaders.front().uniforms.front().get<GlyphDescriptor>();

			if ( !ext::json::isArray( metadata["text settings"]["color"] ) ) {
				metadata["text settings"]["color"][0] = 1.0f;
				metadata["text settings"]["color"][1] = 1.0f;
				metadata["text settings"]["color"][2] = 1.0f;
				metadata["text settings"]["color"][3] = 1.0f;
			}
			if ( !ext::json::isArray( metadata["text settings"]["stroke"] ) ) {
				metadata["text settings"]["stroke"][0] = 0.0f;
				metadata["text settings"]["stroke"][1] = 0.0f;
				metadata["text settings"]["stroke"][2] = 0.0f;
				metadata["text settings"]["stroke"][3] = 1.0f;
			}
			pod::Vector4 color = {
				metadata["text settings"]["color"][0].as<float>(),
				metadata["text settings"]["color"][1].as<float>(),
				metadata["text settings"]["color"][2].as<float>(),
				metadata["text settings"]["color"][3].as<float>()
			};
			if ( metadata["alpha"].is<double>() ) {
				color[3] *= metadata["alpha"].as<float>();
			}
			pod::Vector4 stroke = {
				metadata["text settings"]["stroke"][0].as<float>(),
				metadata["text settings"]["stroke"][1].as<float>(),
				metadata["text settings"]["stroke"][2].as<float>(),
				metadata["text settings"]["stroke"][3].as<float>()
			};

			uniforms.gui.offset = offset;
			uniforms.gui.color = color;
			uniforms.gui.stroke = stroke;
			uniforms.gui.mode = mode;
			uniforms.gui.sdf = metadata["text settings"]["sdf"].as<bool>();
			uniforms.gui.shadowbox = metadata["text settings"]["shadowbox"].as<bool>();
			uniforms.gui.weight = metadata["text settings"]["weight"].as<float>(); // float
			uniforms.gui.spread = metadata["text settings"]["spread"].as<int>(); // int
			uniforms.gui.scale = metadata["text settings"]["scale"].as<float>(); // float
			if ( !ext::json::isNull( metadata["text settings"]["depth"] ) ) 
				uniforms.gui.depth = metadata["text settings"]["depth"].as<float>();
			else uniforms.gui.depth = 0.0f;

			for ( std::size_t i = 0; i < 2; ++i ) {
				if ( metadata["text settings"]["world"].as<bool>() ) {
					auto& scene = uf::scene::getCurrentScene();
					auto& controller = scene.getController();
					auto& camera = controller.getComponent<uf::Camera>();

					pod::Transform<> flatten = uf::transform::flatten( this->getComponent<pod::Transform<>>() );
					auto model = uf::transform::model( flatten );
					auto view = camera.getView(i);
					auto projection = camera.getProjection(i);
					uniforms.matrices.model[i] = projection * view * model;
				} else {			
					uf::Matrix4 translation, rotation, scale;
					pod::Transform<> flatten = uf::transform::flatten(transform, false);
					// make our own flattened position, for some reason this causes z to be 6.6E+28
					flatten.position = transform.position;
					if ( transform.reference ) flatten.position += transform.reference->position;
					flatten.orientation.w *= -1;
					rotation = uf::quaternion::matrix(flatten.orientation);
				//	pod::Vector3f offsetSize = { ext::gui::size.current.x, ext::gui::size.current.y, 1 };
				//	translation = uf::matrix::translate( uf::matrix::identity(), flatten.position * offsetSize );
					translation = uf::matrix::translate( uf::matrix::identity(), flatten.position );
					scale = uf::matrix::scale( scale, transform.scale );
				//	uniforms.matrices.model[i] = ::matrix * translation * rotation * scale;
					uniforms.matrices.model[i] = translation * rotation * scale;
				}
			}
			uniforms.gui.depth = 1.0f - uniforms.gui.depth; 
			// graphic.updateBuffer( uniforms, 0, false );
			graphic.material.shaders.front().updateBuffer( uniforms, 0, false );
			// calculate click box
			auto& model = uniforms.matrices.model[0];
			pod::Vector2f min = {  1,  1 };
			pod::Vector2f max = { -1, -1 };
			if ( this->hasComponent<uf::GuiMesh>() ) {
				auto& mesh = this->getComponent<uf::GuiMesh>();
				for ( auto& vertex : mesh.vertices ) {
					pod::Vector4f position = { vertex.position.x, vertex.position.y, 0, 1 };
					// gcc10+ doesn't like implicit template arguments
					pod::Vector4f translated = uf::matrix::multiply<float>( model, position );
					min.x = std::min( min.x, translated.x );
					max.x = std::max( max.x, translated.x );
					
					min.y = std::min( min.y, translated.y );
					max.y = std::max( max.y, translated.y );
				}
			} else if ( this->hasComponent<ext::Gui::glyph_mesh_t>() ) {
				auto& mesh = this->getComponent<ext::Gui::glyph_mesh_t>();
				for ( auto& vertex : mesh.vertices ) {
					pod::Vector4f position = { vertex.position.x, vertex.position.y, 0, 1 };
					// gcc10+ doesn't like implicit template arguments
					pod::Vector4f translated = uf::matrix::multiply<float>( model, position );
					min.x = std::min( min.x, translated.x );
					max.x = std::max( max.x, translated.x );
					
					min.y = std::min( min.y, translated.y );
					max.y = std::max( max.y, translated.y );
				}
			}
			metadata["box"]["min"]["x"] = min.x;
			metadata["box"]["min"]["y"] = min.y;
			metadata["box"]["max"]["x"] = max.x;
			metadata["box"]["max"]["y"] = max.y;
		/*
			int i = 0;
			for ( auto& vertex : vertices ) {
				pod::Vector4f position = { vertex.position.x, vertex.position.y, 0, 1 };
				// gcc10+ doesn't like implicit template arguments
				pod::Vector4f translated = uf::matrix::multiply<float>( model, position );
				// points.push_back( translated );
				metadata["box"][i]["x"] = translated.x;
				metadata["box"][i]["y"] = translated.y;
				++i;
			}
		*/
		} else {
			if ( !ext::json::isArray( metadata["color"] ) ) {
				metadata["color"][0] = 1.0f;
				metadata["color"][1] = 1.0f;
				metadata["color"][2] = 1.0f;
				metadata["color"][3] = 1.0f;
			}
			pod::Vector4 color = {
				metadata["color"][0].as<float>(),
				metadata["color"][1].as<float>(),
				metadata["color"][2].as<float>(),
				metadata["color"][3].as<float>()
			};
			if ( metadata["alpha"].is<double>() ) {
				color[3] *= metadata["alpha"].as<float>();
			}
			struct UniformDescriptor {
				struct {
					alignas(16) pod::Matrix4f model[2];
				} matrices;
				struct {
					alignas(16) pod::Vector4f offset;
					alignas(16) pod::Vector4f color;
					alignas(4) int32_t mode = 0;
					alignas(4) float depth = 0.0f;
					alignas(8) pod::Vector2f padding;
				} gui;
			};
			auto& uniforms = graphic.material.shaders.front().uniforms.front().get<UniformDescriptor>();
			uniforms.gui.offset = offset;
			uniforms.gui.color = color;
			uniforms.gui.mode = mode;
			if ( !ext::json::isNull( metadata["depth"] ) )
				uniforms.gui.depth = metadata["depth"].as<float>();
			else
				uniforms.gui.depth = 0;
			
			for ( std::size_t i = 0; i < 2; ++i ) {
				if ( metadata["world"].as<bool>() ) {
				/*
					pod::Transform<> flatten = uf::transform::flatten(camera.getTransform(), true);
					pod::Matrix4 rotation = uf::quaternion::matrix( uf::vector::multiply( { 1, 1, 1, -1 }, flatten.orientation) );
					uniforms.matrices.model[i] = ::matrix * rotation;
				*/
					auto& scene = uf::scene::getCurrentScene();
					auto& controller = scene.getController();
					auto& camera = controller.getComponent<uf::Camera>();

					pod::Transform<> flatten = uf::transform::flatten( this->getComponent<pod::Transform<>>() );
					auto model = uf::transform::model( flatten );
					auto view = camera.getView(i);
					auto projection = camera.getProjection(i);
					uniforms.matrices.model[i] = projection * view * model;
				} else {
					uf::Matrix4 translation, rotation, scale;
					pod::Transform<> flatten = uf::transform::flatten(transform, false);
					flatten.orientation.w *= -1;
					rotation = uf::quaternion::matrix( flatten.orientation );
					scale = uf::matrix::scale( scale, transform.scale );
					translation = uf::matrix::translate( uf::matrix::identity(), flatten.position );
					uniforms.matrices.model[i] = translation * scale * rotation;
				}
			}
			uniforms.gui.depth = 1.0f - uniforms.gui.depth; 
			// graphic.updateBuffer( uniforms, 0, false );
			graphic.material.shaders.front().updateBuffer( uniforms, 0, false );
			// calculate click box
			{
				auto& model = uniforms.matrices.model[0];
				auto& mesh = this->getComponent<uf::GuiMesh>();
				auto& vertices = mesh.vertices;
				pod::Vector2f min = {  1,  1 };
				pod::Vector2f max = { -1, -1 };
				for ( auto& vertex : vertices ) {
					pod::Vector4f position = { vertex.position.x, vertex.position.y, 0, 1 };
					// gcc10+ doesn't like implicit template arguments
					pod::Vector4f translated = uf::matrix::multiply<float>( model, position );
					min.x = std::min( min.x, translated.x );
					max.x = std::max( max.x, translated.x );
					
					min.y = std::min( min.y, translated.y );
					max.y = std::max( max.y, translated.y );
				}
				metadata["box"]["min"]["x"] = min.x;
				metadata["box"]["min"]["y"] = min.y;
				metadata["box"]["max"]["x"] = max.x;
				metadata["box"]["max"]["y"] = max.y;
			/*
				int i = 0;
				for ( auto& vertex : vertices ) {
					pod::Vector4f position = { vertex.position.x, vertex.position.y, 0, 1 };
					// gcc10+ doesn't like implicit template arguments
					pod::Vector4f translated = uf::matrix::multiply<float>( model, position );
					// points.push_back( translated );
					metadata["box"][0][i]["x"] = translated.x;
					metadata["box"][0][i]["y"] = translated.y;
					++i;
				}
			*/
			}
		}	
	}
}
void ext::GuiBehavior::destroy( uf::Object& self ){}
#undef this