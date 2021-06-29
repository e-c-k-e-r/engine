#if 0
#include "dialogue.h"

#include ".h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/graphic/mesh.h>
#include <uf/utils/window/window.h>
#include <uf/utils/camera/camera.h>
//#include <uf/ext/renderer/renderer.h>

#include <uf/utils/image/image.h>
#include <uf/utils/audio/audio.h>
#include <uf/engine/asset/asset.h>

#include "../gui/dialogue.h"

namespace {
	ext::Gui* gui;

	uf::Serializer masterTableGet( const uf::stl::string& table ) {
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& mastertable = scene.getComponent<uf::Serializer>();
		return mastertable["system"]["mastertable"][table];
	}
	uf::Serializer masterDataGet( const uf::stl::string& table, const uf::stl::string& key ) {
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& mastertable = scene.getComponent<uf::Serializer>();
		return mastertable["system"]["mastertable"][table][key];
	}
	inline int64_t parseInt( const uf::stl::string& str ) {
		return atoi(str.c_str());
	}
}

namespace {
	void playSound( ext::DialogueManager& entity, const uf::stl::string& id, const uf::stl::string& key ) {
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();

		uf::Serializer cardData = masterDataGet("Card", id);
		uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].as<uf::stl::string>());
		uf::stl::string name = charaData["filename"].as<uf::stl::string>();
		uf::stl::string url = "https://cdn..xyz//unity/Android/voice/voice_" + name + "_"+key+".ogg";

		std::cout << url << std::endl;

		if ( charaData["internal"].as<bool>() ) {
			url = uf::io::root+"/smtsamo/voice/voice_" + name + "_" + key + ".ogg";
		}

		uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
		assetLoader.cache(url, "asset:Cache.Voice." + std::to_string(entity.getUid()));
	}
	void playSound( ext::DialogueManager& entity, std::size_t uid, const uf::stl::string& key ) {
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& pMetadata = entity.getComponent<uf::Serializer>();
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();

		uf::Entity*  = scene.findByUid(uid);
		if ( ! ) return;
		uf::Serializer& metadata = ->getComponent<uf::Serializer>();
		uf::stl::string id = metadata[""]["id"].as<uf::stl::string>();
		playSound( entity, id, key );
	}
	void playSound( ext::DialogueManager& entity, const uf::stl::string& key ) {
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& metadata = entity.getComponent<uf::Serializer>();
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();

		uf::stl::string url = "./" + key + ".ogg";

		uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
		assetLoader.cache(url, "asset:Cache.SFX." + std::to_string(entity.getUid()));
	}
	void playMusic( ext::DialogueManager& entity, const uf::stl::string& filename ) {
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();

		assetLoader.load(filename, "asset:Load." + std::to_string(scene.getUid()));
	}

	uf::Audio sfx;
	uf::Audio voice;
	uf::stl::vector<ext::Housamo*> transients;
}
void ext::DialogueManager::initialize() {	
	uf::Object::initialize();

	::gui = NULL;
	transients.clear();

	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
//	uf::stl::vector<ext::Housamo>& transients = this->getComponent<uf::stl::vector<ext::Housamo>>();

	this->m_name = "Dialogue Manager";

	// asset loading
	this->addHook( "asset:Cache.Voice.%UID%", [&](const uf::stl::string& event)->uf::stl::string{	
		uf::Serializer json = event;
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& metadata = this->getComponent<uf::Serializer>();
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();
		
		uf::stl::string filename = json["filename"].as<uf::stl::string>();

		if ( uf::io::extension(filename) != "ogg" ) return "false";

		if ( filename == "" ) return "false";
		if ( voice.playing() ) voice.stop();
		voice = uf::Audio();
		voice.load(filename);
		voice.setVolume(masterdata["volumes"]["voice"].as<float>());
		voice.play();

		return "true";
	});
	this->addHook( "asset:Cache.SFX.%UID%", [&](const uf::stl::string& event)->uf::stl::string{	
		uf::Serializer json = event;
		
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();
		
		uf::stl::string filename = json["filename"].as<uf::stl::string>();

		if ( uf::io::extension(filename) != "ogg" ) return "false";

		if ( filename == "" ) return "false";
		if ( sfx.playing() ) sfx.stop();
		sfx = uf::Audio();
		sfx.load(filename);
		sfx.setVolume(masterdata["volumes"]["sfx"].as<float>());
		sfx.play();

		return "true";
	});
	this->addHook( "asset:Music.Load.%UID%", [&](const uf::stl::string& event)->uf::stl::string{	
		uf::Serializer json = event;
		if ( json["music"].is<uf::stl::string>() ) playMusic(*this, json["music"].as<uf::stl::string>());
		return "true";
	});
	this->addHook( "asset:Sfx.Load.%UID%", [&](const uf::stl::string& event)->uf::stl::string{	
		uf::Serializer json = event;
		if ( json["sfx"].is<uf::stl::string>() ) playSound(*this, json["sfx"].as<uf::stl::string>());
		return "true";
	});
	this->addHook( "asset:Voice.Load.%UID%", [&](const uf::stl::string& event)->uf::stl::string{	
		uf::Serializer json = event;
		if ( ext::json::isObject( json["voice"] ) ) playSound(*this, json["voice"]["id"].as<uf::stl::string>(), json["voice"]["key"].as<uf::stl::string>());
		return "true";
	});

	// bind events
	this->addHook( "menu:Dialogue.Start.%UID%", [&](const uf::stl::string& event)->uf::stl::string{	
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
	this->addHook( "menu:Dialogue.Gui.%UID%", [&](const uf::stl::string& event)->uf::stl::string{	
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
	this->addHook( "menu:Dialogue.Action.%UID%", [&](const uf::stl::string& event)->uf::stl::string{	
		uf::Serializer json = event;
		
		uf::stl::string hookName = "";
		uf::Serializer payload;

		uf::stl::string action = json["action"].as<uf::stl::string>();
		
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
		if ( hookName != "" ) result = this->callHook(hookName, payload)[0].as<uf::Serializer>();
		return result;
	});

	this->addHook( "menu:Dialogue.Turn.%UID%", [&](const uf::stl::string& event)->uf::stl::string{	
		uf::Serializer json = event;

		uf::Serializer payload;
		uf::stl::string index = json["index"].as<uf::stl::string>();
		uf::Serializer part = metadata["dialogue"][index];
		if ( part["quit"].as<bool>() ) return this->callHook("menu:Dialogue.End.%UID%")[0].as<uf::Serializer>();
		payload["message"] = part["message"];
		payload["actions"] = part["actions"];
		if ( part["music"].is<uf::stl::string>() ) {
			uf::Serializer pload;
			pload["music"] =  part["music"];
			this->queueHook( "asset:Music.Load.%UID%", pload );
		}
		if ( part["sfx"].is<uf::stl::string>() ) {
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
			this->queueHook( hook["name"].as<uf::stl::string>(), uf::Serializer{hook["payload"]}, hook["timeout"].as<float>() );
		});
		return payload;
	});
	this->addHook( "menu:Dialogue.End.%UID%", [&](const uf::stl::string& event)->uf::stl::string{	
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
#endif