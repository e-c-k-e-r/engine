#include "world.h"
#include <uf/utils/time/time.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/window/window.h>

#include <uf/utils/audio/audio.h>
#include <uf/utils/thread/thread.h>

#include "../ext.h"
#include "../asset/asset.h"
#include "../asset/masterdata.h"
#include ".//battle.h"
#include "./gui/menu.h"
#include "./gui/battle.h"
#include ".//dialogue.h"
#include "./gui/dialogue.h"

#include <uf/ext/vulkan/vulkan.h>

namespace {
	uf::Camera* camera;
}

void ext::World::initialize() {
	ext::Object::initialize();

	this->m_name = "World";

	this->load("./entities/world.json");

	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	ext::Asset& assetLoader = this->getComponent<ext::Asset>();

	/* Grab master data */ {
		std::vector<std::string> tables = {
			"Card",
			"Chara",
			"Skill",
			"Element",
			"Status",
		};
		for ( auto& table : tables ) {
			ext::MasterData data;
			metadata["system"]["mastertable"][table] = data.load(table);
		}
	}

	this->addHook( "system:Quit.%UID%", [&](const std::string& event)->std::string{
		std::cout << event << std::endl;
		ext::ready = false;
		return "true";
	});
	this->addHook( "world:Music.LoadPrevious.%UID%", [&](const std::string& event)->std::string{
		uf::Serializer json = event;

		if ( metadata["previous bgm"]["filename"] == "" ) return "false";

		std::string filename = metadata["previous bgm"]["filename"].asString();
		float timestamp = metadata["previous bgm"]["timestamp"].asFloat();

//		std::cout << metadata["previous bgm"] << std::endl;

		uf::Audio& audio = this->getComponent<uf::Audio>();
		if ( audio.playing() ) {
			metadata["previous bgm"]["filename"] = audio.getFilename();
			metadata["previous bgm"]["timestamp"] = audio.getTime();
			audio.stop();
		}
		audio.load(filename);
		audio.setVolume(metadata["volumes"]["bgm"].asFloat());
		audio.setTime(timestamp);
		audio.play();

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

	static uf::Timer<long long> timer(false);
	if ( !timer.running() ) timer.start();
	this->addHook( "menu:Pause", [&](const std::string& event)->std::string{
		if ( timer.elapsed().asDouble() < 1 ) return "false";
		timer.reset();

		uf::Serializer json = event;
		ext::Gui* guiManager = (ext::Gui*) this->findByName("Gui Manager");
		if ( !guiManager ) return "false";

		ext::Gui* guiMenu = new ext::GuiMenu;
		uf::Serializer& pMetadata = guiMenu->getComponent<uf::Serializer>();
		guiManager->addChild(*guiMenu);
		guiMenu->load("./entities/gui/pause/menu.json");
		pMetadata["menu"] = json["menu"];
		guiMenu->initialize();

		uf::Serializer payload;
		payload["uid"] = guiMenu->getUid();
		return payload;
	});
	this->addHook( "world:Entity.LoadAsset", [&](const std::string& event)->std::string{
		uf::Serializer json = event;

		std::string asset = json["asset"].asString();
		std::string uid = json["uid"].asString();

		assetLoader.load(asset, "asset:Load." + uid);

		return "true";
	});
	this->addHook( "world:Battle.Prepare", [&](const std::string& event)->std::string{
		uf::Serializer json = event;

		ext::Player& player = this->getPlayer();
		uf::Serializer& pMetadata = player.getComponent<uf::Serializer>();

		uf::Serializer payload;
		
		payload["battle"]["music"] = json["music"];
		payload["battle"]["enemy"] = json[""];
		payload["battle"]["enemy"]["uid"] = json["uid"];

		payload["battle"]["player"] = pMetadata[""];
		payload["battle"]["player"]["uid"] = player.getUid();

		pMetadata["system"]["control"] = false;
		pMetadata["system"]["menu"] = "battle";
			
		this->callHook("world:Battle.Start", payload);

		return "true";
	});
	this->addHook( "world:Battle.Start", [&](const std::string& event)->std::string{
		if ( timer.elapsed().asDouble() < 1 ) return "false";
		timer.reset();
		ext::Gui* guiManager = (ext::Gui*) this->findByName("Gui Manager");
		if ( !guiManager ) return "false";
		uf::Serializer json = event;

		ext::HousamoBattle* battleManager = new ext::HousamoBattle;
		this->addChild(*battleManager);
		battleManager->initialize();
		battleManager->callHook("world:Battle.Start.%UID%", json);

		uf::Serializer payload;
		return payload;
	});
	this->addHook( "world:Battle.End", [&](const std::string& event)->std::string{
		uf::Serializer json = event;
		ext::HousamoBattle* battleManager = (ext::HousamoBattle*) this->findByName("Battle Manager");
		if ( !battleManager ) return "false";

		this->removeChild(*battleManager);
		battleManager->destroy();
		delete battleManager;

		return "true";
	});
	this->addHook( "menu:Dialogue.Start", [&](const std::string& event)->std::string{
		if ( timer.elapsed().asDouble() < 1 ) return "false";
		timer.reset();
		ext::Gui* guiManager = (ext::Gui*) this->findByName("Gui Manager");
		if ( !guiManager ) return "false";
		uf::Serializer json = event;

		ext::DialogueManager* dialogueManager = new ext::DialogueManager;
		this->addChild(*dialogueManager);
		dialogueManager->initialize();
		dialogueManager->callHook("menu:Dialogue.Start.%UID%", json);
		return json;
	});
	this->addHook( "menu:Dialogue.End", [&](const std::string& event)->std::string{
		uf::Serializer json = event;
		ext::DialogueManager* dialogueManager = (ext::DialogueManager*) this->findByName("Dialogue Manager");
		if ( !dialogueManager ) return "false";

		this->removeChild(*dialogueManager);
		dialogueManager->destroy();
		delete dialogueManager;

		return "true";
	});

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
}

void ext::World::tick() {
	ext::Object::tick();

	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	ext::Asset& assetLoader = this->getComponent<ext::Asset>();

	/* check if audio needs to loop */ try {
		uf::Audio& bgm = this->getComponent<uf::Audio>();
		float current = bgm.getTime();
		float end = bgm.getDuration();
		float epsilon = 0.005f;
		if ( current + epsilon >= end || !bgm.playing() ) {
			// intro to main transition
			std::string filename = bgm.getFilename();
			filename = assetLoader.getOriginal(filename);
			if ( filename.find("_intro") != std::string::npos ) {
				assetLoader.load(uf::string::replace( filename, "_intro", "" ), "asset:Load." + std::to_string(this->getUid()));
			// loop
			} else {
				bgm.setTime(0);
				if ( !bgm.playing() ) bgm.play();
			}
		}
	} catch ( ... ) {

	}

	/* Regain control if nothing requests it */ {
		ext::Gui* menu = (ext::Gui*) this->findByName("Gui: Menu");
		if ( !menu ) {
			uf::Serializer payload;
			payload["state"] = false;
			uf::hooks.call("window:Mouse.CursorVisibility", payload);
			uf::hooks.call("window:Mouse.Lock");
		}
	}

	/* Print World Tree */ {
		static uf::Timer<long long> timer(false);
		if ( !timer.running() ) timer.start();
		if ( uf::Window::isKeyPressed("U") && timer.elapsed().asDouble() >= 1 ) { timer.reset();
			std::function<void(const uf::Entity*, int)> recurse = [&]( const uf::Entity* parent, int indent ) {
				for ( const uf::Entity* entity : parent->getChildren() ) {
					for ( int i = 0; i < indent; ++i ) uf::iostream<<"\t";
					uf::iostream<<entity->getName()<<": "<<entity->getUid();
					if ( entity->hasComponent<pod::Transform<>>() ) {
						const pod::Transform<>& t = uf::transform::flatten(entity->getComponent<pod::Transform<>>());
						uf::iostream << " (" << t.position.x << ", " << t.position.y << ", " << t.position.z << ")";
					}
					uf::iostream<<"\n";
					recurse(entity, indent + 1);
				}
			}; recurse(this, 0);
		}
	}
	/* Print Entity Information */  {
		static uf::Timer<long long> timer(false);
		if ( !timer.running() ) timer.start();
		if ( uf::Window::isKeyPressed("U") && timer.elapsed().asDouble() >= 1 ) { timer.reset();
			uf::iostream << "Current size: " << uf::Entity::entities.size() << " | UIDs: " << uf::Entity::uids << "\n";
			uint i = 0; for ( uf::Entity* e : uf::Entity::entities ) {
				if ( e && !e->hasParent() ) {
					++i;
					uf::iostream << "Orphan: " << e->getName() << ": " << e << "\n";
				}
			}
			uf::iostream << "Orphans: " << i << "\n";
		}
	}
	/* Print Entity Information */  {
		static uf::Timer<long long> timer(false);
		if ( !timer.running() ) timer.start();
		if ( uf::Window::isKeyPressed("P") && timer.elapsed().asDouble() >= 1 ) { timer.reset();
	    	uf::iostream << ext::vulkan::allocatorStats() << "\n";
		}
	}
	
	/* Updates Sound Listener */ {
		ext::Player& player = this->getPlayer();
		pod::Transform<>& transform = player.getComponent<pod::Transform<>>();
		
		ext::oal.listener( "POSITION", { transform.position.x, transform.position.y, transform.position.z } );
		ext::oal.listener( "VELOCITY", { 0, 0, 0 } );
		ext::oal.listener( "ORIENTATION", { 0, 0, 1, 1, 0, 0 } );
	}
}


uf::Camera& ext::World::getCamera() {
	if ( !::camera ) ::camera = this->getPlayer().getComponentPointer<uf::Camera>();
	return *::camera;
}

void ext::World::render() {
	ext::Object::render();
}

ext::Player& ext::World::getPlayer() {
	return *((ext::Player*) this->findByName("Player"));
}
const ext::Player& ext::World::getPlayer() const {
	return *((const ext::Player*) this->findByName("Player"));
}