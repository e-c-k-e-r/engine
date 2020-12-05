#include "../main.h"

#include <uf/utils/window/window.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/image/image.h>
#include <uf/utils/audio/audio.h>
#include <uf/spec/terminal/terminal.h>
#include <uf/utils/hook/hook.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/renderer/renderer.h>
#include <uf/ext/vulkan/vulkan.h>

bool client::ready = false;
bool client::terminated = false;
uf::Window client::window;
uf::Serializer client::config;

void client::initialize() {
	uf::IoStream::ncurses = true;
	uf::renderer::device.window = &client::window;

	ext::load();

	/* Initialize config */ {
		struct {
			uf::Serializer ext;
			uf::Serializer fallback;
		} config;
		/* Get configuration */ {
			config.ext = ext::config.serialize();
		}
		/* Merge */ {
			client::config = config.ext;
			client::config.merge( config.fallback, true );
		}
	}
	/* Initialize window */ {
		// Window size
		pod::Vector2i size; {
			size.x = client::config["window"]["size"]["x"].as<size_t>();
			size.y = client::config["window"]["size"]["y"].as<size_t>();
			// request system size
			if ( size.x <= 0 && size.y <= 0 ) {
				auto resolution = client::window.getResolution();
				client::config["window"]["size"]["x"] = (size.x = resolution.x);
				client::config["window"]["size"]["y"] = (size.y = resolution.y);
			}
		}
		// Window title
		uf::String title; {
			title = client::config["window"]["title"].as<std::string>();
		}
		// Terminal window;
		spec::terminal.setVisible( client::config["window"]["terminal"]["visible"].as<bool>() );
		// Ncurses
		uf::IoStream::ncurses = client::config["window"]["terminal"]["ncurses"].as<bool>();

		// Window's context settings
		uf::renderer::settings::width = size.x;
		uf::renderer::settings::height = size.y;
		client::window.create( size, title );

		// Set refresh rate
		ext::config["window"]["refresh rate"] = client::window.getRefreshRate();

		// Miscellaneous
		client::window.setVisible(client::config["window"]["visible"].as<bool>());
		client::window.setCursorVisible(client::config["window"]["cursor"]["visible"].as<bool>());
		client::window.setKeyRepeatEnabled(client::config["window"]["keyboard"]["repeat"].as<bool>());
	//	client::window.centerWindow();
	//	client::window.setPosition({0, 0});
	//	client::window.setMouseGrabbed(true);

		/* Set Icon */ if ( client::config["window"]["icon"].is<std::string>() ) {
			uf::Image icon;
			icon.open(client::config["window"]["icon"].as<std::string>());
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
	/*
		uf::hooks.shouldPreferReadable();
		if ( client::config["engine"]["hook"]["mode"] == "Readable" ) {

		}
	*/
	}

	/* Initialize OpenAL */ {
		if ( !ext::oal.initialize() ) {
			std::cerr << "[ERROR] AL failed to initialize!" << std::endl;
			std::exit(EXIT_SUCCESS);
			return;
		}
	}
	
	/* Initialize hooks */ {
	//	if ( client::config["engine"]["hook"]["mode"] == "Both" || client::config["engine"]["hook"]["mode"] == "Readable" ) {
			uf::hooks.addHook( "window:Mouse.CursorVisibility", [&](const ext::json::Value& json){
				client::window.setCursorVisible(json["state"].as<bool>());
				client::window.setMouseGrabbed(!json["state"].as<bool>());
				client::config["mouse"]["visible"] = json["state"].as<bool>();
				client::config["window"]["mouse"]["center"] = !json["state"].as<bool>();
			});
			uf::hooks.addHook( "window:Mouse.Lock", [&](const ext::json::Value& json){
				if ( client::window.hasFocus() ) {
					client::window.setMousePosition(client::window.getSize()/2);
				}
			});
			uf::hooks.addHook( "window:Closed", [&](const ext::json::Value& json){
				client::ready = false;
			//	std::exit(EXIT_SUCCESS);
			} );
			uf::hooks.addHook( "window:Title.Changed", [&](const ext::json::Value& json){
				if ( json["invoker"] != "os" ) {
					if ( !ext::json::isObject( json["window"] ) ) return;
					uf::String title = json["window"]["title"].as<std::string>();
					client::window.setTitle(title);
				}
			} );
			uf::hooks.addHook( "window:Resized", [&](const ext::json::Value& json){
				pod::Vector2i size; {
					size.x = json["window"]["size"]["x"].as<size_t>();
					size.y = json["window"]["size"]["y"].as<size_t>();
				}
				if ( json["invoker"] != "os" ) {
					client::window.setSize(size);
				}
				// Update viewport
				if ( !ext::json::isArray( client::config["engine"]["ext"]["vulkan"]["framebuffer"]["size"] ) ) {
					float scale = client::config["engine"]["ext"]["vulkan"]["framebuffer"]["size"].is<double>() ? client::config["engine"]["ext"]["vulkan"]["framebuffer"]["size"].as<float>() : 1;
					uf::renderer::settings::width = size.x * scale;
					uf::renderer::settings::height = size.y * scale;
				}

				uf::renderer::states::resized = true;
			} );
	/*
		}
		else if ( client::config["engine"]["hook"]["mode"] == "Both" || client::config["engine"]["hook"]["mode"] == "Optimal" ) {
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
	*/
	}
	if ( client::config["window"]["mode"].as<std::string>() == "fullscreen" ) client::window.switchToFullscreen();
	if ( client::config["window"]["mode"].as<std::string>() == "borderless" ) client::window.switchToFullscreen( true );
	client::ready = true;
}
void client::tick() {
//	uf::hooks.call("system:Tick");
	client::window.pollEvents();
	// call mouse move
	// query lock
	if ( client::window.hasFocus() && client::config["window"]["mouse"]["center"].as<bool>() ) {
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

	client::window.terminate();

	if ( !ext::oal.terminate() ) {
		std::cerr << "[ERROR] AL failed to terminate!" << std::endl;
		std::exit(EXIT_SUCCESS);
		return;
	}
}