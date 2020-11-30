#include "dialogue.h"

#include ".h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/graphic/mesh.h>
#include <uf/utils/window/window.h>
#include <uf/utils/camera/camera.h>
#include <uf/ext/vulkan/graphic.h>
#include <uf/ext/vulkan/device.h>
#include <uf/ext/vulkan/swapchain.h>
#include <uf/ext/vulkan/vulkan.h>
#include <uf/utils/image/image.h>
#include <uf/utils/audio/audio.h>
#include <uf/engine/asset/asset.h>

#include "../gui/dialogue.h"

namespace {
	ext::Gui* gui;

	uf::Serializer masterTableGet( const std::string& table ) {
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& mastertable = scene.getComponent<uf::Serializer>();
		return mastertable["system"]["mastertable"][table];
	}
	uf::Serializer masterDataGet( const std::string& table, const std::string& key ) {
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& mastertable = scene.getComponent<uf::Serializer>();
		return mastertable["system"]["mastertable"][table][key];
	}
	inline int64_t parseInt( const std::string& str ) {
		return atoi(str.c_str());
	}
}

namespace {
	void playSound( ext::DialogueManager& entity, const std::string& id, const std::string& key ) {
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();

		uf::Serializer cardData = masterDataGet("Card", id);
		uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].as<std::string>());
		std::string name = charaData["filename"].as<std::string>();
		std::string url = "https://cdn..xyz//unity/Android/voice/voice_" + name + "_"+key+".ogg";

		std::cout << url << std::endl;

		if ( charaData["internal"].as<bool>() ) {
			url = "./data/smtsamo/voice/voice_" + name + "_" + key + ".ogg";
		}

		uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
		assetLoader.cache(url, "asset:Cache.Voice." + std::to_string(entity.getUid()));
	}
	void playSound( ext::DialogueManager& entity, std::size_t uid, const std::string& key ) {
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& pMetadata = entity.getComponent<uf::Serializer>();
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();

		uf::Entity*  = scene.findByUid(uid);
		if ( ! ) return;
		uf::Serializer& metadata = ->getComponent<uf::Serializer>();
		std::string id = metadata[""]["id"].as<std::string>();
		playSound( entity, id, key );
	}
	void playSound( ext::DialogueManager& entity, const std::string& key ) {
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& metadata = entity.getComponent<uf::Serializer>();
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();

		std::string url = "./" + key + ".ogg";

		uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
		assetLoader.cache(url, "asset:Cache.SFX." + std::to_string(entity.getUid()));
	}
	void playMusic( ext::DialogueManager& entity, const std::string& filename ) {
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();

		assetLoader.load(filename, "asset:Load." + std::to_string(scene.getUid()));
	}

	uf::Audio sfx;
	uf::Audio voice;
	std::vector<ext::Housamo*> transients;
}
void ext::DialogueManager::initialize() {	
	uf::Object::initialize();

	::gui = NULL;
	transients.clear();

	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
//	std::vector<ext::Housamo>& transients = this->getComponent<std::vector<ext::Housamo>>();

	this->m_name = "Dialogue Manager";

	// asset loading
	this->addHook( "asset:Cache.Voice.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& metadata = this->getComponent<uf::Serializer>();
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();
		
		std::string filename = json["filename"].as<std::string>();

		if ( uf::io::extension(filename) != "ogg" ) return "false";

		if ( filename == "" ) return "false";
		if ( voice.playing() ) voice.stop();
		voice = uf::Audio();
		voice.load(filename);
		voice.setVolume(masterdata["volumes"]["voice"].as<float>());
		voice.play();

		return "true";
	});
	this->addHook( "asset:Cache.SFX.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();
		
		std::string filename = json["filename"].as<std::string>();

		if ( uf::io::extension(filename) != "ogg" ) return "false";

		if ( filename == "" ) return "false";
		if ( sfx.playing() ) sfx.stop();
		sfx = uf::Audio();
		sfx.load(filename);
		sfx.setVolume(masterdata["volumes"]["sfx"].as<float>());
		sfx.play();

		return "true";
	});
	this->addHook( "asset:Music.Load.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		if ( json["music"].is<std::string>() ) playMusic(*this, json["music"].as<std::string>());
		return "true";
	});
	this->addHook( "asset:Sfx.Load.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		if ( json["sfx"].is<std::string>() ) playSound(*this, json["sfx"].as<std::string>());
		return "true";
	});
	this->addHook( "asset:Voice.Load.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		if ( ext::json::isObject( json["voice"] ) ) playSound(*this, json["voice"]["id"].as<std::string>(), json["voice"]["key"].as<std::string>());
		return "true";
	});

	// bind events
	this->addHook( "menu:Dialogue.Start.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		{
			metadata["dialogue"] = json["dialogue"];
			uf::Serializer payload;
			payload["music"] =  metadata["dialogue"]["music"];
			payload["sfx"] =  metadata["dialogue"]["sfx"];
			payload["voice"] =  metadata["dialogue"]["voice"];
			// this->queueHook( "asset:Music.Load.%UID%", payload );
		}
		metadata["uid"] = json["uid"];

		this->queueHook("menu:Dialogue.Gui.%UID%");
		
		uf::Serializer payload;
		payload["dialogue"] = metadata["dialogue"];
		return payload;
	});
	this->addHook( "menu:Dialogue.Gui.%UID%", [&](const std::string& event)->std::string{	
		ext::Gui* guiManager = (ext::Gui*) this->globalFindByName("Gui Manager");
		ext::Gui* guiMenu = new ext::GuiDialogue;
		guiManager->addChild(*guiMenu);
		guiMenu->as<uf::Object>().load("./entities/gui/dialogue/menu.json");
		guiMenu->initialize();

		uf::Serializer payload;
		payload["dialogue"] = metadata["dialogue"];
		guiMenu->queueHook("menu:Dialogue.Start.%UID%", payload);

		::gui = guiMenu;

		return "true";
	});
	// json.action and json.uid
	this->addHook( "menu:Dialogue.Action.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		
		std::string hookName = "";
		uf::Serializer payload;

		std::string action = json["action"].as<std::string>();
		
		if ( action == "dialogue-select" ) {
			payload = json;
			hookName = "menu:Dialogue.Turn.%UID%";
		} else {
			payload["message"] = "Invalid command: %#FF0000%" + action;
			payload["timeout"] = 2.0f;
			payload["invalid"] = true;
			return payload;
		}

		uf::Serializer result;
		if ( hookName != "" ) result = this->callHook(hookName, payload)[0];
		return result;
	});

	this->addHook( "menu:Dialogue.Turn.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;

		uf::Serializer payload;
		std::string index = json["index"].as<std::string>();
		uf::Serializer part = metadata["dialogue"][index];
		if ( part["quit"].as<bool>() ) return this->callHook("menu:Dialogue.End.%UID%")[0];
		payload["message"] = part["message"];
		payload["actions"] = part["actions"];
		if ( part["music"].is<std::string>() ) {
			uf::Serializer pload;
			pload["music"] =  part["music"];
			this->queueHook( "asset:Music.Load.%UID%", pload );
		}
		if ( part["sfx"].is<std::string>() ) {
			uf::Serializer pload;
			pload["sfx"] =  part["sfx"];
			this->queueHook( "asset:Sfx.Load.%UID%", pload );
		}
		if ( ext::json::isObject( part["voice"] ) ) {
			uf::Serializer pload;
			pload["voice"] =  part["voice"];
			this->queueHook( "asset:Voice.Load.%UID%", pload );
		}
		if ( ext::json::isObject( part["hook"] ) ) {
			part["hooks"].emplace_back( part["hook"] );
		}
	//	for ( auto hook : part["hooks"] ) {
		ext::json::forEach(part["hooks"], [&](ext::json::Value& hook){
			hook["payload"]["uid"] = metadata["uid"];
			this->queueHook( hook["name"].as<std::string>(), uf::Serializer{hook["payload"]}, hook["timeout"].as<float>() );
		});
		return payload;
	});
	this->addHook( "menu:Dialogue.End.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		// play music
		uf::Scene& scene = uf::scene::getCurrentScene();
		scene.callHook("world:Music.LoadPrevious.%UID%");
		::gui->callHook("menu:Close.%UID%");

		this->callHook("menu:Dialogue.End");

		uf::Serializer payload;
		payload["end"] = true;
		return payload;
	});
}

void ext::DialogueManager::tick() {	
	uf::Object::tick();
}

void ext::DialogueManager::destroy() {
	uf::Object::destroy();
}
void ext::DialogueManager::render() {	
	uf::Object::render();
}