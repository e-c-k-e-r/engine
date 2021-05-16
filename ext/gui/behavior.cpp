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
#if UF_USE_FREETYPE
	struct {
		ext::freetype::Glyph glyph;
		std::unordered_map<std::string, std::unordered_map<std::string, uf::Glyph>> cache;
	} glyphs;
#endif
	std::string defaultRenderMode = "Gui";
	uf::Serializer defaultSettings;

	std::vector<std::string> metadataKeys = {
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
	};
}
std::vector<pod::GlyphBox> ext::Gui::generateGlyphs( const std::string& _string ) {
	std::vector<pod::GlyphBox> gs;
#if UF_USE_FREETYPE
	uf::Object& gui = *this;
	std::string string = _string;
	auto& metadata = gui.getComponent<ext::GuiBehavior::Metadata>();
	auto& metadataGlyph = gui.getComponent<ext::GuiBehavior::GlyphMetadata>();
	auto& metadataJson = gui.getComponent<uf::Serializer>();

	metadata.deserialize();
	metadataGlyph.deserialize();

//	std::string font = uf::io::root+"/fonts/" + metadataJson["text settings"]["font"].as<std::string>();
	std::string font = uf::io::root+"/fonts/" + metadataGlyph.font;
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
			std::vector<pod::Vector3f> container;
			std::size_t index = 0;
		} colors;
	} stat;

	float scale = metadataGlyph.scale;
	pod::Vector3f color = metadata.color;
	stat.colors.container.push_back(color);
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
			std::string key = ""; {
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
		std::string key = ""; {
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
			std::cout << "Invalid color index `" << stat.colors.index <<  "` for string: " <<  string << ": (" << stat.colors.container.size() << ")" << std::endl;
			g.color = metadata.color;
		}
		gs.push_back(g);
	}
#endif
	return gs;
}

void ext::Gui::load( const uf::Image& image ) {
	auto& scene = uf::scene::getCurrentScene();
/*
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

	auto& mesh = this->getComponent<ext::Gui::mesh_t>();
	auto& transform = this->getComponent<pod::Transform<>>();
	mesh.vertices = {
		{ pod::Vector3f{ 1.0f, -1.0f, 0.0f}, pod::Vector2f{1.0f, 0.0f}, metadata.color },
		{ pod::Vector3f{-1.0f, -1.0f, 0.0f}, pod::Vector2f{0.0f, 0.0f}, metadata.color },
		{ pod::Vector3f{-1.0f,  1.0f, 0.0f}, pod::Vector2f{0.0f, 1.0f}, metadata.color },
	
		{ pod::Vector3f{-1.0f,  1.0f, 0.0f}, pod::Vector2f{0.0f, 1.0f}, metadata.color },
		{ pod::Vector3f{ 1.0f,  1.0f, 0.0f}, pod::Vector2f{1.0f, 1.0f}, metadata.color },
		{ pod::Vector3f{ 1.0f, -1.0f, 0.0f}, pod::Vector2f{1.0f, 0.0f}, metadata.color },
	};

	pod::Vector2f raidou = { 1, 1 };
	bool modified = false;

	if ( metadataJson["mode"].as<std::string>() == "flat" ) {
		if ( ext::json::isNull(metadataJson["projection"]) ) metadataJson["projection"] = false;
		if ( ext::json::isNull(metadataJson["flip uv"]) ) metadataJson["flip uv"] = true;
		if ( ext::json::isNull(metadataJson["front face"]) ) metadataJson["front face"] = "ccw";
	#if UF_USE_OPENGL
		metadataJson["cull mode"] = "back";
	//	metadataJson["depth test"]["test"] = false;
	//	metadataJson["depth test"]["write"] = true;
	//	if ( metadataJson["flip uv"].is<bool>() ) metadataJson["flip uv"] = !metadataJson["flip uv"].as<bool>();
	#endif
	} else {
		if ( ext::json::isNull(metadataJson["projection"]) ) metadataJson["projection"] = true;
		if ( ext::json::isNull(metadataJson["flip uv"]) ) metadataJson["flip uv"] = false;
		if ( ext::json::isNull(metadataJson["front face"]) ) metadataJson["front face"] = "cw";
	}
	metadataJson["cull mode"] = "none";
	graphic.descriptor.parse( metadataJson );
	
	if ( metadataJson["flip uv"].as<bool>() ) {
		for ( auto& v : mesh.vertices ) v.uv.y = 1 - v.uv.y;
	}
	if ( metadata.depth != 0.0f )
		for ( auto& v : mesh.vertices ) v.position.z = metadata.depth;
	if ( metadataJson["scaling"].is<std::string>() ) {
		std::string mode = metadataJson["scaling"].as<std::string>();
		if ( mode == "mesh" ) {
		//	raidou.x = (float) image.getDimensions().x / image.getDimensions().y;
			raidou.y = (float) image.getDimensions().y / image.getDimensions().x;
			modified = true;
		}
	} else if ( ext::json::isArray(metadataJson["scaling"]) ) {
		raidou = uf::vector::decode( metadataJson["scaling"], raidou );
		modified = true;
	}
	if ( modified ) {
		transform.scale.x = raidou.x;
		transform.scale.y = raidou.y;
	}
	if ( metadataJson["layer"].is<std::string>() ) {
		graphic.initialize( metadataJson["layer"].as<std::string>() );
	} else if ( !ext::json::isNull( metadataJson["gui layer"] ) && !metadataJson["gui layer"].as<bool>() ) {
		graphic.initialize();
	} else if ( metadataJson["gui layer"].is<std::string>() ) {
		graphic.initialize( metadataJson["gui layer"].as<std::string>() );
	} else {
		graphic.initialize( ::defaultRenderMode );
	}
	graphic.initializeMesh( mesh );
	struct {
		std::string vertex = uf::io::root+"/shaders/gui/base.vert.spv";
		std::string fragment = uf::io::root+"/shaders/gui/base.frag.spv";
	} filenames;
	std::string suffix = ""; {
		std::string _ = scene.getComponent<uf::Serializer>()["shaders"]["gui"]["suffix"].as<std::string>();
		if ( _ != "" ) suffix = _ + ".";
	}
	if ( metadataJson["shaders"]["vertex"].is<std::string>() ) filenames.vertex = metadataJson["shaders"]["vertex"].as<std::string>();
	if ( metadataJson["shaders"]["fragment"].is<std::string>() ) filenames.fragment = metadataJson["shaders"]["fragment"].as<std::string>();
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
#define this (&self)
void ext::GuiBehavior::initialize( uf::Object& self ) {	
	auto& metadata = this->getComponent<ext::GuiBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();

	metadata.serialize = [&]() {
		metadataJson["gui"]["color"] = uf::vector::encode( metadata.color );
		metadataJson["gui"]["world"] = metadata.world;
		if ( metadata.mode == 1 ) metadataJson["gui"]["only model"] = true;
		else if ( metadata.mode == 2 ) metadataJson["gui"]["world"] = true;
		else if ( metadata.mode == 3 ) metadataJson["gui"]["projection"] = true;
		metadataJson["box"]["min"] = uf::vector::encode( metadata.box.min );
		metadataJson["box"]["max"] = uf::vector::encode( metadata.box.max );
		metadataJson["gui"]["clicked"] = metadata.clicked;
		metadataJson["gui"]["hovered"] = metadata.hovered;
		metadataJson["gui"]["uv"] = uf::vector::encode( metadata.uv );
		metadataJson["gui"]["shader"] = metadata.shader;
		metadataJson["gui"]["depth"] = metadata.depth;
		metadataJson["gui"]["alpha"] = metadata.alpha;

		for ( auto& key : ::metadataKeys ) {
			if ( !ext::json::isNull( metadataJson[key] ) )
				metadataJson["gui"][key] = metadataJson[key];
			if ( !ext::json::isNull( metadataJson["text settings"][key] ) )
				metadataJson["gui"][key] = metadataJson["text settings"][key];
		}
	};
	metadata.deserialize = [&]() {
		for ( auto& key : ::metadataKeys ) {
			if ( !ext::json::isNull( metadataJson[key] ) )
				metadataJson["gui"][key] = metadataJson[key];
			if ( !ext::json::isNull( metadataJson["text settings"][key] ) )
				metadataJson["gui"][key] = metadataJson["text settings"][key];
		}
		
		metadata.color = uf::vector::decode( metadataJson["gui"]["color"], metadata.color );
		metadata.world = metadataJson["gui"]["world"].as<bool>();
		if ( metadataJson["gui"]["only model"].as<bool>() ) metadata.mode = 1;
		else if ( metadataJson["gui"]["world"].as<bool>() ) metadata.mode = 2;
		else if ( metadataJson["gui"]["projection"].as<bool>() ) metadata.mode = 3;
		metadata.box.min = uf::vector::decode( metadataJson["box"]["min"], metadata.box.min );
		metadata.box.max = uf::vector::decode( metadataJson["box"]["max"], metadata.box.max );
		metadata.clicked = metadataJson["gui"]["clicked"].as<bool>();
		metadata.hovered = metadataJson["gui"]["hovered"].as<bool>();
		metadata.uv = uf::vector::decode( metadataJson["gui"]["uv"], metadata.uv );
		metadata.shader = metadataJson["gui"]["shader"].as<int>();
		metadata.depth = metadataJson["gui"]["depth"].as<float>();
		metadata.alpha = metadataJson["gui"]["alpha"].as<float>();
		if ( metadataJson["gui"]["alpha"].is<float>() ) metadata.color[3] *= metadata.alpha;
	};
	this->addHook( "object:UpdateMetadata.%UID%", metadata.deserialize);
	metadata.deserialize();

	this->addHook( "asset:Load.%UID%", [&](ext::json::Value& json){
		std::string filename = json["filename"].as<std::string>();
		std::string category = json["category"].as<std::string>();

		if ( category != "" && category != "images" ) return;
		if ( category == "" && uf::io::extension(filename) != "png" && uf::io::extension(filename) != "jpg" && uf::io::extension(filename) != "jpeg" ) return;

		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
		if ( !assetLoader.has<uf::Image>(filename) ) return;
		auto& image = assetLoader.get<uf::Image>(filename);
		this->as<ext::Gui>().load( image );
	});		
	if ( metadataJson["system"]["clickable"].as<bool>() ) {
		uf::Timer<long long> clickTimer(false);
		clickTimer.start( uf::Time<>(-1000000) );
		if ( !clickTimer.running() ) clickTimer.start();
		this->addHook( "gui:Clicked.%UID%", [&](ext::json::Value& json){
			if ( ext::json::isObject( metadataJson["events"]["click"] ) ) {
				uf::Serializer event = metadataJson["events"]["click"];
				metadataJson["events"]["click"] = ext::json::array(); //Json::arrayValue;
				metadataJson["events"]["click"][0] = event;
			} else if ( !ext::json::isArray( metadataJson["events"]["click"] ) ) {
				this->getParent().as<uf::Object>().callHook("gui:Clicked.%UID%", json);
				return;
			}
			for ( int i = 0; i < metadataJson["events"]["click"].size(); ++i ) {
				uf::Serializer event = metadataJson["events"]["click"][i];
				uf::Serializer payload = event["payload"];
				if ( event["delay"].is<float>() ) {
					this->queueHook(event["name"].as<std::string>(), payload, event["delay"].as<float>());
				} else {
					this->callHook(event["name"].as<std::string>(), payload );
				}
			}
		});
		this->addHook( "window:Mouse.Click", [&](ext::json::Value& json){
			if ( metadata.world ) return;
			if ( !metadata.box.min && !metadata.box.max ) return;
			bool down = json["mouse"]["state"].as<std::string>() == "Down";
			bool clicked = false;
			if ( down ) {
				pod::Vector2ui position = uf::vector::decode( json["mouse"]["position"], pod::Vector2ui{} ); {
				//	position.x = json["mouse"]["position"]["x"].as<int>() > 0 ? json["mouse"]["position"]["x"].as<size_t>() : 0;
				//	position.y = json["mouse"]["position"]["y"].as<int>() > 0 ? json["mouse"]["position"]["y"].as<size_t>() : 0;
				}
				pod::Vector2f click; {
					click.x = (float) position.x / (float) ext::gui::size.current.x;
					click.y = (float) position.y / (float) ext::gui::size.current.y;

					click.x = (click.x * 2.0f) - 1.0f;
					click.y = (click.y * 2.0f) - 1.0f;

					float x = click.x;
					float y = click.y;


					if (json["invoker"] == "vr" ) {
						pod::Vector3f xy = uf::vector::decode( json["mouse"]["position"], pod::Vector2i{} );
						x = xy.x;
						y = xy.y;
					//	x = json["mouse"]["position"]["x"].as<float>();
					//	y = json["mouse"]["position"]["y"].as<float>();
					}
					clicked = ( metadata.box.min.x <= x && metadata.box.min.y <= y && metadata.box.max.x >= x && metadata.box.max.y >= y );
				}

			}
			metadata.clicked = clicked;
			metadataJson["clicked"] = clicked;
			if ( clicked ) this->callHook("gui:Clicked.%UID%", json);
			this->callHook("gui:Mouse.Clicked.%UID%", json);
		} );
	}
	if ( metadataJson["system"]["hoverable"].as<bool>() ) {
		uf::Timer<long long> hoverTimer(false);
		hoverTimer.start( uf::Time<>(-1000000) );
		this->addHook( "gui:Hovered.%UID%", [&](ext::json::Value& json){
			if ( ext::json::isObject( metadataJson["events"]["hover"] ) ) {
				uf::Serializer event = metadataJson["events"]["hover"];
				metadataJson["events"]["hover"] = ext::json::array(); //Json::arrayValue;
				metadataJson["events"]["hover"][0] = event;
			} else if ( !ext::json::isArray( metadataJson["events"]["hover"] ) ) {
				this->getParent().as<uf::Object>().callHook("gui:Clicked.%UID%", json);
				return;
			}
			for ( int i = 0; i < metadataJson["events"]["hover"].size(); ++i ) {
				uf::Serializer event = metadataJson["events"]["hover"][i];
				uf::Serializer payload = event["payload"];
				float delay = event["delay"].as<float>();
				if ( event["delay"].is<double>() ) {
					this->queueHook(event["name"].as<std::string>(), payload, event["delay"].as<float>());
				} else {
					this->callHook(event["name"].as<std::string>(), payload );
				}
			}
			return;
		});
		this->addHook( "window:Mouse.Moved", [&](ext::json::Value& json){
			if ( this->getUid() == 0 ) return;
			if ( !this->hasComponent<ext::Gui::mesh_t>() ) return;

			if ( metadata.world ) return;
			if ( !metadata.box.min && !metadata.box.max ) return;

			bool down = json["mouse"]["state"].as<std::string>() == "Down";
			bool clicked = false;
			pod::Vector2ui position = uf::vector::decode( json["mouse"]["position"], pod::Vector2ui{} ); {
			//	position.x = json["mouse"]["position"]["x"].as<int>() > 0 ? json["mouse"]["position"]["x"].as<size_t>() : 0;
			//	position.y = json["mouse"]["position"]["y"].as<int>() > 0 ? json["mouse"]["position"]["y"].as<size_t>() : 0;
			}
			pod::Vector2f click; {
				click.x = (float) position.x / (float) ext::gui::size.current.x;
				click.y = (float) position.y / (float) ext::gui::size.current.y;

				click.x = (click.x * 2.0f) - 1.0f;
				click.y = (click.y * 2.0f) - 1.0f;
				float x = click.x;
				float y = click.y;

				clicked = ( metadata.box.min.x <= x && metadata.box.min.y <= y && metadata.box.max.x >= x && metadata.box.max.y >= y );
			}
			metadata.hovered = clicked;
			metadataJson["hovered"] = clicked;

			if ( clicked && hoverTimer.elapsed().asDouble() >= 1 ) {
				hoverTimer.reset();
				this->callHook("gui:Hovered.%UID%", json);
			}
			this->callHook("gui:Mouse.Moved.%UID%", json);
		} );
	}
#if UF_USE_FREETYPE
	if ( metadataJson["text settings"]["string"].is<std::string>() ) {
		if ( ext::json::isNull( ::defaultSettings["metadata"] ) ) ::defaultSettings.readFromFile(uf::io::root+"/entities/gui/text/string.json");

		ext::json::forEach(::defaultSettings["metadata"]["text settings"], [&]( const std::string& key, ext::json::Value& value ){
			if ( ext::json::isNull( metadataJson["text settings"][key] ) )
				metadataJson["text settings"][key] = value;
		});

		auto& metadataGlyph = this->getComponent<ext::GuiBehavior::GlyphMetadata>();
		metadataGlyph.serialize = [&]() {
			metadataJson["text settings"]["font"] = metadataGlyph.font;
			metadataJson["text settings"]["string"] = metadataGlyph.string;

			metadataJson["text settings"]["scale"] = metadataGlyph.scale;
			metadataJson["text settings"]["padding"] = uf::vector::encode( metadataGlyph.padding );
			metadataJson["text settings"]["spread"] = metadataGlyph.spread;
			metadataJson["text settings"]["size"] = metadataGlyph.size;
			metadataJson["text settings"]["weight"] = metadataGlyph.weight;
			metadataJson["text settings"]["sdf"] = metadataGlyph.sdf;
			metadataJson["text settings"]["shadowbox"] = metadataGlyph.shadowbox;
			metadataJson["text settings"]["stroke"] = uf::vector::encode( metadataGlyph.stroke );

			metadataJson["text settings"]["origin"] = metadataGlyph.origin;
			metadataJson["text settings"]["align"] = metadataGlyph.align;
			metadataJson["text settings"]["direction"] = metadataGlyph.direction;

			for ( auto& key : ::metadataKeys ) {
				if ( !ext::json::isNull( metadataJson[key] ) )
					metadataJson["gui"][key] = metadataJson[key];
				if ( !ext::json::isNull( metadataJson["text settings"][key] ) )
					metadataJson["gui"][key] = metadataJson["text settings"][key];
			}
		};
		metadataGlyph.deserialize = [&]() {
			for ( auto& key : ::metadataKeys ) {
				if ( !ext::json::isNull( metadataJson[key] ) ) metadataJson["gui"][key] = metadataJson[key];
				if ( !ext::json::isNull( metadataJson["text settings"][key] ) ) metadataJson["gui"][key] = metadataJson["text settings"][key];
			}
			metadataGlyph.font = metadataJson["gui"]["font"].as<std::string>();
			metadataGlyph.string = metadataJson["gui"]["string"].as<std::string>();
			
			metadataGlyph.scale = metadataJson["gui"]["scale"].as<float>();
			metadataGlyph.padding = uf::vector::decode( metadataJson["gui"]["padding"], metadataGlyph.padding );
			metadataGlyph.spread = metadataJson["gui"]["spread"].as<float>();
			metadataGlyph.size = metadataJson["gui"]["size"].as<float>();
			metadataGlyph.weight = metadataJson["gui"]["weight"].as<float>();
			metadataGlyph.sdf = metadataJson["gui"]["sdf"].as<bool>();
			metadataGlyph.shadowbox = metadataJson["gui"]["shadowbox"].as<bool>();
			metadataGlyph.stroke = uf::vector::decode( metadataJson["gui"]["stroke"], metadataGlyph.stroke );
			
			metadataGlyph.origin = metadataJson["gui"]["origin"].as<std::string>();
			metadataGlyph.align = metadataJson["gui"]["align"].as<std::string>();
			metadataGlyph.direction = metadataJson["gui"]["direction"].as<std::string>();
		};
		this->addHook( "object:UpdateMetadata.%UID%", metadataGlyph.deserialize);
	#if 0
		metadataGlyph.deserialize();
		{
			std::vector<pod::GlyphBox> glyphs = this->as<ext::Gui>().generateGlyphs();
			
			std::string font = uf::io::root+"/fonts/" + metadataGlyph.font;
			std::string key = ""; {
				key += std::to_string(metadataGlyph.padding[0]) + ",";
				key += std::to_string(metadataGlyph.padding[1]) + ";";
				key += std::to_string(metadataGlyph.spread) + ";";
				key += std::to_string(metadataGlyph.size) + ";";
				key += metadataGlyph.font + ";";
				key += std::to_string(metadataGlyph.sdf);
			}
			auto& scene = uf::scene::getCurrentScene();
			auto& atlas = this->getComponent<uf::Atlas>();
			auto& images = atlas.getImages();
			auto& mesh = this->getComponent<ext::Gui::glyph_mesh_t>();
			auto& graphic = this->getComponent<uf::Graphic>();

			if ( metadataJson["mode"].as<std::string>() == "flat" ) {
				if ( ext::json::isNull(metadataJson["projection"]) ) metadataJson["projection"] = false;
				if ( ext::json::isNull(metadataJson["front face"]) ) metadataJson["front face"] = "ccw";
			#if UF_USE_OPENGL
				metadataJson["cull mode"] = "front";
			//	metadataJson["depth test"]["test"] = false;
			//	metadataJson["depth test"]["write"] = true;
			#endif
			} else {
				if ( ext::json::isNull(metadataJson["projection"]) ) metadataJson["projection"] = true;
				if ( ext::json::isNull(metadataJson["front face"]) ) metadataJson["front face"] = "cw";
			}
			metadataJson["cull mode"] = "none";
			graphic.descriptor.parse( metadataJson );

			mesh.vertices.reserve( glyphs.size() * 6 );
			std::unordered_map<std::string, std::string> glyphHashMap;
			for ( auto& g : glyphs ) {
				auto glyphKey = std::to_string((uint64_t) g.code) + ";"+key;
				auto& glyph = ::glyphs.cache[font][glyphKey];
			#if UF_USE_OPENGL
				const uint8_t* buffer = glyph.getBuffer();
				uf::Image::container_t pixels;
				std::size_t len = glyph.getSize().x * glyph.getSize().y;
				pixels.reserve( len * 4 );
				for ( size_t i = 0; i < len; ++i ) {
					pixels.emplace_back( buffer[i] );
					pixels.emplace_back( buffer[i] );
					pixels.emplace_back( buffer[i] );
					pixels.emplace_back( buffer[i] );
				}
				glyphHashMap[glyphKey] = atlas.addImage( &pixels[0], glyph.getSize(), 8, 4, true );
			#else
				glyphHashMap[glyphKey] = atlas.addImage( glyph.getBuffer(), glyph.getSize(), 8, 1, true );
			#endif
			}

			atlas.generate();

			float scale = metadataGlyph.scale;
			auto& transform = this->getComponent<pod::Transform<>>();
			transform.scale.x = scale;
			transform.scale.y = scale;

			for ( auto& g : glyphs ) {
				auto glyphKey = std::to_string((uint64_t) g.code) + ";"+key;
				auto& glyph = ::glyphs.cache[font][glyphKey];
				auto hash = glyphHashMap[glyphKey];
				// add vertices
				mesh.vertices.push_back({pod::Vector3f{ g.box.x,           g.box.y + g.box.h, 0 }, atlas.mapUv( pod::Vector2f{ 0.0f, 0.0f }, hash ), g.color});
				mesh.vertices.push_back({pod::Vector3f{ g.box.x,           g.box.y          , 0 }, atlas.mapUv( pod::Vector2f{ 0.0f, 1.0f }, hash ), g.color});
				mesh.vertices.push_back({pod::Vector3f{ g.box.x + g.box.w, g.box.y          , 0 }, atlas.mapUv( pod::Vector2f{ 1.0f, 1.0f }, hash ), g.color});
				mesh.vertices.push_back({pod::Vector3f{ g.box.x,           g.box.y + g.box.h, 0 }, atlas.mapUv( pod::Vector2f{ 0.0f, 0.0f }, hash ), g.color});
				mesh.vertices.push_back({pod::Vector3f{ g.box.x + g.box.w, g.box.y          , 0 }, atlas.mapUv( pod::Vector2f{ 1.0f, 1.0f }, hash ), g.color});
				mesh.vertices.push_back({pod::Vector3f{ g.box.x + g.box.w, g.box.y + g.box.h, 0 }, atlas.mapUv( pod::Vector2f{ 1.0f, 0.0f }, hash ), g.color});
			}
			for ( size_t i = 0; i < mesh.vertices.size(); i += 6 ) {
				for ( size_t j = 0; j < 6; ++j ) {
					auto& vertex = mesh.vertices[i+j];
					vertex.position.x /= ext::gui::size.reference.x;
					vertex.position.y /= ext::gui::size.reference.y;
					vertex.position.z = metadata.depth;
					vertex.uv.y = 1 - vertex.uv.y;
				}
			}

			auto& texture = graphic.material.textures.emplace_back();
			texture.loadFromImage( atlas.getAtlas() );
			graphic.initialize( ::defaultRenderMode );
			graphic.initializeMesh( mesh );

			std::string suffix = ""; {
				std::string _ = scene.getComponent<uf::Serializer>()["shaders"]["gui"]["suffix"].as<std::string>();
				if ( _ != "" ) suffix = _ + ".";
			}
			struct {
				std::string vertex = uf::io::root+"/shaders/gui/text.vert.spv";
				std::string fragment = uf::io::root+"/shaders/gui/text.frag.spv";
			} filenames;
			if ( metadataJson["shaders"]["vertex"].is<std::string>() ) filenames.vertex = metadataJson["shaders"]["vertex"].as<std::string>();
			if ( metadataJson["shaders"]["fragment"].is<std::string>() ) filenames.fragment = metadataJson["shaders"]["fragment"].as<std::string>();
			else if ( suffix != "" ) filenames.fragment = uf::io::root+"/shaders/gui/text."+suffix+"frag.spv";

			graphic.material.initializeShaders({
				{filenames.vertex, uf::renderer::enums::Shader::VERTEX},
				{filenames.fragment, uf::renderer::enums::Shader::FRAGMENT},
			});
		}
	#endif
		this->addHook( "object:Reload.%UID%", [&](ext::json::Value& json){
			if ( json["old"]["text settings"]["string"] == json["new"]["text settings"]["string"] ) return;
			this->queueHook( "gui:UpdateString.%UID%");
		});
		this->addHook( "gui:UpdateString.%UID%", [&](ext::json::Value& json){
			ext::json::forEach(::defaultSettings["metadata"]["text settings"], [&]( const std::string& key, ext::json::Value& value ){
				if ( ext::json::isNull( metadataJson["text settings"][key] ) )
					metadataJson["text settings"][key] = value;
			});
			if ( json["string"].is<std::string>() ) metadataJson["text settings"]["string"] = json["string"];
			std::string string = metadataJson["text settings"]["string"].as<std::string>();


			auto& metadataGlyph = this->getComponent<ext::GuiBehavior::GlyphMetadata>();
			bool first = metadataGlyph.string == "" || json["first"].as<bool>();
			metadataGlyph.deserialize();

			std::vector<pod::GlyphBox> glyphs = this->as<ext::Gui>().generateGlyphs( string );
			std::string font = uf::io::root+"/fonts/" + metadataGlyph.font;
			std::string key = ""; {
				key += std::to_string(metadataGlyph.padding[0]) + ",";
				key += std::to_string(metadataGlyph.padding[1]) + ";";
				key += std::to_string(metadataGlyph.spread) + ";";
				key += std::to_string(metadataGlyph.size) + ";";
				key += metadataGlyph.font + ";";
				key += std::to_string(metadataGlyph.sdf);
			}
			auto& scene = uf::scene::getCurrentScene();
			auto& mesh = this->getComponent<ext::Gui::glyph_mesh_t>();
			auto& graphic = this->getComponent<uf::Graphic>();
			auto& atlas = this->getComponent<uf::Atlas>();
			auto& images = atlas.getImages();
			if ( !first ) {
				atlas.clear();
				mesh.vertices.clear();
				mesh.indices.clear();
				
				for ( auto& texture : graphic.material.textures ) texture.destroy();
				graphic.material.textures.clear();
			}
			mesh.vertices.reserve( glyphs.size() * 6 );

			std::unordered_map<std::string, std::string> glyphHashMap;
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

			for ( auto& g : glyphs ) {
				auto glyphKey = std::to_string((uint64_t) g.code) + ";"+key;
				auto& glyph = ::glyphs.cache[font][glyphKey];
				auto hash = glyphHashMap[glyphKey];
				// add vertices
				mesh.vertices.push_back({pod::Vector3f{ g.box.x,           g.box.y + g.box.h, 0 }, atlas.mapUv( pod::Vector2f{ 0.0f, 0.0f }, hash ), g.color});
				mesh.vertices.push_back({pod::Vector3f{ g.box.x,           g.box.y          , 0 }, atlas.mapUv( pod::Vector2f{ 0.0f, 1.0f }, hash ), g.color});
				mesh.vertices.push_back({pod::Vector3f{ g.box.x + g.box.w, g.box.y          , 0 }, atlas.mapUv( pod::Vector2f{ 1.0f, 1.0f }, hash ), g.color});
				mesh.vertices.push_back({pod::Vector3f{ g.box.x,           g.box.y + g.box.h, 0 }, atlas.mapUv( pod::Vector2f{ 0.0f, 0.0f }, hash ), g.color});
				mesh.vertices.push_back({pod::Vector3f{ g.box.x + g.box.w, g.box.y          , 0 }, atlas.mapUv( pod::Vector2f{ 1.0f, 1.0f }, hash ), g.color});
				mesh.vertices.push_back({pod::Vector3f{ g.box.x + g.box.w, g.box.y + g.box.h, 0 }, atlas.mapUv( pod::Vector2f{ 1.0f, 0.0f }, hash ), g.color});
			}
			for ( size_t i = 0; i < mesh.vertices.size(); i += 6 ) {
				for ( size_t j = 0; j < 6; ++j ) {
					auto& vertex = mesh.vertices[i+j];
					vertex.position.x /= ext::gui::size.reference.x;
					vertex.position.y /= ext::gui::size.reference.y;
					vertex.position.z = metadata.depth;
				}
			}

			auto& texture = graphic.material.textures.emplace_back();
			texture.loadFromImage( atlas.getAtlas() );

			if ( first ) {
				graphic.initialize( ::defaultRenderMode );
				graphic.initializeMesh( mesh );

				std::string suffix = ""; {
					std::string _ = scene.getComponent<uf::Serializer>()["shaders"]["gui"]["suffix"].as<std::string>();
					if ( _ != "" ) suffix = _ + ".";
				}
				struct {
					std::string vertex = uf::io::root+"/shaders/gui/text.vert.spv";
					std::string fragment = uf::io::root+"/shaders/gui/text.frag.spv";
				} filenames;
				if ( metadataJson["shaders"]["vertex"].is<std::string>() ) filenames.vertex = metadataJson["shaders"]["vertex"].as<std::string>();
				if ( metadataJson["shaders"]["fragment"].is<std::string>() ) filenames.fragment = metadataJson["shaders"]["fragment"].as<std::string>();
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
		this->callHook("gui:UpdateString.%UID%");
	}
#endif
}
void ext::GuiBehavior::tick( uf::Object& self ) {
#if 0
	uf::Serializer& metadataJson = this->getComponent<uf::Serializer>();
	if ( metadataJson["text settings"]["fade in speed"].is<double>() && !metadataJson["system"]["faded in"].as<bool>() ) {
		float speed = metadataJson["text settings"]["fade in speed"].as<float>();
		float alpha = metadataJson["text settings"]["color"][3].as<float>();
		speed *= uf::physics::time::delta;
		if ( alpha < 1 && alpha + speed > 1 ) {
			alpha = 1;
			speed = 0;

			metadataJson["system"]["faded in"] = true;
		}
		if ( alpha + speed <= 1 ) {
			metadataJson["text settings"]["color"][3] = alpha + speed;
		}
	}
#endif
}
template<size_t N = uf::renderer::settings::maxViews>
struct UniformDescriptor {
	struct {
		alignas(16) pod::Matrix4f model[N];
	} matrices;
	struct {
		alignas(16) pod::Vector4f offset;
		alignas(16) pod::Vector4f color;
		alignas(4) int32_t mode = 0;
		alignas(4) float depth = 0.0f;
		alignas(8) pod::Vector2f padding;
	} gui;
};
template<size_t N = uf::renderer::settings::maxViews>
struct GlyphUniformDescriptor {
	struct {
		alignas(16) pod::Matrix4f model[N];
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
void ext::GuiBehavior::render( uf::Object& self ){
	auto& metadata = this->getComponent<ext::GuiBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();
	/* Update uniforms */ if ( this->hasComponent<uf::Graphic>() ) {
		auto& scene = uf::scene::getCurrentScene();
		auto& graphic = this->getComponent<uf::Graphic>();
		auto& controller = scene.getController();
		auto& camera = controller.getComponent<uf::Camera>();
		auto& transform = this->getComponent<pod::Transform<>>();
		if ( !graphic.initialized ) return;

		bool isGlyph = this->hasComponent<ext::GuiBehavior::GlyphMetadata>(); //metadataGlyph.string != "";
	#if UF_USE_OPENGL
		auto model = uf::matrix::identity();
		auto uniformBuffer = graphic.getUniform();
		pod::Uniform& uniform = *((pod::Uniform*) graphic.device->getBuffer(uniformBuffer.buffer));
		uniform.projection = uf::matrix::identity();
		if ( metadata.mode == 1 ) {
			uniform.modelView = transform.model; 
		} else if ( metadata.mode == 2 ) {
			auto& scene = uf::scene::getCurrentScene();
			auto& controller = scene.getController();
			auto& camera = controller.getComponent<uf::Camera>();
			uniform.projection = camera.getProjection();
			uniform.modelView = camera.getView() * uf::transform::model( transform );
			model = uniform.modelView;
		} else if ( metadata.mode == 3 ) {
			pod::Transform<> flatten = uf::transform::flatten( transform );
			uniform.projection = camera.getProjection();
			uniform.modelView = 
				uf::matrix::translate( uf::matrix::identity(), flatten.position ) *
				uf::matrix::scale( uf::matrix::identity(), flatten.scale ) *
				uf::quaternion::matrix( flatten.orientation ) *
				flatten.model;
			model = uniform.modelView;
		} else {
			pod::Transform<> flatten = uf::transform::flatten( transform );
			model = 
				uf::matrix::translate( uf::matrix::identity(), flatten.position ) *
				uf::matrix::scale( uf::matrix::identity(), flatten.scale ) *
				uf::quaternion::matrix( flatten.orientation ) *
				flatten.model;
			
			flatten.position.y = -flatten.position.y;
			if ( isGlyph ) flatten.scale.y = -flatten.scale.y;

			flatten.position.z = 1;
			uniform.modelView = 
				uf::matrix::translate( uf::matrix::identity(), flatten.position ) *
				uf::matrix::scale( uf::matrix::identity(), flatten.scale ) *
				uf::quaternion::matrix( flatten.orientation ) *
				flatten.model;
		}
		graphic.updateUniform( &uniform, sizeof(uniform) );
	#else
		if ( !graphic.material.hasShader("vertex") ) return;

		auto& shader = graphic.material.getShader("vertex");
		auto& uniform = shader.getUniform("UBO");
		auto& uniforms = uniform.get<UniformDescriptor<>>();

		uniforms.gui.offset = metadata.uv;
		uniforms.gui.mode = metadata.shader;
		uniforms.gui.depth = metadata.depth;

		uniforms.gui.depth = 1 - uniforms.gui.depth;

		uniforms.gui.color = metadata.color;

		for ( std::size_t i = 0; i < uf::renderer::settings::maxViews; ++i ) {
			if ( metadata.mode == 1 ) {
				uniforms.matrices.model[i] = transform.model; 
			} else if ( metadata.mode == 2 ) {
				auto& scene = uf::scene::getCurrentScene();
				auto& controller = scene.getController();
				auto& camera = controller.getComponent<uf::Camera>();
				uniforms.matrices.model[i] = camera.getProjection(i) * camera.getView(i) * uf::transform::model( transform );
			} else if ( metadata.mode == 3 ) {
				pod::Transform<> flatten = uf::transform::flatten( transform );
				uniforms.matrices.model[i] = 
					camera.getProjection(i) *
					uf::matrix::translate( uf::matrix::identity(), flatten.position ) *
					uf::matrix::scale( uf::matrix::identity(), flatten.scale ) *
					uf::quaternion::matrix( flatten.orientation ) *
					flatten.model;
			} else {
				pod::Transform<> flatten = uf::transform::flatten( transform );
				uniforms.matrices.model[i] = 
					uf::matrix::translate( uf::matrix::identity(), flatten.position ) *
					uf::matrix::scale( uf::matrix::identity(), flatten.scale ) *
					uf::quaternion::matrix( flatten.orientation ) *
					flatten.model;
			}
		}

		// set glyph-based uniforms
		if ( isGlyph ) {
			auto& uniforms = uniform.get<GlyphUniformDescriptor<>>();
			auto& metadataGlyph = this->getComponent<ext::GuiBehavior::GlyphMetadata>();

			uniforms.gui.sdf = metadataGlyph.sdf;
			uniforms.gui.shadowbox = metadataGlyph.shadowbox;
			uniforms.gui.stroke = metadataGlyph.stroke;
			uniforms.gui.weight = metadataGlyph.weight;
			uniforms.gui.spread = metadataGlyph.spread;
			uniforms.gui.scale = metadataGlyph.scale;
		}
		shader.updateUniform( "UBO", uniform );

		// calculate click box
		auto& model = uniforms.matrices.model[0];
	#endif
		pod::Vector2f min = {  1,  1 };
		pod::Vector2f max = { -1, -1 };

		if ( this->hasComponent<ext::Gui::mesh_t>() ) {
			auto& mesh = this->getComponent<ext::Gui::mesh_t>();
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
		
		metadata.box.min.x = min.x;
		metadata.box.min.y = min.y;
		metadata.box.max.x = max.x;
		metadata.box.max.y = max.y;
	}
}
void ext::GuiBehavior::destroy( uf::Object& self ){}
#undef this