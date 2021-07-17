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
	};
}
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

//	auto& mesh = this->getComponent<ext::Gui::mesh_t>();
	auto& transform = this->getComponent<pod::Transform<>>();
	uf::stl::vector<pod::Vertex_3F2F3F> vertices = {
		{ pod::Vector3f{ 1.0f, -1.0f, 0.0f}, pod::Vector2f{1.0f, 0.0f}, metadata.color },
		{ pod::Vector3f{-1.0f, -1.0f, 0.0f}, pod::Vector2f{0.0f, 0.0f}, metadata.color },
		{ pod::Vector3f{-1.0f,  1.0f, 0.0f}, pod::Vector2f{0.0f, 1.0f}, metadata.color },
	
		{ pod::Vector3f{-1.0f,  1.0f, 0.0f}, pod::Vector2f{0.0f, 1.0f}, metadata.color },
		{ pod::Vector3f{ 1.0f,  1.0f, 0.0f}, pod::Vector2f{1.0f, 1.0f}, metadata.color },
		{ pod::Vector3f{ 1.0f, -1.0f, 0.0f}, pod::Vector2f{1.0f, 0.0f}, metadata.color },
	};

	pod::Vector2f raidou = { 1, 1 };
	bool modified = false;

	if ( metadataJson["mode"].as<uf::stl::string>() == "flat" ) {
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
	if ( uf::matrix::reverseInfiniteProjection ) {
	} else {
		metadata.depth = -metadata.depth;
	}
	if ( metadataJson["flip uv"].as<bool>() ) for ( auto& v : vertices ) v.uv.y = 1 - v.uv.y;
	if ( metadata.depth != 0.0f ) for ( auto& v : vertices ) v.position.z = metadata.depth;

	if ( metadataJson["scaling"].is<uf::stl::string>() ) {
		uf::stl::string mode = metadataJson["scaling"].as<uf::stl::string>();
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
	if ( metadataJson["layer"].is<uf::stl::string>() ) {
		graphic.initialize( metadataJson["layer"].as<uf::stl::string>() );
	} else if ( !ext::json::isNull( metadataJson["gui layer"] ) && !metadataJson["gui layer"].as<bool>() ) {
		graphic.initialize();
	} else if ( metadataJson["gui layer"].is<uf::stl::string>() ) {
		graphic.initialize( metadataJson["gui layer"].as<uf::stl::string>() );
	} else {
		graphic.initialize( ::defaultRenderMode );
	}

	auto& mesh = this->getComponent<uf::Mesh>();
	mesh.bind<pod::Vertex_3F2F3F>();
	mesh.insertVertices( vertices );

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
UF_BEHAVIOR_TRAITS_CPP(ext::GuiBehavior, ticks = true, renders = false, multithread = false)
#define this (&self)
void ext::GuiBehavior::initialize( uf::Object& self ) {	
	auto& metadata = this->getComponent<ext::GuiBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();

	this->addHook( "object:UpdateMetadata.%UID%", [&](){
		metadata.deserialize(self, metadataJson);
	});
	metadata.deserialize(self, metadataJson);

	this->addHook( "asset:Load.%UID%", [&](ext::json::Value& json){
		uf::stl::string filename = json["filename"].as<uf::stl::string>();
		uf::stl::string category = json["category"].as<uf::stl::string>();

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
					this->queueHook(event["name"].as<uf::stl::string>(), payload, event["delay"].as<float>());
				} else {
					this->callHook(event["name"].as<uf::stl::string>(), payload );
				}
			}
		});
		this->addHook( "window:Mouse.Click", [&](ext::json::Value& json){
			if ( metadata.world ) return;
			if ( !metadata.box.min && !metadata.box.max ) return;
			bool down = json["mouse"]["state"].as<uf::stl::string>() == "Down";
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
					this->queueHook(event["name"].as<uf::stl::string>(), payload, event["delay"].as<float>());
				} else {
					this->callHook(event["name"].as<uf::stl::string>(), payload );
				}
			}
			return;
		});
		this->addHook( "window:Mouse.Moved", [&](ext::json::Value& json){
			if ( this->getUid() == 0 ) return;
			if ( !this->hasComponent<uf::Mesh>() ) return;

			if ( metadata.world ) return;
			if ( !metadata.box.min && !metadata.box.max ) return;

			bool clicked = false;
			bool down = json["mouse"]["state"].as<uf::stl::string>() == "Down";
			pod::Vector2ui position = uf::vector::decode( json["mouse"]["position"], pod::Vector2ui{} );
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
	if ( metadataJson["text settings"]["string"].is<uf::stl::string>() ) {
		if ( ext::json::isNull( ::defaultSettings["metadata"] ) ) ::defaultSettings.readFromFile(uf::io::root+"/entities/gui/text/string.json");

		ext::json::forEach(::defaultSettings["metadata"]["text settings"], [&]( const uf::stl::string& key, ext::json::Value& value ){
			if ( ext::json::isNull( metadataJson["text settings"][key] ) )
				metadataJson["text settings"][key] = value;
		});
		auto& metadataGlyph = this->getComponent<ext::GuiBehavior::GlyphMetadata>();

		this->addHook( "object:UpdateMetadata.%UID%", [&](){
			metadataGlyph.deserialize(self, metadataJson);
		});
	//	metadataGlyph.deserialize(self, metadataJson);
		
		this->addHook( "object:Reload.%UID%", [&](ext::json::Value& json){
			if ( json["old"]["text settings"]["string"] == json["new"]["text settings"]["string"] ) return;
			this->queueHook( "gui:UpdateString.%UID%");
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

			uf::stl::vector<pod::Vertex_3F2F3F> vertices;
			vertices.reserve( glyphs.size() * 6 );
			
			for ( auto& g : glyphs ) {
				auto glyphKey = std::to_string((uint64_t) g.code) + ";"+key;
				auto& glyph = ::glyphs.cache[font][glyphKey];
				auto hash = glyphHashMap[glyphKey];
				// add vertices
				vertices.emplace_back(pod::Vertex_3F2F3F{pod::Vector3f{ g.box.x,           g.box.y + g.box.h, 0 }, atlas.mapUv( pod::Vector2f{ 0.0f, 0.0f }, hash ), g.color});
				vertices.emplace_back(pod::Vertex_3F2F3F{pod::Vector3f{ g.box.x,           g.box.y          , 0 }, atlas.mapUv( pod::Vector2f{ 0.0f, 1.0f }, hash ), g.color});
				vertices.emplace_back(pod::Vertex_3F2F3F{pod::Vector3f{ g.box.x + g.box.w, g.box.y          , 0 }, atlas.mapUv( pod::Vector2f{ 1.0f, 1.0f }, hash ), g.color});
				vertices.emplace_back(pod::Vertex_3F2F3F{pod::Vector3f{ g.box.x,           g.box.y + g.box.h, 0 }, atlas.mapUv( pod::Vector2f{ 0.0f, 0.0f }, hash ), g.color});
				vertices.emplace_back(pod::Vertex_3F2F3F{pod::Vector3f{ g.box.x + g.box.w, g.box.y          , 0 }, atlas.mapUv( pod::Vector2f{ 1.0f, 1.0f }, hash ), g.color});
				vertices.emplace_back(pod::Vertex_3F2F3F{pod::Vector3f{ g.box.x + g.box.w, g.box.y + g.box.h, 0 }, atlas.mapUv( pod::Vector2f{ 1.0f, 0.0f }, hash ), g.color});
			}
			for ( size_t i = 0; i < vertices.size(); i += 6 ) {
				for ( size_t j = 0; j < 6; ++j ) {
					auto& vertex = vertices[i+j];
					vertex.position.x /= ext::gui::size.reference.x;
					vertex.position.y /= ext::gui::size.reference.y;
					vertex.position.z = metadata.depth;
				}
			}
			mesh.bind<pod::Vertex_3F2F3F>();
			mesh.insertVertices( vertices );

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
		this->callHook("gui:UpdateString.%UID%");
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
		auto& transform = this->getComponent<pod::Transform<>>();
		if ( !graphic.initialized ) return;

		bool isGlyph = this->hasComponent<ext::GuiBehavior::GlyphMetadata>();
	#if UF_USE_OPENGL
		auto model = uf::matrix::identity();
		auto uniformBuffer = graphic.getUniform();
		pod::Uniform& uniform = *((pod::Uniform*) graphic.device->getBuffer(uniformBuffer.buffer));
		if ( metadata.mode == 1 ) {
			uniform.modelView = transform.model; 
			uniform.projection = uf::matrix::identity();
		} else if ( metadata.mode == 2 ) {
			auto& scene = uf::scene::getCurrentScene();
			auto& controller = scene.getController();
			auto& camera = controller.getComponent<uf::Camera>();
			uniform.modelView = camera.getView() * uf::transform::model( transform );
			uniform.projection = camera.getProjection();
			model = uniform.modelView;
		} else if ( metadata.mode == 3 ) {
			pod::Transform<> flatten = uf::transform::flatten( transform );
			uniform.modelView = 
				uf::matrix::translate( uf::matrix::identity(), flatten.position ) *
				uf::matrix::scale( uf::matrix::identity(), flatten.scale ) *
				uf::quaternion::matrix( flatten.orientation ) *
				flatten.model;
			uniform.projection = camera.getProjection();
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
			uniform.projection = uf::matrix::identity();
		}
		graphic.updateUniform( &uniform, sizeof(uniform) );
	#elif UF_USE_VULKAN
		if ( !graphic.material.hasShader("vertex") ) return;

		auto& shader = graphic.material.getShader("vertex");
		auto& uniform = shader.getUniform("UBO");
		auto& uniforms = uniform.get<UniformDescriptor<>>(false); // skip validation

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
		// set glyph-based uniforms
		if ( isGlyph && uniform.size() == sizeof(::GlyphUniformDescriptor<>) ) {
			auto& uniforms = uniform.get<::GlyphUniformDescriptor<>>();
			auto& metadataGlyph = this->getComponent<ext::GuiBehavior::GlyphMetadata>();

			if ( metadataGlyph.sdf ) uniforms.gui.mode &= 1 << 1;
			if ( metadataGlyph.shadowbox ) uniforms.gui.mode &= 1 << 2;

			uniforms.glyph.stroke = metadataGlyph.stroke;
			uniforms.glyph.spread = metadataGlyph.spread;
			uniforms.glyph.weight = metadataGlyph.weight;
			uniforms.glyph.scale = metadataGlyph.scale;
		}
	//	shader.updateBuffer( uniform, shader.getUniformBuffer("UBO") );
		shader.updateUniform( "UBO", uniform );

		// calculate click box
		auto& model = uniforms.matrices[0].model;
	#endif
		pod::Vector2f min = {  1,  1 };
		pod::Vector2f max = { -1, -1 };

	/*
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
	*/
		
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
	serializer["gui"]["clicked"] = clicked;
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
	
	color = uf::vector::decode( serializer["gui"]["color"], color );
	world = serializer["gui"]["world"].as<bool>();
	if ( serializer["gui"]["only model"].as<bool>() ) mode = 1;
	else if ( serializer["gui"]["world"].as<bool>() ) mode = 2;
	else if ( serializer["gui"]["projection"].as<bool>() ) mode = 3;
	box.min = uf::vector::decode( serializer["box"]["min"], box.min );
	box.max = uf::vector::decode( serializer["box"]["max"], box.max );
	clicked = serializer["gui"]["clicked"].as<bool>();
	hovered = serializer["gui"]["hovered"].as<bool>();
	uv = uf::vector::decode( serializer["gui"]["uv"], uv );
	shader = serializer["gui"]["shader"].as<int>();
	depth = serializer["gui"]["depth"].as<float>();
	alpha = serializer["gui"]["alpha"].as<float>();
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
	/*this->*/font = serializer["gui"]["font"].as<uf::stl::string>();
	/*this->*/string = serializer["gui"]["string"].as<uf::stl::string>();
	
	/*this->*/scale = serializer["gui"]["scale"].as<float>();
	/*this->*/padding = uf::vector::decode( serializer["gui"]["padding"], /*this->*/padding );
	/*this->*/spread = serializer["gui"]["spread"].as<float>();
	/*this->*/size = serializer["gui"]["size"].as<float>();
	/*this->*/weight = serializer["gui"]["weight"].as<float>();
	/*this->*/sdf = serializer["gui"]["sdf"].as<bool>();
	/*this->*/shadowbox = serializer["gui"]["shadowbox"].as<bool>();
	/*this->*/stroke = uf::vector::decode( serializer["gui"]["stroke"], /*this->*/stroke );
	
	/*this->*/origin = serializer["gui"]["origin"].as<uf::stl::string>();
	/*this->*/align = serializer["gui"]["align"].as<uf::stl::string>();
	/*this->*/direction = serializer["gui"]["direction"].as<uf::stl::string>();
}
#undef this