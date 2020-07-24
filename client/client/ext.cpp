#include "../main.h"

#include <uf/utils/window/window.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/image/image.h>
#include <uf/utils/audio/audio.h>
#include <uf/spec/terminal/terminal.h>
#include <uf/utils/hook/hook.h>
#include <uf/utils/thread/thread.h>
#include <uf/ext/vulkan/vulkan.h>

bool client::ready = false;
bool client::terminated = false;
uf::Window client::window;
uf::Serializer client::config;

void client::initialize() {
	uf::IoStream::ncurses = true;
	ext::vulkan::device.window = &client::window;
	/* Initialize config */ {
		struct {
			uf::Serializer ext;
			uf::Serializer fallback;
		} config;
		/* Get configuration */ {
			config.ext = ext::getConfig();
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
		spec::terminal.setVisible( client::config["window"]["terminal"]["visible"].asBool() );
		// Ncurses
		uf::IoStream::ncurses = client::config["window"]["terminal"]["ncurses"].asBool();

		// Window's context settings
		ext::vulkan::width = size.x;
		ext::vulkan::height = size.y;
		client::window.create( size, title );

		// Miscellaneous
		client::window.setVisible(client::config["window"]["visible"].asBool());
		client::window.setCursorVisible(client::config["window"]["cursor"]["visible"].asBool());
		client::window.setKeyRepeatEnabled(client::config["window"]["keyboard"]["repeat"].asBool());
	//	client::window.centerWindow();
	//	client::window.setPosition({0, 0});
	//	client::window.setMouseGrabbed(true);

		/* Set Icon */ if ( client::config["window"]["icon"].isString() ) {
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
		if ( client::config["engine"]["hook"]["mode"] == "Readable" ) {}
	}

	/* Initialize OpenAL */ {
		if ( !ext::oal.initialize() ) {
			std::cerr << "[ERROR] AL failed to initialize!" << std::endl;
			std::exit(EXIT_SUCCESS);
			return;
		}
	}
	
	/* Initialize hooks */ {
		if ( client::config["engine"]["hook"]["mode"] == "Both" || client::config["engine"]["hook"]["mode"] == "Readable" ) {
			uf::hooks.addHook( "window:Mouse.CursorVisibility", [&](const std::string& event)->std::string{
				uf::Serializer json = event;
				client::window.setCursorVisible(json["state"].asBool());
				client::window.setMouseGrabbed(!json["state"].asBool());
				client::config["mouse"]["visible"] = json["state"].asBool();
				client::config["window"]["mouse"]["center"] = !json["state"].asBool();
				return "true";
			});
			uf::hooks.addHook( "window:Mouse.Lock", [&](const std::string& event)->std::string{
				if ( client::window.hasFocus() ) {
					client::window.setMousePosition(client::window.getSize()/2);
				}
				return "true";
			});
			uf::hooks.addHook( "window:Closed", [&](const std::string& event)->std::string{
				client::ready = false;
			//	std::exit(EXIT_SUCCESS);
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
				ext::vulkan::width = size.x;
				ext::vulkan::height = size.y;

				ext::vulkan::rebuild = true;
				return "true";
			} );
		} else if ( client::config["engine"]["hook"]["mode"] == "Both" || client::config["engine"]["hook"]["mode"] == "Optimal" ) {
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
	// call mouse move
	// query lock
	if ( client::window.hasFocus() && client::config["window"]["mouse"]["center"].asBool() ) {
		auto previous = client::window.getMousePosition();
		client::window.setMousePosition(client::window.getSize()/2);
		auto current = client::window.getMousePosition();

		auto size = client::window.getSize();
		uf::Serializer payload;
		payload["invoker"] = "client";
		payload["mouse"]["delta"]["x"] = previous.x - current.x;
		payload["mouse"]["delta"]["y"] = previous.y - current.y;
		payload["mouse"]["position"]["x"] = current.x;
		payload["mouse"]["position"]["y"] = current.y;
		payload["mouse"]["size"]["x"] = size.x;
		payload["mouse"]["size"]["y"] = size.y;
		payload["mouse"]["state"] = "???";
		payload["type"] = "window:Mouse.Moved";
		uf::hooks.call("window:Mouse.Moved", payload);
	}
/*
{
        "invoker" : "os",
        "mouse" :
        {
                "delta" :
                {
                        "x" : -17,
                        "y" : -12
                },
                "position" :
                {
                        "x" : 371,
                        "y" : 276
                },
                "size" :
                {
                        "x" : 800,
                        "y" : 600
                },
                "state" : "???"
        },
        "type" : "window:Mouse.Moved"
}
*/
}

void client::render() {
	client::window.display();
}

void client::terminate() {
	/* Close Threads */ {
		uf::thread::terminate();
	}

	/* Close vulkan */ {
		ext::vulkan::destroy();
	}

	client::window.terminate();

	if ( !ext::oal.terminate() ) {
		std::cerr << "[ERROR] AL failed to terminate!" << std::endl;
		std::exit(EXIT_SUCCESS);
		return;
	}
}