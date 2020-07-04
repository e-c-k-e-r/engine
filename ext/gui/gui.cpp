#include "gui.h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/window/window.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/utils/string/ext.h>

#include <uf/utils/text/glyph.h>
#include "../world/world.h"
#include "../asset/asset.h"

#include <unordered_map>
#include <locale>
#include <codecvt>

#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/graphics/gui.h>

#include <uf/utils/http/http.h>
#include <uf/utils/audio/audio.h>
#include <sys/stat.h>
#include <fstream>

#include "menu.h"
#include "battle.h"

#include <regex>

namespace {
	struct {
		ext::freetype::Glyph glyph;
		std::unordered_map<std::string, std::unordered_map<std::string, uf::Glyph>> cache;
	} glyphs;

	struct {
		pod::Vector2ui current = {
			ext::vulkan::width,
			ext::vulkan::height,
		};
		pod::Vector2ui reference = {
			ext::vulkan::width,
			ext::vulkan::height,
		};
	} size;

	std::mutex mutex;
	uint64_t uid = 0;
	struct Job {
		uint64_t uid;
	};
	std::queue<Job> jobs;

	struct GlyphDescriptor {
		struct {
			alignas(16) pod::Matrix4f model[2];
		} matrices;
		struct {
			alignas(16) pod::Vector4f offset;
			alignas(16) pod::Vector4f color;
			int32_t mode = 0;
			float depth = 0.0f;
			int32_t sdf = false;
			int32_t shadowbox = false;
			alignas(16) pod::Vector4f stroke;
			float weight;
			int32_t spread;
			float scale;
		} gui;
	};

	struct GlyphBox {
		struct {
			float x, y, w, h;
		} box;
		uint64_t code;
		pod::Vector3f color;
	};

	pod::Matrix4 matrix;

	float area(int x1, int y1, int x2, int y2, int x3, int y3) { 
		return abs((x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2)) / 2.0); 
	}

	std::vector<::GlyphBox> generateGlyphs( ext::Gui& gui, std::string string = "" ) {
		uf::Serializer& metadata = gui.getComponent<uf::Serializer>();
		std::string font = "./data/fonts/" + metadata["text settings"]["font"].asString();
		if ( ::glyphs.cache[font].empty() ) {
			ext::freetype::initialize( ::glyphs.glyph, font );
		}
		if ( string == "" ) {
			string = metadata["text settings"]["string"].asString();
		}
		// uf::Glyph& glyph = this->getComponent<uf::Glyph>();
		pod::Transform<>& transform = gui.getComponent<pod::Transform<>>();
		std::vector<::GlyphBox> gs;
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

		float scale = metadata["text settings"]["scale"].asFloat();
		float scaleX = 2.0f / (float) ::size.reference.x;
		float scaleY = 2.0f / (float) ::size.reference.y;
		if ( ::size.current.x != 0 && ::size.current.y != 0 ) {
		//	scaleX = 2.0f / (float) ::size.current.x;
		//	scaleY = 2.0f / (float) ::size.current.y;
		}
		scaleX *= scale;
		scaleY *= scale;

		{
			pod::Vector3f color = {
				metadata["text settings"]["color"][0].asFloat(),
				metadata["text settings"]["color"][1].asFloat(),
				metadata["text settings"]["color"][2].asFloat(),
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
			while ( matched = std::regex_search( text, match, regex ) && --maxTries > 0 ) {
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
					key += metadata["text settings"]["padding"][0].asString() + ",";
					key += metadata["text settings"]["padding"][1].asString() + ";";
					key += metadata["text settings"]["spread"].asString() + ";";
					key += metadata["text settings"]["size"].asString() + ";";
					key += metadata["text settings"]["font"].asString() + ";";
					key += metadata["text settings"]["sdf"].asString();
				}
				uf::Glyph& glyph = ::glyphs.cache[font][key];

				if ( !glyph.generated() ) {
					glyph.setPadding( { metadata["text settings"]["padding"][0].asUInt(), metadata["text settings"]["padding"][1].asUInt() } );
					glyph.setSpread( metadata["text settings"]["spread"].asUInt() );
					if ( metadata["text settings"]["sdf"].asBool() ) glyph.useSdf(true);
					glyph.generate( ::glyphs.glyph, c, metadata["text settings"]["size"].asInt() );
				}
				
				stat.biggest.x = std::max( (float) stat.biggest.x, glyph.getSize().x * scaleX);
				stat.biggest.y = std::max( (float) stat.biggest.y, glyph.getSize().y * scaleY);

				stat.average.sum += glyph.getSize().x * scaleX;
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
					key += metadata["text settings"]["padding"][0].asString() + ",";
					key += metadata["text settings"]["padding"][1].asString() + ";";
					key += metadata["text settings"]["spread"].asString() + ";";
					key += metadata["text settings"]["size"].asString() + ";";
					key += metadata["text settings"]["font"].asString() + ";";
					key += metadata["text settings"]["sdf"].asString();
				}
				uf::Glyph& glyph = ::glyphs.cache[font][key];
				::GlyphBox g;


				g.box.w = glyph.getSize().x * scaleX;
				g.box.h = glyph.getSize().y * scaleY;
			//	stat.cursor.x -= glyph.getPadding().x * scaleX;
				g.box.x = (stat.cursor.x + stat.origin.x) + (glyph.getBearing().x * scaleX);
				if ( metadata["text settings"]["wrap"].asBool() && g.box.x + g.box.w >= 2 ) {
					stat.cursor.y += stat.biggest.y;
					g.box.x -= stat.cursor.x; stat.cursor.x = 0;
				}
			//	g.box.y = (stat.cursor.y + stat.origin.y) - (glyph.getBearing().y * scaleY);
		//		g.box.y = (stat.cursor.y + stat.origin.y);
				g.box.y = (stat.cursor.y + stat.origin.y) - (glyph.getBearing().y * scaleY) + (glyph.getSize().y * scaleY);
			//	g.box.y = (stat.cursor.y + stat.origin.y) - (glyph.getSize().y - glyph.getBearing().y * scaleY);

				stat.cursor.x += (glyph.getAdvance().x) * scaleX;
				stat.cursor.x += metadata["text settings"]["kerning"].asFloat() * scaleX;
			//	stat.cursor.x += glyph.getPadding().x * scaleX;
			//	stat.cursor.y -= (glyph.getAdvance().y) * scaleY;
			//	stat.cursor.y -= glyph.getPadding().y * scaleY;
				
				stat.box.w = std::max( (float) stat.box.w, (float) (stat.cursor.x + g.box.w) );
				stat.box.h = std::max( (float) stat.box.h, (float) (stat.cursor.y + g.box.h) );

			}

			stat.origin.x = ( !metadata["text settings"]["world"].asBool() && transform.position.x != (int) transform.position.x ) ? transform.position.x * ::size.current.x : transform.position.x;
			stat.origin.y = ( !metadata["text settings"]["world"].asBool() && transform.position.y != (int) transform.position.y ) ? transform.position.y * ::size.current.y : transform.position.y;

		
			if ( metadata["text settings"]["origin"].isArray() ) {
				stat.origin.x = metadata["text settings"]["origin"][0].asInt();
				stat.origin.y = metadata["text settings"]["origin"][1].asInt();
			}
			else if ( metadata["text settings"]["origin"] == "top" ) stat.origin.y = ::size.current.y - stat.origin.y - stat.biggest.y;// else stat.origin.y = stat.origin.y;
			if ( metadata["text settings"]["align"] == "right" ) stat.origin.x = ::size.current.x - stat.origin.x - stat.box.w;// else stat.origin.x = stat.origin.x;
			else if ( metadata["text settings"]["align"] == "center" )
				stat.origin.x -= stat.box.w * 0.5f;
			/*
				std::cout << "String: " << metadata["text settings"]["string"].asString()
					<< "\n\tWidth: " << stat.box.w << " | " << (stat.origin.x -= stat.box.w * 0.5f)
				<< std::endl;
			*/
		}

		stat.biggest.x *= 2;
		stat.biggest.y *= 2;

		// Render Glyphs
		stat.cursor.x = 0;
		stat.cursor.y = 0;

		for ( auto it = str.begin(); it != str.end(); ++it ) {
			unsigned long c = *it; if ( c == '\n' ) {
				if ( metadata["text settings"]["direction"] == "down" ) stat.cursor.y += stat.biggest.y; else stat.cursor.y -= stat.biggest.y;
				stat.cursor.x = 0;
				continue;
			} else if ( c == '\t' ) {
				// Fixed movement vs Real Tabbing
				if ( false ) {
					stat.cursor.x += stat.average.tab;
				} else {
					stat.cursor.x = ((stat.cursor.x / stat.average.tab) + (1 * scaleX)) * stat.average.tab;
				}
				continue;
			} else if ( c == ' ' ) {
				stat.cursor.x += stat.average.tab / 2.0f;
				continue;
			} else if ( c == COLORCONTROL ) {
				++stat.colors.index;
				continue;
			}
			std::string key = ""; {
				key += std::to_string(c) + ";";
				key += metadata["text settings"]["padding"][0].asString() + ",";
				key += metadata["text settings"]["padding"][1].asString() + ";";
				key += metadata["text settings"]["spread"].asString() + ";";
				key += metadata["text settings"]["size"].asString() + ";";
				key += metadata["text settings"]["font"].asString() + ";";
				key += metadata["text settings"]["sdf"].asString();
			}
			uf::Glyph& glyph = ::glyphs.cache[font][key];

			::GlyphBox g;
			g.code = c;

			g.box.w = glyph.getSize().x * scaleX;
			g.box.h = glyph.getSize().y * scaleY;
			g.box.x = (stat.cursor.x + stat.origin.x) + (glyph.getBearing().x * scaleX);
			if ( metadata["text settings"]["wrap"].asBool() && g.box.x + g.box.w >= 2 ) {
				if ( metadata["text settings"]["direction"] == "down" ) stat.cursor.y += stat.biggest.y; else stat.cursor.y -= stat.biggest.y;
				g.box.x -= stat.cursor.x; stat.cursor.x = 0;
			}
		//	g.box.y = (stat.cursor.y + stat.origin.y) - (glyph.getBearing().y * scaleY);
			g.box.y = (stat.cursor.y + stat.origin.y) - (glyph.getBearing().y * scaleY) + (glyph.getSize().y * scaleY);
//			g.box.y = (stat.cursor.y + stat.origin.y) - ((glyph.getSize().y - glyph.getBearing().y) * scaleY);
//			std::cout << "Glyph: " << (char) c << " " << glyph.getBearing().y << ", " << glyph.getSize().y << ", " << g.box.y << std::endl;
		//	g.box.y = (stat.cursor.y + stat.origin.y) - (glyph.getSize().y - glyph.getBearing().y * scaleY);

			try {
				g.color = stat.colors.container.at(stat.colors.index);
			} catch ( ... ) {
				std::cout << "Invalid color index `" << stat.colors.index <<  "` for string: " <<  string << ": (" << stat.colors.container.size() << ")" << std::endl;
				g.color = {
					metadata["text settings"]["color"][0].asFloat(),
					metadata["text settings"]["color"][1].asFloat(),
					metadata["text settings"]["color"][2].asFloat(),
				};
			}

			stat.cursor.x += (glyph.getAdvance().x) * scaleX;
			stat.cursor.x += metadata["text settings"]["kerning"].asFloat() * scaleX;
		//	stat.cursor.y -= (glyph.getAdvance().y) * scaleY;
		//	stat.cursor.y -= glyph.getPadding().y * scaleY;

			gs.push_back(g);
		}
		return gs;
	}

	void loadGui( ext::Gui& gui, uf::Image& image ) {
		uf::Serializer& metadata = gui.getComponent<uf::Serializer>();
		metadata["render"] = true;
		uf::GuiMesh& mesh = gui.getComponent<uf::GuiMesh>();
		/* get original image size (before padding) */ {
			metadata["original size"]["x"] = image.getDimensions().x;
			metadata["original size"]["y"] = image.getDimensions().y;

			image.padToPowerOfTwo();

			metadata["current size"]["x"] = image.getDimensions().x;
			metadata["current size"]["y"] = image.getDimensions().y;
		}
		pod::Vector2f correction = {
			(metadata["current size"]["x"].asInt() - metadata["original size"]["x"].asInt()) / (metadata["current size"]["x"].asFloat()),
			(metadata["current size"]["y"].asInt() - metadata["original size"]["y"].asInt()) / (metadata["current size"]["y"].asFloat())
		};
		mesh.vertices = {
			{ {-1.0f, 1.0f}, {0.0f, 0.0f}, },
			{ {-1.0f, -1.0f}, {0.0f, 1.0f-correction.y}, },
			{ {1.0f, -1.0f}, {1.0f-correction.x, 1.0f-correction.y}, },

			{ {-1.0f, 1.0f}, {0.0f, 0.0f}, },
			{ {1.0f, -1.0f}, {1.0f-correction.x, 1.0f-correction.y}, },
			{ {1.0f, 1.0f}, {1.0f-correction.x, 0.0f}, }
		};
		mesh.initialize(true);
		std::string suffix = ""; {
			std::string _ = gui.getRootParent<ext::World>().getComponent<uf::Serializer>()["shaders"]["gui"]["suffix"].asString();
			if ( _ != "" ) suffix = _ + ".";
		}
		if ( gui.getName() == "Gui: Text" ) {
			mesh.graphic.bindUniform<::GlyphDescriptor>();
			struct {
				std::string vertex = "./data/shaders/gui.text.stereo.vert.spv";
				std::string fragment = "./data/shaders/gui.text.frag.spv";
			} filenames;
			if ( metadata["shaders"]["vertex"].isString() ) filenames.vertex = metadata["shaders"]["vertex"].asString();
			if ( metadata["shaders"]["fragment"].isString() ) filenames.fragment = metadata["shaders"]["fragment"].asString();
			else if ( suffix != "" ) filenames.fragment = "./data/shaders/gui.text."+suffix+"frag.spv";
			mesh.graphic.initializeShaders({
				{filenames.vertex, VK_SHADER_STAGE_VERTEX_BIT},
				{filenames.fragment, VK_SHADER_STAGE_FRAGMENT_BIT}
			});
		} else {
			mesh.graphic.bindUniform<uf::StereoGuiMeshDescriptor>();
			struct {
				std::string vertex = "./data/shaders/gui.stereo.vert.spv";
				std::string fragment = "./data/shaders/gui.frag.spv";
			} filenames;
			if ( metadata["shaders"]["vertex"].isString() ) filenames.vertex = metadata["shaders"]["vertex"].asString();
			if ( metadata["shaders"]["fragment"].isString() ) filenames.fragment = metadata["shaders"]["fragment"].asString();
			else if ( suffix != "" ) filenames.fragment = "./data/shaders/gui."+suffix+"frag.spv";
			mesh.graphic.initializeShaders({
				{filenames.vertex, VK_SHADER_STAGE_VERTEX_BIT},
				{filenames.fragment, VK_SHADER_STAGE_FRAGMENT_BIT}
			});
		}

		mesh.graphic.texture.loadFromImage( 
			image,
			ext::vulkan::device,
			ext::vulkan::device.graphicsQueue
		);

		mesh.graphic.initialize( ext::vulkan::device, ext::vulkan::swapchain );
		mesh.graphic.autoAssign();

		{
			pod::Transform<>& transform = gui.getComponent<pod::Transform<>>();
			uf::GuiMesh& mesh = gui.getComponent<uf::GuiMesh>();
			auto& texture = mesh.graphic.texture;

			pod::Vector2f textureSize = {
				metadata["original size"]["x"].asFloat(),
				metadata["original size"]["y"].asFloat()
			};

			if ( metadata["scaling"].asString() == "fixed" ) {
				transform.scale = pod::Vector3{ (float) textureSize.x / ::size.current.x, (float) textureSize.y / ::size.current.y, 1 };
			} else if ( metadata["scaling"].asString() == "fixed-1080p" ) {
				transform.scale = pod::Vector3{ (float) textureSize.x / 1920, (float) textureSize.y / 1080, 1 };
			}
		}
	}
}


void ext::Gui::initialize() {
	ext::Object::initialize();
	

	{
		const ext::World& world = this->getRootParent<ext::World>();
		const uf::Serializer& _metadata = world.getComponent<uf::Serializer>();
		::size.current = {
			_metadata["window"]["size"]["x"].asFloat(),
			_metadata["window"]["size"]["y"].asFloat(),
		};
	}

	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	ext::Asset& assetLoader = this->getComponent<ext::Asset>();


	// master gui manager
	this->addHook( "window:Resized", [&](const std::string& event)->std::string{
		uf::Serializer json = event;
		if ( !this->hasComponent<uf::GuiMesh>() ) return "false";

		pod::Vector2ui size; {
			size.x = json["window"]["size"]["x"].asUInt64();
			size.y = json["window"]["size"]["y"].asUInt64();
		}

		::matrix = uf::matrix::ortho( 0.0f, (float) size.x, 0.0f, (float) size.y );
		::size.current = size;
	//	::size.reference = size;

		return "true";
	} );
	if ( this->m_name == "Gui Manager" ) {
		return;
	}
	
	this->addHook( "glyph:Load.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		unsigned long c = json["glyph"].asUInt64();
		std::string font = "./data/fonts/" + metadata["text settings"]["font"].asString();
		std::string key = ""; {
			key += std::to_string(c) + ";";
			key += metadata["text settings"]["padding"][0].asString() + ",";
			key += metadata["text settings"]["padding"][1].asString() + ";";
			key += metadata["text settings"]["spread"].asString() + ";";
			key += metadata["text settings"]["size"].asString() + ";";
			key += metadata["text settings"]["font"].asString() + ";";
			key += metadata["text settings"]["sdf"].asString();
		}
		uf::Glyph& glyph = ::glyphs.cache[font][key];
		
		uf::Image image; {
			const uint8_t* buffer = glyph.getBuffer();
			uf::Image::container_t pixels;
			std::size_t len = glyph.getSize().x * glyph.getSize().y;
			pixels.insert( pixels.end(), buffer, buffer + len );
			image.loadFromBuffer( pixels, glyph.getSize(), 8, 1, true );
		}
		loadGui( *this, image );
		return "true";
	});
	this->addHook( "asset:Load.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		std::string filename = json["filename"].asString();

		if ( uf::string::extension(filename) != "png" ) return "false";

		ext::World& world = this->getRootParent<ext::World>();
		ext::Asset& assetLoader = world.getComponent<ext::Asset>();
		const uf::Image* imagePointer = NULL;
		try { imagePointer = &assetLoader.get<uf::Image>(filename); } catch ( ... ) {}
		if ( !imagePointer ) return "false";
		
		uf::Image image = *imagePointer;

		loadGui( *this, image );
		return "true";
	});

	this->addHook( "window:Resized", [&](const std::string& event)->std::string{
		uf::Serializer json = event;
		if ( !this->hasComponent<uf::GuiMesh>() ) return "false";

		pod::Vector2ui size; {
			size.x = json["window"]["size"]["x"].asUInt64();
			size.y = json["window"]["size"]["y"].asUInt64();
		}
		pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
		uf::GuiMesh& mesh = this->getComponent<uf::GuiMesh>();
		auto& texture = mesh.graphic.texture;
		pod::Vector2f textureSize = {
			metadata["original size"]["x"].asFloat(),
			metadata["original size"]["y"].asFloat()
		};


		if ( metadata["text settings"].isObject() ) {
		} else if ( metadata["scaling"].asString() == "fixed" ) {
			transform.scale = pod::Vector3{ (float) textureSize.x / size.x, (float) textureSize.y / size.y, 1 };
		} else if ( metadata["scaling"].asString() == "fixed-1080p" ) {
			transform.scale = pod::Vector3{ (float) textureSize.x / 1920, (float) textureSize.y / 1080, 1 };
		}

		return "true";
	} );

	
	if ( metadata["_config"]["clickable"].asBool() ) {
		uf::Timer<long long> clickTimer(false);
		clickTimer.start();
		this->addHook( "gui:Clicked.%UID%", [&](const std::string& event)->std::string{
			uf::Serializer json = event;
			if ( !metadata["events"]["click"].isObject() ) return "true";
			uf::Serializer payload = metadata["events"]["click"]["payload"];
			this->callHook(metadata["events"]["click"]["name"].asString(), payload);
			return "true";
		});
		this->addHook( "window:Mouse.Click", [&](const std::string& event)->std::string{
			uf::Serializer json = event;
			if ( !this->hasComponent<uf::GuiMesh>() ) return "false";
			uf::Serializer& metadata = this->getComponent<uf::Serializer>();
			if ( metadata["world"].asBool() ) return "true";
			if ( !metadata["render"].asBool() ) return "true";
			if ( metadata["box"] == Json::nullValue ) return "true";

			bool down = json["mouse"]["state"].asString() == "Down";
			bool clicked = false;
			if ( down ) {
				pod::Vector2ui position; {
					position.x = json["mouse"]["position"]["x"].asInt() > 0 ? json["mouse"]["position"]["x"].asUInt() : 0;
					position.y = json["mouse"]["position"]["y"].asInt() > 0 ? json["mouse"]["position"]["y"].asUInt() : 0;
				}
				pod::Vector2f click; {
					click.x = (float) position.x / (float) ::size.current.x;
					click.y = (float) position.y / (float) ::size.current.y;

					float x = (click.x = (click.x * 2.0f) - 1.0f);
					float y = (click.y = (click.y * 2.0f) - 1.0f);

					for (int i = 0, j = metadata["box"].size() - 1; i < metadata["box"].size(); j = i++) {
						float xi = metadata["box"][i]["x"].asFloat(), yi = metadata["box"][i]["y"].asFloat();
						float xj = metadata["box"][j]["x"].asFloat(), yj = metadata["box"][j]["y"].asFloat();
						bool intersect = ((yi > y) != (yj > y)) && (x < (xj - xi) * (y - yi) / (yj - yi) + xi);
						if (intersect) clicked = !clicked;
					}
				}
			}
			metadata["clicked"] = clicked;

			if ( metadata["clicked"].asBool() && clickTimer.elapsed().asDouble() >= 1 ) {
				clickTimer.reset();
				this->callHook("gui:Clicked.%UID%");
			}
			return "true";
		} );
	}
	if ( metadata["_config"]["hoverable"].asBool() ) {
		this->addHook( "window:Mouse.Moved", [&](const std::string& event)->std::string{
			uf::Serializer json = event;
			if ( !this->hasComponent<uf::GuiMesh>() ) return "false";
			uf::Serializer& metadata = this->getComponent<uf::Serializer>();
			if ( metadata["world"].asBool() ) return "true";
			if ( !metadata["render"].asBool() ) return "true";
			if ( metadata["box"] == Json::nullValue ) return "true";

			bool down = json["mouse"]["state"].asString() == "Down";
			bool clicked = false;
			pod::Vector2ui position; {
				position.x = json["mouse"]["position"]["x"].asInt() > 0 ? json["mouse"]["position"]["x"].asUInt() : 0;
				position.y = json["mouse"]["position"]["y"].asInt() > 0 ? json["mouse"]["position"]["y"].asUInt() : 0;
			}
			pod::Vector2f click; {
				click.x = (float) position.x / (float) ::size.current.x;
				click.y = (float) position.y / (float) ::size.current.y;

				float x = (click.x = (click.x * 2.0f) - 1.0f);
				float y = (click.y = (click.y * 2.0f) - 1.0f);

				for (int i = 0, j = metadata["box"].size() - 1; i < metadata["box"].size(); j = i++) {
					float xi = metadata["box"][i]["x"].asFloat(), yi = metadata["box"][i]["y"].asFloat();
					float xj = metadata["box"][j]["x"].asFloat(), yj = metadata["box"][j]["y"].asFloat();
					bool intersect = ((yi > y) != (yj > y)) && (x < (xj - xi) * (y - yi) / (yj - yi) + xi);
					if (intersect) clicked = !clicked;
				}
			}
			metadata["hovered"] = clicked;

			return "true";
		} );
	}
	if ( metadata["text settings"]["string"].isString() ) {
	/*
		mutex.lock();
		jobs.push({ this->getUid() });
		mutex.unlock();
	*/

		uf::Serializer defaultSettings;
		defaultSettings.readFromFile("./data/entities/gui/text/string.json");

		if ( metadata["text settings"]["padding"].isNull() ) metadata["text settings"]["padding"] = defaultSettings["metadata"]["text settings"]["padding"];
		if ( metadata["text settings"]["spread"].isNull() ) metadata["text settings"]["spread"] = defaultSettings["metadata"]["text settings"]["spread"];
		if ( metadata["text settings"]["weight"].isNull() ) metadata["text settings"]["weight"] = defaultSettings["metadata"]["text settings"]["weight"];
		if ( metadata["text settings"]["size"].isNull() ) metadata["text settings"]["size"] = defaultSettings["metadata"]["text settings"]["size"];
		if ( metadata["text settings"]["scale"].isNull() ) metadata["text settings"]["scale"] = defaultSettings["metadata"]["text settings"]["scale"];
		if ( metadata["text settings"]["sdf"].isNull() ) metadata["text settings"]["sdf"] = defaultSettings["metadata"]["text settings"]["sdf"];
		if ( metadata["text settings"]["kerning"].isNull() ) metadata["text settings"]["kerning"] = defaultSettings["metadata"]["text settings"]["kerning"];
		if ( metadata["text settings"]["font"].isNull() ) metadata["text settings"]["font"] = defaultSettings["metadata"]["text settings"]["font"];

		float delay = 0.0f;
		std::vector<::GlyphBox> glyphs = generateGlyphs(*this);
		for ( auto& glyph : glyphs ) {
			// append new child
			ext::Gui* glyphElement = new ext::Gui;
			this->addChild(*glyphElement);
			glyphElement->load("./entities/gui/text/letter.json");
			uf::Serializer& pMetadata = glyphElement->getComponent<uf::Serializer>();
			pMetadata["events"] = metadata["events"];

			pMetadata["text settings"] = metadata["text settings"];
			pMetadata["text settings"].removeMember("string");
			
			pMetadata["text settings"]["color"][0] = glyph.color[0];
			pMetadata["text settings"]["color"][1] = glyph.color[1];
			pMetadata["text settings"]["color"][2] = glyph.color[2];

			pMetadata["_config"]["hoverable"] = metadata["_config"]["hoverable"];
			pMetadata["_config"]["clickable"] = metadata["_config"]["clickable"];
			
			glyphElement->initialize();

			pod::Transform<>& pTransform = glyphElement->getComponent<pod::Transform<>>();

			pTransform.position.x = glyph.box.x;
			pTransform.position.y = glyph.box.y;
			pTransform.scale.x = glyph.box.w;
			pTransform.scale.y = glyph.box.h;
		
			pTransform.reference = this->getComponentPointer<pod::Transform<>>();
		
			uf::Serializer payload;
			payload["glyph"] = (uint64_t) glyph.code;
			if ( metadata["text settings"]["scroll speed"].isNumeric() ) {
				glyphElement->queueHook("glyph:Load.%UID%", payload, delay);
				delay += metadata["text settings"]["scroll speed"].asFloat();
			} else {
				glyphElement->callHook("glyph:Load.%UID%", payload);
			}
		}
	}
}
void ext::Gui::tick() {
	ext::Object::tick();
/*
	if ( this->m_name == "Gui Manager" ) {
		mutex.lock();
		while ( !jobs.empty() ) {
			auto job = jobs.front();
			uint64_t uid = job.uid;
			ext::Gui* element = (ext::Gui*) this->findByUid( uid );
			jobs.pop();


			if ( !element ) continue;
			if ( element->getUid() != uid ) continue;

			std::vector<::GlyphBox> glyphs = generateGlyphs(*element);
			uf::Serializer& metadata = element->getComponent<uf::Serializer>();
			
			for ( auto& glyph : glyphs ) {
				// append new child
				ext::Gui* glyphElement = new ext::Gui;
				element->addChild(*glyphElement);
				glyphElement->load("./entities/gui/text/letter.json");
				uf::Serializer& pMetadata = glyphElement->getComponent<uf::Serializer>();
				pMetadata["text settings"] = metadata["text settings"];
				pMetadata["text settings"].removeMember("string");
				
				pMetadata["text settings"]["color"][0] = glyph.color[0];
				pMetadata["text settings"]["color"][1] = glyph.color[1];
				pMetadata["text settings"]["color"][2] = glyph.color[2];

				pMetadata["_config"]["hoverable"] = metadata["_config"]["hoverable"];
				pMetadata["_config"]["clickable"] = metadata["_config"]["clickable"];
				
				glyphElement->initialize();

				pod::Transform<>& pTransform = glyphElement->getComponent<pod::Transform<>>();

				pTransform.position.x = glyph.box.x;
				pTransform.position.y = glyph.box.y;
				pTransform.scale.x = glyph.box.w;
				pTransform.scale.y = glyph.box.h;
			
				pTransform.reference = element->getComponentPointer<pod::Transform<>>();

				uf::Serializer payload;
				payload["glyph"] = (uint64_t) glyph.code;
				glyphElement->callHook("glyph:Load.%UID%", payload);
			}
		}
		mutex.unlock();
	}
*/

	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	if ( metadata["text settings"]["fade in speed"].isNumeric() && !metadata["system"]["faded in"].asBool() ) {
		float speed = metadata["text settings"]["fade in speed"].asFloat();
		float alpha = metadata["text settings"]["color"][3].asFloat();
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
void ext::Gui::render() {
	ext::Object::render();

	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	/* Update uniforms */ if ( this->hasComponent<uf::GuiMesh>() ) {
		auto& scene = this->getRootParent<ext::World>();
		auto& mesh = this->getComponent<uf::GuiMesh>();
		auto& camera = scene.getController()->getComponent<uf::Camera>();
		auto& transform = this->getComponent<pod::Transform<>>();
		if ( !mesh.generated ) return;		

		pod::Vector4 offset = {
			metadata["uv"][0].asFloat(),
			metadata["uv"][1].asFloat(),
			metadata["uv"][2].asFloat(),
			metadata["uv"][3].asFloat()
		};
		int mode = 0;
		if ( !metadata["shader"].isNull() ) mode = metadata["shader"].asInt();
		
		if ( this->m_name == "Gui: Text" ) {
			::GlyphDescriptor uniforms;
			if ( !metadata["text settings"]["color"].isArray() ) {
				metadata["text settings"]["color"][0] = 1.0f;
				metadata["text settings"]["color"][1] = 1.0f;
				metadata["text settings"]["color"][2] = 1.0f;
				metadata["text settings"]["color"][3] = 1.0f;
			}
			if ( !metadata["text settings"]["stroke"].isArray() ) {
				metadata["text settings"]["stroke"][0] = 0.0f;
				metadata["text settings"]["stroke"][1] = 0.0f;
				metadata["text settings"]["stroke"][2] = 0.0f;
				metadata["text settings"]["stroke"][3] = 1.0f;
			}
			pod::Vector4 color = {
				metadata["text settings"]["color"][0].asFloat(),
				metadata["text settings"]["color"][1].asFloat(),
				metadata["text settings"]["color"][2].asFloat(),
				metadata["text settings"]["color"][3].asFloat()
			};
			pod::Vector4 stroke = {
				metadata["text settings"]["stroke"][0].asFloat(),
				metadata["text settings"]["stroke"][1].asFloat(),
				metadata["text settings"]["stroke"][2].asFloat(),
				metadata["text settings"]["stroke"][3].asFloat()
			};

			if ( uf::Window::isKeyPressed("V") ) {
				metadata["text settings"]["weight"] = metadata["text settings"]["weight"].asFloat() + uf::physics::time::delta;
				std::cout << metadata["text settings"]["weight"].asFloat() << std::endl;
			}
			if ( uf::Window::isKeyPressed("C") ) {
				metadata["text settings"]["weight"] = metadata["text settings"]["weight"].asFloat() - uf::physics::time::delta;
				std::cout << metadata["text settings"]["weight"].asFloat() << std::endl;
			}
			if ( uf::Window::isKeyPressed("N") ) {
				metadata["text settings"]["spread"] = metadata["text settings"]["spread"].asFloat() + uf::physics::time::delta;
				std::cout << metadata["text settings"]["spread"].asFloat() << std::endl;
			}
			if ( uf::Window::isKeyPressed("B") ) {
				metadata["text settings"]["spread"] = metadata["text settings"]["spread"].asFloat() - uf::physics::time::delta;
				std::cout << metadata["text settings"]["spread"].asFloat() << std::endl;
			}

			uniforms.gui.offset = offset;
			uniforms.gui.color = color;
			uniforms.gui.stroke = stroke;
			uniforms.gui.mode = mode;
			uniforms.gui.sdf = metadata["text settings"]["sdf"].asBool();
			uniforms.gui.shadowbox = metadata["text settings"]["shadowbox"].asBool();
			uniforms.gui.weight = metadata["text settings"]["weight"].asFloat(); // float
			uniforms.gui.spread = metadata["text settings"]["spread"].asInt(); // int
			uniforms.gui.scale = metadata["text settings"]["scale"].asFloat(); // float
			if ( !metadata["text settings"]["depth"].isNull() ) 
				uniforms.gui.depth = metadata["text settings"]["depth"].asFloat();
			else uniforms.gui.depth = 0.0f;

			for ( std::size_t i = 0; i < 2; ++i ) {
				if ( metadata["text settings"]["world"].asBool() ) {
					auto& scene = this->getRootParent<ext::Scene>();
					auto& camera = scene.getController()->getComponent<uf::Camera>();

					pod::Transform<> flatten = uf::transform::flatten( this->getComponent<pod::Transform<>>() );
					auto model = uf::transform::model( flatten );
					auto view = camera.getView(i);
					auto projection = camera.getProjection(i);
					uniforms.matrices.model[i] = projection * view * model;
				} else {
					uf::Matrix4 translation, rotation, scale;
					pod::Transform<> flatten = uf::transform::flatten(transform, false);
					flatten.orientation.w *= -1;
					rotation = uf::quaternion::matrix(flatten.orientation);
					scale = uf::matrix::scale( scale, transform.scale );
					translation = uf::matrix::translate( uf::matrix::identity(), flatten.position );
					uniforms.matrices.model[i] = translation * scale * rotation;
				}
			}
			mesh.graphic.updateBuffer( uniforms, 0, false );
			// calculate click box
			{
				auto& model = uniforms.matrices.model[0];
				auto& mesh = this->getComponent<uf::GuiMesh>();
				auto& vertices = mesh.vertices;
				int i = 0;
				for ( auto& vertex : vertices ) {
					auto& position = vertex.position;
					auto translated = uf::matrix::multiply( model, { position.x, position.y, 0.0f, 1.0f } );
					// points.push_back( translated );
					metadata["box"][i]["x"] = translated.x;
					metadata["box"][i]["y"] = translated.y;
					++i;
				}
			}
		} else {
			if ( !metadata["color"].isArray() ) {
				metadata["color"][0] = 1.0f;
				metadata["color"][1] = 1.0f;
				metadata["color"][2] = 1.0f;
				metadata["color"][3] = 1.0f;
			}
			pod::Vector4 color = {
				metadata["color"][0].asFloat(),
				metadata["color"][1].asFloat(),
				metadata["color"][2].asFloat(),
				metadata["color"][3].asFloat()
			};
			uf::StereoGuiMeshDescriptor uniforms;
			uniforms.gui.offset = offset;
			uniforms.gui.color = color;
			uniforms.gui.mode = mode;
			if ( !metadata["depth"].isNull() )
				uniforms.gui.depth = metadata["depth"].asFloat();
			else
				uniforms.gui.depth = 0;
			
			for ( std::size_t i = 0; i < 2; ++i ) {
				if ( metadata["world"].asBool() ) {
				/*
					pod::Transform<> flatten = uf::transform::flatten(camera.getTransform(), true);
					pod::Matrix4 rotation = uf::quaternion::matrix( uf::vector::multiply( { 1, 1, 1, -1 }, flatten.orientation) );
					uniforms.matrices.model[i] = ::matrix * rotation;
				*/
					auto& scene = this->getRootParent<ext::Scene>();
					auto& camera = scene.getController()->getComponent<uf::Camera>();

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
			mesh.graphic.updateBuffer( uniforms, 0, false );
			// calculate click box
			{
				auto& model = uniforms.matrices.model[0];
				auto& mesh = this->getComponent<uf::GuiMesh>();
				auto& vertices = mesh.vertices;
				int i = 0;
				for ( auto& vertex : vertices ) {
					auto& position = vertex.position;
					auto translated = uf::matrix::multiply( model, { position.x, position.y, 0.0f, 1.0f } );
					// points.push_back( translated );
					metadata["box"][i]["x"] = translated.x;
					metadata["box"][i]["y"] = translated.y;
					++i;
				}
			}
		}	
	} else if ( this->m_name != "Gui Manager" ) {
		bool hovered = false;
		bool clicked = false;
		for ( uf::Entity* entity : this->getChildren() ) {
			if ( !entity ) continue;
			if ( entity->getUid() == 0 ) continue;
			uf::Serializer& pMetadata = entity->getComponent<uf::Serializer>();
			if ( pMetadata["hovered"].asBool() ) hovered = true;
			if ( pMetadata["clicked"].asBool() ) clicked = true;
		}
		metadata["clicked"] = clicked;
		metadata["hovered"] = hovered;
	}

	if ( metadata["debug"]["moveable"].asBool() ) {
		pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
		pod::Vector2f step = {
			1 / 1280.0f,
			1 / 720.0f,
		};
		if ( uf::Window::isKeyPressed("U") ) {
			step.x = (step.y = uf::physics::time::delta * 0.5f);
		}

		if ( uf::Window::isKeyPressed("O") && uf::Window::isKeyPressed("J") ) {
			transform.scale.x += step.x * -1;
			pod::Transform<> flat = uf::transform::flatten( transform );
			std::cout << this->m_name << ": Scale: " << flat.scale.x << ", " << flat.scale.y << std::endl;
		}
		if ( uf::Window::isKeyPressed("O") && uf::Window::isKeyPressed("L") ) {
			transform.scale.x += step.x * 1;
			pod::Transform<> flat = uf::transform::flatten( transform );
			std::cout << this->m_name << ": Scale: " << flat.scale.x << ", " << flat.scale.y << std::endl;
		}
		if ( uf::Window::isKeyPressed("O") && uf::Window::isKeyPressed("I") ) {
			transform.scale.y += step.y * -1;
			pod::Transform<> flat = uf::transform::flatten( transform );
			std::cout << this->m_name << ": Scale: " << flat.scale.x << ", " << flat.scale.y << std::endl;
		}
		if ( uf::Window::isKeyPressed("O") && uf::Window::isKeyPressed("K") ) {
			transform.scale.y += step.y * 1;
			pod::Transform<> flat = uf::transform::flatten( transform );
			std::cout << this->m_name << ": Scale: " << flat.scale.x << ", " << flat.scale.y << std::endl;
		}
		if ( uf::Window::isKeyPressed("J") ) {
			transform.position.x += step.x * -1;
			pod::Transform<> flat = uf::transform::flatten( transform );
			std::cout << this->m_name << ": Position: " << flat.position.x << ", " << flat.position.y << std::endl;
		}
		if ( uf::Window::isKeyPressed("L") ) {
			transform.position.x += step.x * 1;
			pod::Transform<> flat = uf::transform::flatten( transform );
			std::cout << this->m_name << ": Position: " << flat.position.x << ", " << flat.position.y << std::endl;
		}
		if ( uf::Window::isKeyPressed("I") ) {
			transform.position.y += step.y * -1;
			pod::Transform<> flat = uf::transform::flatten( transform );
			std::cout << this->m_name << ": Position: " << flat.position.x << ", " << flat.position.y << std::endl;
		}
		if ( uf::Window::isKeyPressed("K") ) {
			transform.position.y += step.y * 1;
			pod::Transform<> flat = uf::transform::flatten( transform );
			std::cout << this->m_name << ": Position: " << flat.position.x << ", " << flat.position.y << std::endl;
		}
	}
}

void ext::Gui::destroy() {
	if ( this->hasComponent<uf::GuiMesh>() ) {
		auto& mesh = this->getComponent<uf::GuiMesh>();
		mesh.graphic.destroy();
		mesh.destroy();
	}
	ext::Object::destroy();
}