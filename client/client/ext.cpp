#include "../main.h"

#include <uf/utils/io/inputs.h>
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
		client::window.setCursorVisible(client::config["window"]["mouse"]["visible"].as<bool>());
		if ( client::config["engine"]["ext"]["imgui"]["enabled"].as<bool>() ) {
			client::window.setCursorVisible(false);
		}
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
		uf::hooks.addHook( "window:Mouse.CursorVisibility", [&]( pod::payloads::windowMouseCursorVisibility& payload ){
			if ( !client::config["engine"]["ext"]["imgui"]["enabled"].as<bool>() ) {
				client::window.setCursorVisible(payload.mouse.visible);
			} else {
				client::window.setCursorVisible(false);
			}
			client::window.setMouseGrabbed(!payload.mouse.visible);
			client::config["mouse"]["visible"] = payload.mouse.visible;
			client::config["window"]["mouse"]["center"] = !payload.mouse.visible;
		});
		uf::hooks.addHook( "window:Mouse.Lock", [&](){
			if ( client::window.hasFocus() ) {
				client::window.setMousePosition(client::window.getSize()/2);
			}
		});
		uf::hooks.addHook( "window:Closed", [&]( pod::payloads::windowEvent& json ){
			client::ready = false;
		} );
		uf::hooks.addHook( "window:Title.Changed", [&]( ext::json::Value& json ){
			if ( json["invoker"] != "os" ) {
				if ( !ext::json::isObject( json["window"] ) ) return;
				uf::String title = json["window"]["title"].as<std::string>();
				client::window.setTitle(title);
			}
		} );
		uf::hooks.addHook( "window:Resized", [&]( pod::payloads::windowResized& payload ){
			if ( payload.window.size.x == uf::renderer::settings::width && payload.window.size.y == uf::renderer::settings::height ) return;
			
			if ( payload.invoker != "os" ) client::window.setSize(payload.window.size);
			// Update viewport

		#if UF_USE_VULKAN
			auto& configRenderJson = client::config["engine"]["ext"]["vulkan"];
		#elif UF_USE_OPENGL
			auto& configRenderJson = client::config["engine"]["ext"]["opengl"];
		#else
			auto& configRenderJson = client::config["engine"]["ext"]["software"];
		#endif

			if ( !ext::json::isArray( configRenderJson["framebuffer"]["size"] ) ) {
				uf::renderer::settings::width = payload.window.size.x;
				uf::renderer::settings::height = payload.window.size.y;
			}
			uf::renderer::states::resized = true;
		} );
	}
#if !UF_ENV_DREAMCAST
	if ( client::config["window"]["mode"].as<std::string>() == "fullscreen" ) client::window.toggleFullscreen();
	else if ( client::config["window"]["mode"].as<std::string>() == "borderless" ) client::window.toggleFullscreen( true );
#endif

	client::ready = true;
#if UF_ENV_DREAMCAST
	client::window.pollEvents();
#endif
}

void client::tick() {
	client::window.bufferInputs();
	client::window.pollEvents();

	if ( client::window.hasFocus() ) {
		// fullscreener
		TIMER(1, (uf::inputs::kbm::states::LAlt || uf::inputs::kbm::states::RAlt) && uf::inputs::kbm::states::Enter ) {
			uf::renderer::states::resized = true;
			client::window.toggleFullscreen( false );
		}
		// mouse move
		uf::inputs::kbm::states::Mouse = {};
		if ( client::config["window"]["mouse"]["center"].as<bool>(false) ) {
			auto size = client::window.getSize();
			auto current = client::window.getMousePosition();
			auto center = client::window.getSize() / 2.0f;
			client::window.setMousePosition(client::window.getSize() / 2.0f);
			client::window.setCursorVisible(false);

		#if UF_INPUT_USE_ENUM_MOUSE
			uf::inputs::kbm::states::Mouse = {
				(current.x - center.x) / (float) size.x,
				(current.y - center.y) / (float) size.y,
			};
		#else
			uf::hooks.call("window:Mouse.Moved", pod::payloads::windowMouseMoved{
				{
					{ "window:Mouse.Moved", "client", },
					{ pod::Vector2ui{ size.x, size.y }, },
				},
				{ center, current - center, 0 }
			});
		#endif
		} else {
		#if UF_INPUT_USE_ENUM_MOUSE
		//	uf::inputs::kbm::states::Mouse = { 0, 0 };
		#endif
		}
	}
}

void client::render() {
	client::window.display();
}

void client::terminate() {
	client::window.terminate();
}