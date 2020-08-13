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
#include <uf/engine/asset/asset.h>
#include <uf/engine/scene/scene.h>

#include <unordered_map>
#include <locale>
#include <codecvt>

#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/graphics/base.h>
#include <uf/ext/vulkan/rendermodes/rendertarget.h>
#include <uf/ext/openvr/openvr.h>

#include <uf/utils/http/http.h>
#include <uf/utils/audio/audio.h>
#include <sys/stat.h>
#include <fstream>

#include <regex>

EXT_OBJECT_REGISTER_CPP(Gui)

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
			1280,
			720,
		};
	} size;
/*
	std::mutex mutex;
	uint64_t uid = 0;
	struct Job {
		uint64_t uid;
	};
	std::queue<Job> jobs;
*/
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

	std::vector<::GlyphBox> generateGlyphs( ext::Gui& gui, std::string string = "" ) {
		uf::Serializer& metadata = gui.getComponent<uf::Serializer>();
		std::string font = "./data/fonts/" + metadata["text settings"]["font"].asString();
		if ( ::glyphs.cache[font].empty() ) {
			ext::freetype::initialize( ::glyphs.glyph, font );
		}
		if ( string == "" ) {
			string = metadata["text settings"]["string"].asString();
		}
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
					key += metadata["text settings"]["padding"][0].asString() + ",";
					key += metadata["text settings"]["padding"][1].asString() + ";";
					key += metadata["text settings"]["spread"].asString() + ";";
					key += metadata["text settings"]["size"].asString() + ";";
					key += metadata["text settings"]["font"].asString() + ";";
					key += metadata["text settings"]["sdf"].asString();
				}
				uf::Glyph& glyph = ::glyphs.cache[font][key];
				::GlyphBox g;

				g.box.w = glyph.getSize().x;
				g.box.h = glyph.getSize().y;
					
				g.box.x = stat.cursor.x + glyph.getBearing().x;
				g.box.y = stat.cursor.y - glyph.getBearing().y; // - (glyph.getSize().y - glyph.getBearing().y);

				stat.cursor.x += (glyph.getAdvance().x);
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
					metadata["text settings"]["color"][0].asFloat(),
					metadata["text settings"]["color"][1].asFloat(),
					metadata["text settings"]["color"][2].asFloat(),
				};
			}

			gs.push_back(g);
		}
		return gs;
	}

	void loadGui( ext::Gui& gui, uf::Image& image ) {
		uf::Serializer& metadata = gui.getComponent<uf::Serializer>();
		metadata["render"] = true;
		{
		//	this->addAlias<uf::GuiMesh, uf::MeshBase>();
			gui.addAlias<uf::GuiMesh, uf::Mesh>();
		}
		uf::GuiMesh& mesh = gui.getComponent<uf::GuiMesh>();
		/* get original image size (before padding) */ {
			metadata["original size"]["x"] = image.getDimensions().x;
			metadata["original size"]["y"] = image.getDimensions().y;

			// image.padToPowerOfTwo();

			metadata["current size"]["x"] = image.getDimensions().x;
			metadata["current size"]["y"] = image.getDimensions().y;
		}
	/*
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
	*/
		std::string suffix = ""; {
			std::string _ = gui.getRootParent<uf::Scene>().getComponent<uf::Serializer>()["shaders"]["gui"]["suffix"].asString();
			if ( _ != "" ) suffix = _ + ".";
		}
		if ( gui.getName() == "Gui: Text" ) {
			::GlyphBox g;
			g.box.x = metadata["text settings"]["box"][0].asFloat();
			g.box.y = metadata["text settings"]["box"][1].asFloat();
			g.box.w = metadata["text settings"]["box"][2].asFloat();
			g.box.h = metadata["text settings"]["box"][3].asFloat();
		/*
			if ( ext::openvr::enabled ) {
				mesh.vertices = {
					{{ g.box.x,           g.box.y + g.box.h }, { 0.0f, 0.0f }},
					{{ g.box.x + g.box.w, g.box.y           }, { 1.0f, 1.0f }},
					{{ g.box.x,           g.box.y           }, { 0.0f, 1.0f }},

					{{ g.box.x,           g.box.y + g.box.h }, { 0.0f, 0.0f }},
					{{ g.box.x + g.box.w, g.box.y + g.box.h }, { 1.0f, 0.0f }},
					{{ g.box.x + g.box.w, g.box.y           }, { 1.0f, 1.0f }},
				};
			} else {
				mesh.vertices = {
					{{ g.box.x,           g.box.y + g.box.h }, { 0.0f, 0.0f }},
					{{ g.box.x,           g.box.y           }, { 0.0f, 1.0f }},
					{{ g.box.x + g.box.w, g.box.y           }, { 1.0f, 1.0f }},

					{{ g.box.x,           g.box.y + g.box.h }, { 0.0f, 0.0f }},
					{{ g.box.x + g.box.w, g.box.y           }, { 1.0f, 1.0f }},
					{{ g.box.x + g.box.w, g.box.y + g.box.h }, { 1.0f, 0.0f }},
				};
			}
		*/
			mesh.vertices = {
				{{ g.box.x,           g.box.y + g.box.h }, { 0.0f, 0.0f }},
				{{ g.box.x,           g.box.y           }, { 0.0f, 1.0f }},
				{{ g.box.x + g.box.w, g.box.y           }, { 1.0f, 1.0f }},

				{{ g.box.x,           g.box.y + g.box.h }, { 0.0f, 0.0f }},
				{{ g.box.x + g.box.w, g.box.y           }, { 1.0f, 1.0f }},
				{{ g.box.x + g.box.w, g.box.y + g.box.h }, { 1.0f, 0.0f }},
			};
			for ( auto& vertex : mesh.vertices ) {
				vertex.position.x /= ::size.reference.x;
				vertex.position.y /= ::size.reference.y;
			}
			mesh.initialize(true);
			mesh.graphic.initialize( "Gui" );
			struct {
				std::string vertex = "./data/shaders/gui.text.vert.spv";
				std::string fragment = "./data/shaders/gui.text.frag.spv";
			} filenames;
			if ( metadata["shaders"]["vertex"].isString() ) filenames.vertex = metadata["shaders"]["vertex"].asString();
			if ( metadata["shaders"]["fragment"].isString() ) filenames.fragment = metadata["shaders"]["fragment"].asString();
			else if ( suffix != "" ) filenames.fragment = "./data/shaders/gui.text."+suffix+"frag.spv";

			mesh.graphic.material.initializeShaders({
				{filenames.vertex, VK_SHADER_STAGE_VERTEX_BIT},
				{filenames.fragment, VK_SHADER_STAGE_FRAGMENT_BIT},
			});
		} else {
		/*
			if ( ext::openvr::enabled ) {
				mesh.vertices = {
					{ {-1.0f, 1.0f}, {0.0f, 0.0f}, },
					{ {1.0f, -1.0f}, {1.0f, 1.0f}, },
					{ {-1.0f, -1.0f}, {0.0f, 1.0f}, },

					{ {-1.0f, 1.0f}, {0.0f, 0.0f}, },
					{ {1.0f, 1.0f}, {1.0f, 0.0f}, },
					{ {1.0f, -1.0f}, {1.0f, 1.0f}, },
				};
			} else {
				mesh.vertices = {
					{ {-1.0f, 1.0f}, {0.0f, 0.0f}, },
					{ {-1.0f, -1.0f}, {0.0f, 1.0f}, },
					{ {1.0f, -1.0f}, {1.0f, 1.0f}, },

					{ {-1.0f, 1.0f}, {0.0f, 0.0f}, },
					{ {1.0f, -1.0f}, {1.0f, 1.0f}, },
					{ {1.0f, 1.0f}, {1.0f, 0.0f}, }
				};
			}
		*/
			mesh.vertices = {
				{ {-1.0f, 1.0f}, {0.0f, 0.0f}, },
				{ {-1.0f, -1.0f}, {0.0f, 1.0f}, },
				{ {1.0f, -1.0f}, {1.0f, 1.0f}, },

				{ {-1.0f, 1.0f}, {0.0f, 0.0f}, },
				{ {1.0f, -1.0f}, {1.0f, 1.0f}, },
				{ {1.0f, 1.0f}, {1.0f, 0.0f}, }
			};
			mesh.initialize(true);
			mesh.graphic.initialize( "Gui" );
			struct {
				std::string vertex = "./data/shaders/gui.vert.spv";
				std::string fragment = "./data/shaders/gui.frag.spv";
			} filenames;
			if ( metadata["shaders"]["vertex"].isString() ) filenames.vertex = metadata["shaders"]["vertex"].asString();
			if ( metadata["shaders"]["fragment"].isString() ) filenames.fragment = metadata["shaders"]["fragment"].asString();
			else if ( suffix != "" ) filenames.fragment = "./data/shaders/gui."+suffix+"frag.spv";
		
			mesh.graphic.material.initializeShaders({
				{filenames.vertex, VK_SHADER_STAGE_VERTEX_BIT},
				{filenames.fragment, VK_SHADER_STAGE_FRAGMENT_BIT},
			});
		}

		auto& texture = mesh.graphic.material.textures.emplace_back();
		texture.loadFromImage( image );

		{
			pod::Transform<>& transform = gui.getComponent<pod::Transform<>>();
			uf::GuiMesh& mesh = gui.getComponent<uf::GuiMesh>();
			auto& texture = mesh.graphic.material.textures.front();

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
	uf::Object::initialize();

	// alias Mesh types
	{
	//	this->addAlias<uf::GuiMesh, uf::MeshBase>();
		this->addAlias<uf::GuiMesh, uf::Mesh>();
	}
	
	{
		const uf::Scene& world = this->getRootParent<uf::Scene>();
		const uf::Serializer& _metadata = world.getComponent<uf::Serializer>();
		::size.current = {
			_metadata["window"]["size"]["x"].asFloat(),
			_metadata["window"]["size"]["y"].asFloat(),
		};
	}

	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Asset& assetLoader = this->getComponent<uf::Asset>();


	// master gui manager
	this->addHook( "window:Resized", [&](const std::string& event)->std::string{
		uf::Serializer json = event;
		if ( !this->hasComponent<uf::GuiMesh>() ) return "false";

		pod::Vector2ui size; {
			size.x = json["window"]["size"]["x"].asUInt64();
			size.y = json["window"]["size"]["y"].asUInt64();
		}

	//	::matrix = uf::matrix::ortho( 0.0f, (float) size.x, 0.0f, (float) size.y );
	//	::matrix = uf::matrix::ortho( size.x * -0.5f, size.x * 0.5f, size.y * -0.5f, size.y * 0.5f );
	//	::matrix = uf::matrix::ortho( size.x * -1.0f, size.x * 1.0f, size.y * -1.0f, size.y * 1.0f );
		::size.current = size;
	//	::size.reference = size;

		return "true";
	} );
	if ( this->m_name == "Gui Manager" ) {
		this->addHook( "window:Mouse.Moved", [&](const std::string& event)->std::string{
			uf::Serializer json = event;

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
			}

			{
				auto& scene = uf::scene::getCurrentScene();
				auto& controller = *scene.getController();
				auto& metadata = controller.getComponent<uf::Serializer>();

				if ( metadata["overlay"]["cursor"]["type"].asString() == "mouse" ) {
					metadata["overlay"]["cursor"]["position"][0] = click.x;
					metadata["overlay"]["cursor"]["position"][1] = click.y;
				}
			}

			return "true";
		});
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

		uf::Scene& world = this->getRootParent<uf::Scene>();
		uf::Asset& assetLoader = world.getComponent<uf::Asset>();
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
		auto& texture = mesh.graphic.material.textures.front();
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

	
	if ( metadata["system"]["clickable"].asBool() ) {
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

					if (json["invoker"] == "vr" ) {
						x = json["mouse"]["position"]["x"].asFloat();
						y = json["mouse"]["position"]["y"].asFloat();
					}

					for (int i = 0, j = metadata["box"].size() - 1; i < metadata["box"].size(); j = i++) {
						float xi = metadata["box"][i]["x"].asFloat(), yi = metadata["box"][i]["y"].asFloat();
						float xj = metadata["box"][j]["x"].asFloat(), yj = metadata["box"][j]["y"].asFloat();
						bool intersect = ((yi > y) != (yj > y)) && (x < (xj - xi) * (y - yi) / (yj - yi) + xi);
						if (intersect) clicked = !clicked;
					}
				}
			/*
				if ( clicked ) std::cout << "Clicked on " << this->m_name << std::endl;
				else {
					std::cout << click.x << ", " << click.y << " " << this->m_name << metadata["box"] << std::endl;
				}
			*/
			}
			metadata["clicked"] = clicked;

			if ( metadata["clicked"].asBool() && clickTimer.elapsed().asDouble() >= 1 ) {
				clickTimer.reset();
				this->callHook("gui:Clicked.%UID%");
			}
			return "true";
		} );
	}
	if ( metadata["system"]["hoverable"].asBool() ) {
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
		uf::Serializer defaultSettings;
		defaultSettings.readFromFile("./data/entities/gui/text/string.json");

		for ( auto it = defaultSettings["metadata"]["text settings"].begin(); it != defaultSettings["metadata"]["text settings"].end(); ++it ) {
			std::string key = it.key().asString();
			if ( metadata["text settings"][key].isNull() ) {
				metadata["text settings"][key] = defaultSettings["metadata"]["text settings"][key];
			}

		}
		float delay = 0.0f;
		float scale = metadata["text settings"]["scale"].asFloat();
		std::vector<::GlyphBox> glyphs = generateGlyphs(*this);
		std::cout << "Loading string: " << metadata["text settings"]["string"] << std::endl;
		for ( auto& glyph : glyphs ) {
			ext::Gui* glyphElement = (ext::Gui*) this->findByUid( this->loadChild("/gui/text/letter.json", false) );
		/*
			ext::Gui* glyphElement = new ext::Gui;
			this->addChild(*glyphElement);
			glyphElement->load("/gui/text/letter.json", true);
		*/

			uf::Serializer& pMetadata = glyphElement->getComponent<uf::Serializer>();
			pMetadata["events"] = metadata["events"];

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
			
			glyphElement->initialize();

			pod::Transform<>& pTransform = glyphElement->getComponent<pod::Transform<>>();
			pTransform.scale.x = scale;
			pTransform.scale.y = scale;
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
	uf::Object::tick();

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
	uf::Object::render();

	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	/* Update uniforms */ if ( this->hasComponent<uf::GuiMesh>() ) {
		auto& scene = this->getRootParent<uf::Scene>();
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
		//	::GlyphDescriptor uniforms;
		//	auto& uniforms = mesh.graphic.uniforms<::GlyphDescriptor>();
			auto& uniforms = mesh.graphic.material.shaders.front().uniforms.front().get<::GlyphDescriptor>();

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
					auto& scene = this->getRootParent<uf::Scene>();
					auto& camera = scene.getController()->getComponent<uf::Camera>();

					pod::Transform<> flatten = uf::transform::flatten( this->getComponent<pod::Transform<>>() );
					auto model = uf::transform::model( flatten );
					auto view = camera.getView(i);
					auto projection = camera.getProjection(i);
					uniforms.matrices.model[i] = projection * view * model;
				} else {			
					uf::Matrix4 translation, rotation, scale;
					pod::Transform<> flatten = uf::transform::flatten(transform, false);
					// make our own flattened position, for some reason this causes z to be 6.6E+28
					flatten.position = transform.position + transform.reference->position;
					flatten.orientation.w *= -1;
					rotation = uf::quaternion::matrix(flatten.orientation);
				//	pod::Vector3f offsetSize = { ::size.current.x, ::size.current.y, 1 };
				//	translation = uf::matrix::translate( uf::matrix::identity(), flatten.position * offsetSize );
					translation = uf::matrix::translate( uf::matrix::identity(), flatten.position );
					scale = uf::matrix::scale( scale, transform.scale );
				//	uniforms.matrices.model[i] = ::matrix * translation * rotation * scale;
					uniforms.matrices.model[i] = translation * rotation * scale;
				}
			}
			uniforms.gui.depth = 1.0f - uniforms.gui.depth; 
			// mesh.graphic.updateBuffer( uniforms, 0, false );
			mesh.graphic.material.shaders.front().updateBuffer( uniforms, 0, false );
			// calculate click box
			{
				auto& model = uniforms.matrices.model[0];
				auto& mesh = this->getComponent<uf::GuiMesh>();
				auto& vertices = mesh.vertices;
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
		//	uf::StereoGuiMeshDescriptor uniforms;
		//	auto& uniforms = mesh.graphic.uniforms<uf::StereoGuiMeshDescriptor>();
			auto& uniforms = mesh.graphic.material.shaders.front().uniforms.front().get<uf::StereoGuiMeshDescriptor>();
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
					auto& scene = this->getRootParent<uf::Scene>();
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
			uniforms.gui.depth = 1.0f - uniforms.gui.depth; 
			// mesh.graphic.updateBuffer( uniforms, 0, false );
			mesh.graphic.material.shaders.front().updateBuffer( uniforms, 0, false );
			// calculate click box
			{
				auto& model = uniforms.matrices.model[0];
				auto& mesh = this->getComponent<uf::GuiMesh>();
				auto& vertices = mesh.vertices;
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
	} else {
		{
		/* Update GUI panel */ {
			auto& scene = this->getRootParent<uf::Scene>();
			auto& controller = *scene.getController();
			auto& camera = controller.getComponent<uf::Camera>();
			auto* renderMode = (ext::vulkan::RenderTargetRenderMode*) &ext::vulkan::getRenderMode("Gui");
			if ( renderMode->getName() == "Gui" ) {
				auto& blitter = renderMode->blitter;
				auto& metadata = controller.getComponent<uf::Serializer>();
				for ( std::size_t i = 0; i < 2; ++i ) {
					pod::Transform<> transform;
					if ( metadata["overlay"]["position"].isArray() ) 
						transform.position = {
							metadata["overlay"]["position"][0].asFloat(),
							metadata["overlay"]["position"][1].asFloat(),
							metadata["overlay"]["position"][2].asFloat(),
						};
					if ( metadata["overlay"]["scale"].isArray() ) 
						transform.scale = {
							metadata["overlay"]["scale"][0].asFloat(),
							metadata["overlay"]["scale"][1].asFloat(),
							metadata["overlay"]["scale"][2].asFloat(),
						};
					if ( metadata["overlay"]["orientation"].isArray() ) 
						transform.orientation = {
							metadata["overlay"]["orientation"][0].asFloat(),
							metadata["overlay"]["orientation"][1].asFloat(),
							metadata["overlay"]["orientation"][2].asFloat(),
							metadata["overlay"]["orientation"][3].asFloat(),
						};
					if ( ext::openvr::enabled && (metadata["overlay"]["enabled"].asBool() || ext::vulkan::getRenderMode("Stereoscopic Deferred", true).getType() == "Stereoscopic Deferred" )) {
						if ( metadata["overlay"]["floating"].asBool() ) {
							pod::Matrix4f model = uf::transform::model( transform );
							blitter.uniforms.matrices.models[i] = camera.getProjection(i) * ext::openvr::hmdEyePositionMatrix( i == 0 ? vr::Eye_Left : vr::Eye_Right ) * model;
						} else {
							auto translation = uf::matrix::translate( uf::matrix::identity(), camera.getTransform().position + controller.getComponent<pod::Transform<>>().position );
							auto rotation = uf::quaternion::matrix( controller.getComponent<pod::Transform<>>().orientation * pod::Vector4f{1,1,1,-1} );
	
							pod::Matrix4f model = translation * rotation * uf::transform::model( transform );
							blitter.uniforms.matrices.models[i] = camera.getProjection(i) * camera.getView(i) * model;
						}
					} else {
						pod::Matrix4t<> model = uf::matrix::translate( uf::matrix::identity(), { 0, 0, 1 } );
						blitter.uniforms.matrices.models[i] = model;
					}
					blitter.uniforms.alpha = metadata["overlay"]["alpha"].asFloat();

					blitter.uniforms.cursor.position.x = (metadata["overlay"]["cursor"]["position"][0].asFloat() + 1.0f) * 0.5f; //(::mouse.position.x + 1.0f) * 0.5f;
					blitter.uniforms.cursor.position.y = (metadata["overlay"]["cursor"]["position"][1].asFloat() + 1.0f) * 0.5f; //(::mouse.position.y + 1.0f) * 0.5f;

					pod::Vector3f cursorSize;
					cursorSize.x = metadata["overlay"]["cursor"]["radius"].asFloat();
					cursorSize.y = metadata["overlay"]["cursor"]["radius"].asFloat();
					cursorSize = uf::matrix::multiply<float>( uf::matrix::inverse( uf::matrix::scale( uf::matrix::identity() , transform.scale) ), cursorSize );
					
					blitter.uniforms.cursor.radius.x = cursorSize.x;
					blitter.uniforms.cursor.radius.y = cursorSize.y;
					
					blitter.uniforms.cursor.color.x = metadata["overlay"]["cursor"]["color"][0].asFloat();
					blitter.uniforms.cursor.color.y = metadata["overlay"]["cursor"]["color"][1].asFloat();
					blitter.uniforms.cursor.color.z = metadata["overlay"]["cursor"]["color"][2].asFloat();
					blitter.uniforms.cursor.color.w = metadata["overlay"]["cursor"]["color"][3].asFloat();
				}
				blitter.updateBuffer( (void*) &blitter.uniforms, sizeof(blitter.uniforms), 0 );
			}
		}
	}
	}

	if ( metadata["debug"]["moveable"].asBool() ) {
		pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
		pod::Vector2f step = {
			4 / (::size.current.x > 0 ? ::size.current.x : ::size.reference.x),
			4 / (::size.current.y > 0 ? ::size.current.y : ::size.reference.y),
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
	uf::Object::destroy();
}