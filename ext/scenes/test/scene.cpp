#include "scene.h"
#include <uf/utils/time/time.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/window/window.h>

#include <uf/utils/audio/audio.h>
#include <uf/utils/thread/thread.h>

#include <uf/engine/asset/asset.h>
#include <uf/engine/asset/masterdata.h>

#include <uf/ext/vulkan/vulkan.h>

#include "../../ext.h"
#include "../../gui/gui.h"

EXT_OBJECT_REGISTER_CPP(TestScene)
void ext::TestScene::initialize() {
	uf::Scene::initialize();
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Asset& assetLoader = this->getComponent<uf::Asset>();

	this->addHook( "system:Quit.%UID%", [&](const std::string& event)->std::string{
		std::cout << event << std::endl;
		ext::ready = false;
		return "true";
	});
	this->addHook( "asset:Load." + std::to_string(this->getUid()), [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		std::string filename = json["filename"].asString();

		if ( uf::string::extension(filename) != "ogg" ) return "false";
		const uf::Audio* audioPointer = NULL;
		try { audioPointer = &assetLoader.get<uf::Audio>(filename); } catch ( ... ) {}
		if ( !audioPointer ) return "false";

		uf::Audio& audio = this->getComponent<uf::Audio>();
		if ( audio.playing() ) {
		/*
			if ( filename.find("_intro") == std::string::npos ) {
				metadata["previous bgm"]["filename"] = audio.getFilename();
				metadata["previous bgm"]["timestamp"] = audio.getTime();
			}
		*/
			audio.stop();
		}
		
//		std::cout << metadata["previous bgm"] << std::endl;

		audio.load(filename);
		audio.setVolume(metadata["volumes"]["bgm"].asFloat());
		audio.play();

		return "true";
	});
	{
		static uf::Timer<long long> timer(false);
		if ( !timer.running() ) timer.start();
		this->addHook( "world:Entity.LoadAsset", [&](const std::string& event)->std::string{
			uf::Serializer json = event;

			std::string asset = json["asset"].asString();
			std::string uid = json["uid"].asString();

			assetLoader.load(asset, "asset:Load." + uid);

			return "true";
		});
	}
	{
		static uf::Timer<long long> timer(false);
		if ( !timer.running() ) timer.start();
		this->addHook( "menu:Pause", [&](const std::string& event)->std::string{
			if ( timer.elapsed().asDouble() < 1 ) return "false";
			timer.reset();

			uf::Serializer json = event;
			ext::Gui* manager = (ext::Gui*) this->findByName("Gui Manager");
			if ( !manager ) return "false";
			uf::Serializer payload;
			ext::Gui* gui = (ext::Gui*) manager->findByUid( (payload["uid"] = manager->loadChild("/scenes/world/gui/pause/menu.json", false)).asUInt64() );
			uf::Serializer& metadata = gui->getComponent<uf::Serializer>();
			metadata["menu"] = json["menu"];
			gui->initialize();
			return payload;
		});
	}

	/* store viewport size */ {
		metadata["window"]["size"]["x"] = ext::vulkan::width;
		metadata["window"]["size"]["y"] = ext::vulkan::height;
		
		this->addHook( "window:Resized", [&](const std::string& event)->std::string{
			uf::Serializer json = event;

			pod::Vector2ui size; {
				size.x = json["window"]["size"]["x"].asUInt64();
				size.y = json["window"]["size"]["y"].asUInt64();
			}

			metadata["window"] = json["window"];

			return "true";
		});
	}

	// lock control
	{
		uf::Serializer payload;
		payload["state"] = true;
		uf::hooks.call("window:Mouse.CursorVisibility", payload);
		uf::hooks.call("window:Mouse.Lock");
	}
}

void ext::TestScene::tick() {
	uf::Scene::tick();

	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Asset& assetLoader = this->getComponent<uf::Asset>();

	/* Regain control if nothing requests it */ {
		ext::Gui* menu = (ext::Gui*) this->findByName("Gui: Menu");
		if ( !menu ) {
			uf::Serializer payload;
			payload["state"] = false;
			uf::hooks.call("window:Mouse.CursorVisibility", payload);
			uf::hooks.call("window:Mouse.Lock");
		}
	}

	/* Updates Sound Listener */ {
		pod::Transform<>& transform = this->getController()->getComponent<pod::Transform<>>();
		
		ext::oal.listener( "POSITION", { transform.position.x, transform.position.y, transform.position.z } );
		ext::oal.listener( "VELOCITY", { 0, 0, 0 } );
		ext::oal.listener( "ORIENTATION", { 0, 0, 1, 1, 0, 0 } );
	}
}