#include "../main.h"

#include <uf/utils/window/window.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/image/image.h>
#include <uf/utils/audio/audio.h>
#include <uf/spec/terminal/terminal.h>
#include <uf/utils/hook/hook.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/renderer/renderer.h>
#include <uf/utils/window/payloads.h>

bool client::ready = false;
bool client::terminated = false;
uf::Window client::window;
uf::Serializer client::config;

void client::initialize() {
	uf::IoStream::ncurses = true;
	uf::renderer::device.window = &client::window;

	ext::load();

	client::config = ext::config;

	/* Initialize window */ {
		// Window size
		pod::Vector2i size = uf::vector::decode( client::config["window"]["size"], pod::Vector2i{} );
		// request system size
		if ( size.x <= 0 && size.y <= 0 ) {
			auto resolution = client::window.getResolution();
			client::config["window"]["size"][0] = (size.x = resolution.x);
			client::config["window"]["size"][1] = (size.y = resolution.y);
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
	#if !UF_ENV_DREAMCAST
		// Set refresh rate
		ext::config["window"]["refresh rate"] = client::window.getRefreshRate();
		// Miscellaneous
		client::window.setVisible(client::config["window"]["visible"].as<bool>());
		client::window.setCursorVisible(client::config["window"]["cursor"]["visible"].as<bool>());
		client::window.setKeyRepeatEnabled(client::config["window"]["keyboard"]["repeat"].as<bool>());
	//	client::window.centerWindow();
	//	client::window.setPosition({0, 0});
	//	client::window.setMouseGrabbed(true);

		if ( client::config["window"]["icon"].is<std::string>() ) {
			uf::Image icon;
			icon.open(client::config["window"]["icon"].as<std::string>());
			client::window.setIcon({(int) icon.getDimensions().x, (int) icon.getDimensions().y}, ((uint8_t*)icon.getPixelsPtr()));
		}

		client::window.setTitle(title);
	#endif
	}
	
	/* Initialize hooks */ {
		uf::hooks.addHook( "window:Mouse.CursorVisibility", [&]( ext::json::Value& json ){
			client::window.setCursorVisible(json["state"].as<bool>());
			client::window.setMouseGrabbed(!json["state"].as<bool>());
			client::config["mouse"]["visible"] = json["state"].as<bool>();
			client::config["window"]["mouse"]["center"] = !json["state"].as<bool>();
		});
		uf::hooks.addHook( "window:Mouse.Lock", [&]( ext::json::Value& json ){
			if ( client::window.hasFocus() ) {
				client::window.setMousePosition(client::window.getSize()/2);
			}
		});
		uf::hooks.addHook( "window:Mouse.Lock", [&](){
			if ( client::window.hasFocus() ) {
				client::window.setMousePosition(client::window.getSize()/2);
			}
		});
		uf::hooks.addHook( "window:Closed", [&]( ext::json::Value& json ){
			client::ready = false;
		} );
		uf::hooks.addHook( "window:Title.Changed", [&]( ext::json::Value& json ){
			if ( json["invoker"] != "os" ) {
				if ( !ext::json::isObject( json["window"] ) ) return;
				uf::String title = json["window"]["title"].as<std::string>();
				client::window.setTitle(title);
			}
		} );
		uf::hooks.addHook( "window:Resized", [&]( ext::json::Value& json ){
			pod::Vector2i size = uf::vector::decode( json["window"]["size"], pod::Vector2i{} );
			if ( size.x == uf::renderer::settings::width && size.y == uf::renderer::settings::height ) return;
			
			if ( json["invoker"] != "os" ) client::window.setSize(size);
			// Update viewport
			if ( !ext::json::isArray( client::config["engine"]["ext"]["vulkan"]["framebuffer"]["size"] ) ) {
				float scale = client::config["engine"]["ext"]["vulkan"]["framebuffer"]["size"].is<double>() ? client::config["engine"]["ext"]["vulkan"]["framebuffer"]["size"].as<float>() : 1;
				uf::renderer::settings::width = size.x * scale;
				uf::renderer::settings::height = size.y * scale;
			}
			uf::renderer::states::resized = true;
		} );
	}
#if !UF_ENV_DREAMCAST
	if ( client::config["window"]["mode"].as<std::string>() == "fullscreen" ) client::window.switchToFullscreen();
	if ( client::config["window"]["mode"].as<std::string>() == "borderless" ) client::window.switchToFullscreen( true );
#endif

	client::ready = true;
#if UF_ENV_DREAMCAST
	client::window.pollEvents();
#endif
}

void client::tick() {
	client::window.pollEvents();

	if ( client::window.hasFocus() && client::config["window"]["mouse"]["center"].as<bool>() ) {
		auto previous = client::window.getMousePosition();
		client::window.setMousePosition(client::window.getSize()/2);
		auto current = client::window.getMousePosition();
		auto size = client::window.getSize();
	#if 0
		{
			uf::Serializer payload;
			payload["invoker"] = "client";
			payload["mouse"]["delta"] = uf::vector::encode( previous - current );
			payload["mouse"]["position"] = uf::vector::encode( current );
			payload["mouse"]["size"] = uf::vector::encode( size );
			payload["mouse"]["state"] = "???";
			payload["type"] = "window:Mouse.Moved";
			uf::hooks.call("window:Mouse.Moved", payload);
		}
	#endif
		uf::hooks.call("window:Mouse.Moved", pod::payloads::windowMouseMoved{
			{
				{
					"window:Mouse.Moved",
					"client",
				},
				{
					pod::Vector2ui{ size.x, size.y },
				},
			},
			{
				current,
				previous - current,
				0
			}
		});
	}
}

void client::render() {
	client::window.display();
}

void client::terminate() {
	/* Close Threads */ {
		uf::thread::terminate();
	}

	client::window.terminate();
}