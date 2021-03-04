#if 0
#include "dialogue.h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/window/window.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/string/ext.h>
// #include <uf/gl/glyph/glyph.h>
#include <uf/engine/asset/asset.h>
#include <uf/utils/math/physics.h>

#include <unordered_map>

//#include <uf/ext/renderer/renderer.h>

#include <uf/utils/http/http.h>
#include <uf/utils/audio/audio.h>
#include <sys/stat.h>
#include <fstream>

#include "..//dialogue.h"

namespace {
	ext::Gui* circleIn;
	ext::Gui* circleOut;
	ext::Gui* dialogueMessage;
	ext::Gui* dialogueOptions;

	ext::DialogueManager* dialogueManager;

	uf::Serializer masterTableGet( const std::string& table ) {
		auto& scene = uf::scene::getCurrentScene();
		uf::Serializer& mastertable = scene.getComponent<uf::Serializer>();
		return mastertable["system"]["mastertable"][table];
	}
	uf::Serializer masterDataGet( const std::string& table, const std::string& key ) {
		auto& scene = uf::scene::getCurrentScene();
		uf::Serializer& mastertable = scene.getComponent<uf::Serializer>();
		return mastertable["system"]["mastertable"][table][key];
	}
	inline int64_t parseInt( const std::string& str ) {
		return atoi(str.c_str());
	}

	void playSound( ext::GuiDialogue& entity, const std::string& id, const std::string& key ) {
		auto& scene = uf::scene::getCurrentScene();
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();

		uf::Serializer cardData = masterDataGet("Card", id);
		uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].as<std::string>());
		std::string name = charaData["filename"].as<std::string>();
		std::string url = "https://cdn..xyz//unity/Android/voice/voice_" + name + "_"+key+".ogg";
		if ( charaData["internal"].as<bool>() ) {
			url = "/smtsamo/voice/voice_" + name + "_" + key + ".ogg";
		}

		uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
		assetLoader.cache(url, "asset:Cache.Voice." + std::to_string(entity.getUid()));
	}
	void playSound( ext::GuiDialogue& entity, std::size_t uid, const std::string& key ) {
		auto& scene = uf::scene::getCurrentScene();
		uf::Serializer& pMetadata = entity.getComponent<uf::Serializer>();
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();

		uf::Entity*  = scene.findByUid(uid);
		if ( ! ) return;
		uf::Serializer& metadata = ->getComponent<uf::Serializer>();
		std::string id = metadata[""]["id"].as<std::string>();
		playSound( entity, id, key );
	}
	void playSound( ext::GuiDialogue& entity, const std::string& key ) {
		auto& scene = uf::scene::getCurrentScene();
		uf::Serializer& metadata = entity.getComponent<uf::Serializer>();
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();

		std::string url = uf::io::root+"/audio/ui/" + key + ".ogg";

		uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
		assetLoader.cache(url, "asset:Cache.SFX." + std::to_string(entity.getUid()));
	}
	void playMusic( ext::GuiDialogue& entity, const std::string& filename ) {
		auto& scene = uf::scene::getCurrentScene();
		uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();

		assetLoader.load(filename, "asset:Load." + std::to_string(scene.getUid()));
	}
	std::string getKeyFromIndex( uf::Serializer& object, uint64_t index ) {
		uint64_t i = 0;
		std::string key = "";
		for ( auto it = object.begin(); it != object.end(); ++it ) {
			if ( i++ == index ) key = it.key();
		}
		return key;
	}

	uf::Audio sfx;
	uf::Audio voice;

	struct DialogueStats {
		struct {
			int selection = 0;
			int selectionsMax = 3;
			std::vector<std::string> invalids;
			uf::Serializer current;
			std::string index = "0";
		} actions;
		std::string state = "open";
		bool initialized = false;
	};
	static DialogueStats stats;
}


void ext::GuiDialogue::initialize() {
	ext::Gui::initialize();

	uf::Scene& scene = uf::scene::getCurrentScene();
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	dialogueManager = (ext::DialogueManager*) this->getRootParent().findByName( "Dialogue Manager" );

	this->addHook( "asset:Cache.SFX." + std::to_string(this->getUid()), [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		
		std::string filename = json["filename"].as<std::string>();

		if ( uf::io::extension(filename) != "ogg" ) return "false";

		if ( filename == "" ) return "false";
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();
		uf::Audio& sfx = this->getComponent<uf::Audio>();
		if ( sfx.playing() ) sfx.stop();
		sfx = uf::Audio();
		sfx.load(filename);
		sfx.setVolume(masterdata["volumes"]["sfx"].as<float>());
		sfx.play();

		return "true";
	});

	this->addHook( "asset:Cache.Voice." + std::to_string(this->getUid()), [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		
		std::string filename = json["filename"].as<std::string>();

		if ( uf::io::extension(filename) != "ogg" ) return "false";

		if ( filename == "" ) return "false";
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();
		uf::Audio& voice = this->getComponent<uf::Audio>();
		if ( voice.playing() ) voice.stop();
		voice = uf::Audio();
		voice.load(filename);
		voice.setVolume(masterdata["volumes"]["voice"].as<float>());
		voice.play();

		return "true";
	});

	// lock control
	{
		uf::Serializer payload;
		payload["state"] = true;
		uf::hooks.call("window:Mouse.CursorVisibility", payload);
		uf::hooks.call("window:Mouse.Lock");
	}

	this->addHook("menu:Close.%UID%", [&]( const std::string& event ) -> std::string {		
		// kill
		this->getParent().removeChild(*this);
		this->destroy();
		delete this;
		return "true";
	});

	stats = DialogueStats();
}
void ext::GuiDialogue::tick() {
	if ( !dialogueManager ) return;

	uf::Scene& scene = uf::scene::getCurrentScene();
	uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();
	uf::Serializer& bMetadata = this->getComponent<uf::Serializer>();

	static float alpha = 0.0f;
	static uf::Timer<long long> timer(false);
	static float timeout = 1.0f;
	if ( !timer.running() ) timer.start();

	std::function<void(uf::Serializer&)> postParseResult = [&]( uf::Serializer& result ) {
		if ( result["end"].as<bool>() ) bMetadata["system"]["closing"] = true;
		if ( !ext::json::isNull( result["timeout"] ) ) timeout = result["timeout"].as<float>();
		else timeout = 1;
		if ( result["message"].as<std::string>() != "" ) {
			if ( dialogueMessage ) {
				dialogueMessage->destroy();
				this->removeChild(*dialogueMessage);
				delete dialogueMessage;	
			}
			dialogueMessage = new ext::Gui;
			this->addChild(*dialogueMessage);
			uf::Serializer& pMetadata = dialogueMessage->getComponent<uf::Serializer>();
			dialogueMessage->as<uf::Object>().load("./dialogue-text.json", true);
			pMetadata["text settings"]["string"] = result["message"].as<std::string>();
			dialogueMessage->initialize();
		}
		timer.reset();
	};

	std::function<void(uf::Serializer)> renderDialogueOptions = [&]( uf::Serializer actions ) {
		if ( dialogueOptions ) {
			this->removeChild(*dialogueOptions);
			dialogueOptions->destroy();
			delete dialogueOptions;
			dialogueOptions = NULL;
		}
		if ( ext::json::isNull( actions ) ) {
			stats.actions.selection = 0;
			return;
		}
		stats.actions.current = actions;
		std::string string = "";
		std::size_t counter = 0;
		std::size_t start = stats.actions.selection;
		auto keys = ext::json::keys( actions );
		for ( int i = start; i < keys.size(); ++i ) {
			std::string key = getKeyFromIndex( actions, i );
			std::string text = actions[key].as<std::string>();
			if ( std::find( stats.actions.invalids.begin(), stats.actions.invalids.end(), key ) != stats.actions.invalids.end() ) continue;
			if ( ++counter > stats.actions.selectionsMax ) break;
			if ( i != stats.actions.selection ) text = "%#FFFFFF%" + text;
			else text = "%#FF0000%" + text;
			string += "\n" + text;
		}
		if ( keys.size() > stats.actions.selectionsMax ) {
			for ( int i = 0; i < keys.size(); ++i ) {
				std::string key = getKeyFromIndex( actions, i );
				std::string text = actions[key].as<std::string>();
				if ( std::find( stats.actions.invalids.begin(), stats.actions.invalids.end(), key ) != stats.actions.invalids.end() ) continue;
				if ( ++counter > stats.actions.selectionsMax ) break;
				if ( i != stats.actions.selection ) text = "%#FFFFFF%" + text;
				else text = "%#FF0000%" + text;
				string += "\n" + text;
			}
		}
		if ( string != "" ) {
			string = string.substr(1);
			dialogueOptions = new ext::Gui;
			this->addChild(*dialogueOptions);
			uf::Serializer& pMetadata = dialogueOptions->getComponent<uf::Serializer>();
			dialogueOptions->as<uf::Object>().load("./dialogue-option.json", true);
			pMetadata["text settings"]["string"] = string;
			pod::Transform<>& pTransform = dialogueOptions->getComponent<pod::Transform<>>();
			dialogueOptions->initialize();
		}
	};

	if ( !stats.initialized ) {
		stats.initialized = true;
		this->addHook("menu:Dialogue.Turn.%UID%", [&](const std::string& event) -> std::string {
			uf::Serializer json = event;
			uf::Serializer payload;
			payload["action"] = "dialogue-select";
			payload["index"] = stats.actions.index;
			uf::Serializer result = dialogueManager->callHook("menu:Dialogue.Action.%UID%", payload)[0].as<uf::Serializer>();
			postParseResult(result);

			// renderDialogueOptions(std::string(""));
			renderDialogueOptions(result["actions"]);

			return "true";
		});
		this->queueHook("menu:Dialogue.Turn.%UID%");
	}

	{
		uf::Serializer& metadata = this->getComponent<uf::Serializer>();
		if ( metadata["system"]["closing"].as<bool>() ) {
			if ( alpha >= 1.0f ) {
				uf::Asset assetLoader;
				std::string canonical = assetLoader.load(uf::io::root+"/audio/ui/menu close.ogg");
				uf::Audio& sfx = assetLoader.get<uf::Audio>(canonical);
				sfx.setVolume(masterdata["volumes"]["sfx"].as<float>());
				sfx.play();
			}
			if ( alpha <= 0.0f ) {
				alpha = 0.0f;
				metadata["system"]["closing"] = false;
				metadata["system"]["closed"] = true;
			} else alpha -= uf::physics::time::delta;
			metadata["color"][3] = alpha;
		} else if ( metadata["system"]["closed"].as<bool>() ) {
			uf::Object& parent = this->getParent<uf::Object>();
			parent.getComponent<uf::Serializer>()["system"]["closed"] = true;
			this->destroy();
			parent.removeChild(*this);
		} else {
			if ( !metadata["initialized"].as<bool>() ) alpha = 0.0f;
			metadata["initialized"] = true;
			if ( alpha >= 1.0f ) alpha = 1.0f;
			else alpha += uf::physics::time::delta * 1.5f;
			metadata["color"][3] = alpha;
		}
	}

	ext::Gui::tick();

	if ( stats.state != "open" ) {
		if ( timer.elapsed().asDouble() >= timeout ) stats.state = "open";
		return;
	}

	struct {
		bool up = uf::Window::isKeyPressed("W");
		bool down = uf::Window::isKeyPressed("S");
		bool back = uf::Window::isKeyPressed("Escape");
		bool select = uf::Window::isKeyPressed("E");
	} keys;
	static bool lastPress = false;

	if ( dialogueOptions ) {
		bool delta = false;
		bool invalid = false;
		pod::Transform<>& transform = dialogueOptions->getComponent<pod::Transform<>>();
		uf::Serializer& metadata = dialogueOptions->getComponent<uf::Serializer>();
		metadata["color"][3] = alpha;
		for ( uf::Entity* pointer : dialogueOptions->getChildren()  ) {
			uf::Serializer& cMetadata = pointer->getComponent<uf::Serializer>();
			cMetadata["text settings"]["color"][3] = alpha;
		}
		static bool released = true;
		if ( keys.up ) { 
			if ( released ) {
				delta = true;
				released = false;
				do {
					--stats.actions.selection;
					if ( stats.actions.selection < 0 ) { stats.actions.selection = stats.actions.current.size() - 1; }
				} while ( std::find( stats.actions.invalids.begin(), stats.actions.invalids.end(), getKeyFromIndex(stats.actions.current, stats.actions.selection) ) != stats.actions.invalids.end() );
			}
		}
		else if ( keys.down ) {
			if ( released ) {
				delta = true;
				released = false;
				do {
					++stats.actions.selection;
					if ( stats.actions.selection >= stats.actions.current.size() ) { stats.actions.selection = 0; }
				} while ( std::find( stats.actions.invalids.begin(), stats.actions.invalids.end(), getKeyFromIndex(stats.actions.current, stats.actions.selection) ) != stats.actions.invalids.end() );
			}
		}
		else released = true;
		if ( stats.actions.selection < 0 ) { stats.actions.selection = stats.actions.current.size() - 1; }
		else if ( stats.actions.selection >= stats.actions.current.size() ) { stats.actions.selection = 0; }
		if ( stats.actions.current.size() == 0 ) released = true;
		if ( !released ) {
			if ( invalid ) {
				playSound(*this, "invalid select");
			} else if ( delta ) {
				playSound(*this, "menu flip");
				renderDialogueOptions(stats.actions.current);
			}
		} else if ( keys.back && timer.elapsed().asDouble() >= timeout ) {
			playSound(*this, "invalid select");
			timer.reset();
		} else if ( keys.select && timer.elapsed().asDouble() >= timeout ) {
			playSound(*this, "menu close");
			uf::Serializer payload;
			std::string action = getKeyFromIndex(stats.actions.current, stats.actions.selection);
			payload["action"] = "dialogue-select";
			payload["index"] = action;
			stats.actions.index = action;
			
			renderDialogueOptions(std::string(""));

			timer.reset();

			stats.state = "waiting";
			stats.actions.invalids.clear();
			this->queueHook("menu:Dialogue.Turn.%UID%", ext::json::null(), timeout);
		}
	}
}

void ext::GuiDialogue::render() {
	ext::Gui::render();
}

void ext::GuiDialogue::destroy() {
	ext::Gui::destroy();
}
#endif