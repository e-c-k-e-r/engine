#include "../main.h"

#include <uf/utils/window/window.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/image/image.h>
#include <uf/gl/gl.h>
#include <uf/spec/terminal/terminal.h>
#include <uf/utils/hook/hook.h>

bool client::ready = false;
bool client::terminated = false;
uf::Window client::window;
uf::Serializer client::config;

void client::initialize() {
	spec::Context::globalInit();
	uf::IoStream::ncurses = true;

	/* Initialize config */ {
		struct {
			uf::Serializer ext;
			uf::Serializer fallback;
		} config;
		/* Get configuration */ {
			config.ext = ext::getConfig();
		}
		/* Initialize default configuration */ {
			config.fallback["terminal"]["visible"] 				= true;
			config.fallback["terminal"]["ncurses"] 				= true;
			config.fallback["window"]["title"] 					= "[uf] Grimgram";
			config.fallback["window"]["icon"] 					= "./cfg/icon.png";
			config.fallback["window"]["size"]["x"] 				= 640;
			config.fallback["window"]["size"]["y"] 				= 480;
			config.fallback["window"]["visible"] 				= true;
			config.fallback["window"]["fullscreen"] 			= false;
			config.fallback["cursor"]["visible"] 				= true;
			config.fallback["keyboard"]["repeat"] 				= true;
			config.fallback["hook"]["mode"] 					= "Readable";
			config.fallback["light"]["ambient"]["g"] 			= 0.0f;
			config.fallback["light"]["ambient"]["b"] 			= 0.0f;
			config.fallback["light"]["ambient"]["a"] 			= 1.0f;

			config.fallback["context"]["depthBits"] 			= 24;
			config.fallback["context"]["stencilBits"] 			= 4;
			config.fallback["context"]["bitsPerPixel"] 			= 8;
			config.fallback["context"]["antialiasingLevel"] 	= 0;
			config.fallback["context"]["majorVersion"] 			= 3;
			config.fallback["context"]["minorVersion"] 			= 0;
		}
		/* Merge */ {
			client::config = config.ext;
			client::config.merge( config.fallback, true );
		}
	}
	/* Initialize window */ {
		// Window size
		pod::Vector2i size; {
			size.x = client::config["window"]["size"]["x"].asUInt();
			size.y = client::config["window"]["size"]["y"].asUInt();
		}
		// Window title
		uf::String title; {
			title = client::config["window"]["title"].asString();
		}
		// Terminal window;
		spec::terminal.setVisible( client::config["terminal"]["visible"].asBool() );
		// Ncurses
		uf::IoStream::ncurses = client::config["terminal"]["ncurses"].asBool();

		// Window's context settings
		spec::Context::Settings settings; {
			settings.depthBits 			= client::config["context"]["depthBits"].asUInt();
			settings.stencilBits 		= client::config["context"]["stencilBits"].asUInt();
			settings.bitsPerPixel 		= client::config["context"]["bitsPerPixel"].asUInt();
			settings.antialiasingLevel 	= client::config["context"]["antialiasingLevel"].asUInt();
			settings.majorVersion 		= client::config["context"]["majorVersion"].asUInt();
			settings.minorVersion 		= client::config["context"]["minorVersion"].asUInt();
		}
		client::window.create( size, title, settings );

		// Miscellaneous
		client::window.setVisible(client::config["window"]["visible"].asBool());
		client::window.setCursorVisible(client::config["cursor"]["visible"].asBool());
		client::window.setKeyRepeatEnabled(client::config["keyboard"]["repeat"].asBool());
		client::window.centerWindow();
		client::window.setPosition({0, 0});
	//	client::window.setMouseGrabbed(true);

		/* Set Icon */ {
			uf::Image icon;
			icon.open(client::config["window"]["icon"].asString());
			client::window.setIcon({(int) icon.getDimensions().x, (int) icon.getDimensions().y}, ((uint8_t*)icon.getPixelsPtr()));
		}
		client::window.setTitle(title); {
			uf::Serializer json;
			std::string hook = "window:Title.Changed";
			json["type"] = hook;
			json["invoker"] = "os";
			json["window"]["title"] = std::string(title);
			uf::hooks.call( hook, json );
		}
		uf::hooks.shouldPreferReadable();
		if ( client::config["hook"]["mode"] == "Readable" ) {}
	}
	/* Initialize OpenGL */ {
		if ( !ext::gl.initialize() ) {
			std::cerr << "[ERROR] GL failed to initialize!" << std::endl;
			std::exit(EXIT_SUCCESS);
			return;
		}

		pod::Vector4f ambient;
		ambient.x = client::config["light"]["ambient"]["r"].asDouble();
		ambient.y = client::config["light"]["ambient"]["g"].asDouble();
		ambient.z = client::config["light"]["ambient"]["b"].asDouble();
		ambient.w = client::config["light"]["ambient"]["a"].asDouble();
		glClearColor(ambient.x, ambient.y, ambient.z, ambient.w);
	}
	
	/* Initialize hooks */ {
		if ( client::config["hook"]["mode"] == "Both" || client::config["hook"]["mode"] == "Readable" ) {
			uf::hooks.addHook( "window:Mouse.Lock", [&](const std::string& event)->std::string{
				if ( client::window.hasFocus() ) {
					client::window.setMousePosition(client::window.getSize()/2);
				}
				return "true";
			});
			uf::hooks.addHook( "window:Closed", [&](const std::string& event)->std::string{
				client::ready = false;
				std::exit(EXIT_SUCCESS);
				return "true";
			} );
			uf::hooks.addHook( "window:Title.Changed", [&](const std::string& event)->std::string{
				uf::Serializer json = event;
				if ( json["invoker"] != "os" ) {
					if ( !json["window"].isObject() ) return "false";
					uf::String title = json["window"]["title"].asString();
					client::window.setTitle(title);
				}
				return "true";
			} );
			uf::hooks.addHook( "window:Resized", [&](const std::string& event)->std::string{
				uf::Serializer json = event;
				pod::Vector2i size; {
					size.x = json["window"]["size"]["x"].asUInt64();
					size.y = json["window"]["size"]["y"].asUInt64();
				}
				if ( json["invoker"] != "os" ) {
					client::window.setSize(size);
				}
				// Update viewport
				glViewport( 0, 0, size.x, size.y );
				client::window.centerWindow();

				return "true";
			} );
		} else if ( client::config["hook"]["mode"] == "Both" || client::config["hook"]["mode"] == "Optimal" ) {
			uf::hooks.addHook( "window:Closed", [&](const uf::OptimalHook::argument_t& userdata)->uf::OptimalHook::return_t{
				client::ready = false;
				std::exit(EXIT_SUCCESS);
				return NULL;
			} );
			uf::hooks.addHook( "window:Title.Changed", [&](const uf::OptimalHook::argument_t& userdata)->uf::OptimalHook::return_t{
				// Hook information
				struct Hook {
					std::string type;
					std::string invoker;
					
					struct {
						std::string title;
					} window;
				};
				const Hook& hook = uf::userdata::get<Hook>(userdata);
				if ( hook.invoker != "os" ) {
					uf::String title = hook.window.title;
					client::window.setTitle(title);
				}
				return NULL;
			} );
			uf::hooks.addHook( "window:Resized", [&](const uf::OptimalHook::argument_t& userdata)->uf::OptimalHook::return_t{
				// Hook information
				struct Hook {
					std::string type;
					std::string invoker;
					
					struct {
						pod::Vector2i size;
					} window;
				};
				const Hook& hook = uf::userdata::get<Hook>(userdata);
				if ( hook.invoker != "os" ) {
					client::window.setSize(hook.window.size);
				}
				// Update viewport
				glViewport( 0, 0, hook.window.size.x, hook.window.size.y );
				return NULL;
			} );
		}
	}
	if ( client::config["window"]["fullscreen"].asBool() ) client::window.switchToFullscreen();
	client::ready = true;
}
void client::tick() {
//	uf::hooks.call("system:Tick");
	client::window.pollEvents();
}

void client::render() {
	client::window.display();
}

void client::terminate() {
	client::window.terminate();
	spec::Context::globalCleanup();
}