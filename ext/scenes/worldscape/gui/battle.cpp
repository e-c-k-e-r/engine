#if 0
#include "battle.h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/window/window.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/string/ext.h>
// #include <uf/gl/glyph/glyph.h>
#include <uf/utils/math/physics.h>

#include "..//battle.h"
#include <uf/engine/asset/asset.h>

#include <uf/utils/memory/unordered_map.h>

//#include <uf/ext/renderer/renderer.h>

#include <uf/utils/http/http.h>
#include <uf/utils/audio/audio.h>
#include <sys/stat.h>
#include <fstream>

namespace {
	ext::Gui* circleIn;
	ext::Gui* circleOut;
	ext::Gui* battleMessage;
	ext::Gui* commandOptions;
	ext::Gui* battleOptions;
	ext::Gui* targetOptions;
	ext::Gui* optionsBackground;
	ext::Gui* turnCounters;
	ext::Gui* partyMemberCommandCircle;
	ext::Gui* critCutInBackground;
	ext::Gui* critCutIn;

	uf::stl::vector<ext::Gui*> partyMemberButtons(5);
//	uf::stl::vector<ext::Gui*> partyMemberIcons(5);
//	uf::stl::vector<ext::Gui*> partyMemberTexts(5);
	uf::stl::vector<ext::Gui*> particles;

	ext::HousamoBattle* battleManager;


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
	inline uf::stl::string colorString( const uf::stl::string& hex ) {
		return "%#" + hex + "%";
	}

	void playSound( ext::GuiBattle& entity, const uf::stl::string& id, const uf::stl::string& key ) {
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();

		uf::Serializer cardData = masterDataGet("Card", id);
		uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].as<uf::stl::string>());
		uf::stl::string name = charaData["filename"].as<uf::stl::string>();

		if ( name == "none" ) return;

		uf::stl::string url = "https://cdn..xyz//unity/Android/voice/voice_" + name + "_"+key+".ogg";
		if ( charaData["internal"].as<bool>() ) {
			url = "/smtsamo/voice/voice_" + name + "_" + key + ".ogg";
		}

		uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
		assetLoader.cache(url, "asset:Cache.Voice." + std::to_string(entity.getUid()));
	}
	void playSound( ext::GuiBattle& entity, std::size_t uid, const uf::stl::string& key ) {
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& pMetadata = entity.getComponent<uf::Serializer>();
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();

		uf::Entity*  = scene.findByUid(uid);
		if ( ! ) return;
		uf::Serializer& metadata = ->getComponent<uf::Serializer>();
		uf::stl::string id = metadata[""]["id"].as<uf::stl::string>();
		playSound( entity, id, key );
	}
	void playSound( ext::GuiBattle& entity, const uf::stl::string& key ) {
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& metadata = entity.getComponent<uf::Serializer>();
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();

		uf::stl::string url = uf::io::root+"/audio/ui/" + key + ".ogg";

		uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
		assetLoader.cache(url, "asset:Cache.SFX." + std::to_string(entity.getUid()));
	}
	void playMusic( ext::GuiBattle& entity, const uf::stl::string& filename ) {
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();

		assetLoader.load(filename, "asset:Load." + std::to_string(scene.getUid()));
	}
	uf::stl::string getKeyFromIndex( uf::Serializer& object, uint64_t index ) {
		uint64_t i = 0;
		uf::stl::string key = "";
		for ( auto it = object.begin(); it != object.end(); ++it ) {
			if ( i++ == index ) key = it.key();
		}
		return key;
	}

	uf::Audio sfx;
	uf::Audio voice;

	struct BattleStats {
		struct {
			int selection = 0;
			int selectionsMax = 3;
			uf::stl::vector<uf::stl::string> invalids;
		} skill;
		struct {
			int selection = 0;
			int selectionsMax = 3;
			uf::stl::vector<uf::stl::string> invalids;
			uf::Serializer current;
		} actions;
		struct {
			int selection = 0;
			int selectionsMax = 3;
			uf::stl::vector<uf::stl::string> invalids;
			uf::Serializer current;
		} targets;
		uf::Serializer currentMember;
		uf::Serializer previousMember;
		uf::stl::string state = "open";
		bool initialized = false;
	};
	static BattleStats stats;
}


void ext::GuiBattle::initialize() {
	ext::Gui::initialize();

	uf::Scene& scene = uf::scene::getCurrentScene();
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	battleManager = (ext::HousamoBattle*) this->getRootParent().findByName( "Battle Manager" );

	partyMemberButtons = uf::stl::vector<ext::Gui*>(5);

	this->addHook( "asset:Cache.SFX." + std::to_string(this->getUid()), [&](const uf::stl::string& event)->uf::stl::string{	
		uf::Serializer json = event;
		
		uf::stl::string filename = json["filename"].as<uf::stl::string>();

		if ( uf::io::extension(filename) != "ogg" ) return "false";

		if ( filename == "" ) return "false";
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();
		uf::Audio& sfx = this->getComponent<uf::SoundEmitter>().add(filename);
	/*
		uf::Audio& sfx = this->getComponent<uf::Audio>();
		if ( sfx.playing() ) sfx.stop();
		sfx = uf::Audio();
		sfx.load(filename);
	*/
		sfx.setVolume(masterdata["volumes"]["sfx"].as<float>());
		sfx.play();

		return "true";
	});

	this->addHook( "asset:Cache.Voice." + std::to_string(this->getUid()), [&](const uf::stl::string& event)->uf::stl::string{	
		uf::Serializer json = event;
		
		uf::stl::string filename = json["filename"].as<uf::stl::string>();

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
	{
		battleMessage = new ext::Gui;
		this->addChild(*battleMessage);
		battleMessage->as<uf::Object>().load("./battle-text.json", true);
		battleMessage->initialize();
	}
	/* Magic Circle Outter */ {
		circleOut = new ext::Gui;
		this->addChild(*circleOut);
		circleOut->as<uf::Object>().load("./circle-out.json", true);
		circleOut->initialize();
	}
	/* Magic Circle Inner */ {
		circleIn = new ext::Gui;
		this->addChild(*circleIn);
		circleIn->as<uf::Object>().load("./circle-in.json", true);
		circleIn->initialize();
	}
	/* Command Circle */ {
		partyMemberCommandCircle = new ext::Gui;
		this->addChild(*partyMemberCommandCircle);
		partyMemberCommandCircle->as<uf::Object>().load("./partyMemberCommandCircle.json", true);
		partyMemberCommandCircle->initialize();
	}
	/* Command Circle */ {
		optionsBackground = new ext::Gui;
		this->addChild(*optionsBackground);
		optionsBackground->as<uf::Object>().load("./optionsBackground.json", true);
		optionsBackground->initialize();
	}
	/* Turn Counter Container */ {
		turnCounters = new ext::Gui;
		this->addChild(*turnCounters);
		turnCounters->as<uf::Object>().load("./turnCounters.json", true);
		turnCounters->initialize();
	}	

	this->addHook("world:Battle.TurnStart.%UID%", [&](const uf::stl::string& event) -> uf::stl::string {
		uf::Serializer json = event;

		
		// turn start, play sfx
		if ( json["battle"]["turn"]["start of turn"].as<bool>() ) {
			playSound(*this, "turn");
		}
		
		return "true";
	});
	this->addHook("world:Battle.Damage.%UID%", [&](const uf::stl::string& event) -> uf::stl::string {
		uf::Serializer json = event;

		uf::stl::string uid = json["target"]["uid"].as<uf::stl::string>();
		uf::stl::string damage = json["target"]["damage"].as<uf::stl::string>();

		pod::Transform<> pTransform;
		pTransform.position = { 0, 0, 0 };

		for ( auto* pointerButton : partyMemberButtons ) {
			if ( !pointerButton ) continue;
			if ( pointerButton->getUid() == 0 ) continue;
			for ( auto* pointer : pointerButton->getChildren() ) {
				if ( !pointer ) continue;
				if ( pointer->getUid() == 0 ) continue;
				if ( pointer->getName() == "Menu: Party Member Icon" ) {
					uf::Serializer& pMetadata = pointer->getComponent<uf::Serializer>();
					if ( pMetadata[""]["uid"].as<uf::stl::string>() != uid ) continue;
					pTransform = pointer->getComponent<pod::Transform<>>();
					pTransform.position.y -= (0.1);
				}
			}
		}
		ext::Gui* particle = new ext::Gui;
		this->addChild(*particle);
		uf::Serializer& pMetadata = particle->getComponent<uf::Serializer>();
		particle->as<uf::Object>().load("./damageText.json", true);
		uf::stl::string color = json["color"].is<uf::stl::string>() ? json["color"].as<uf::stl::string>() : "FF0000";
		pMetadata["text settings"]["string"] = "%#"+color+"%" + damage;
		pod::Transform<>& transform = particle->getComponent<pod::Transform<>>();
		transform = pTransform;
		pMetadata["system"]["percent"] = 0;
		particle->initialize();
		particles.push_back(particle);

		return "true";
	});
	this->addHook("world:Battle.Message.%UID%", [&](const uf::stl::string& event) -> uf::stl::string {
		uf::Serializer json = event;
		uf::stl::string message = json["message"].as<uf::stl::string>();

		if ( battleMessage ) {
			battleMessage->destroy();
			this->removeChild(*battleMessage);
			delete battleMessage;	
		}
		battleMessage = new ext::Gui;
		this->addChild(*battleMessage);
		uf::Serializer& pMetadata = battleMessage->getComponent<uf::Serializer>();
		battleMessage->as<uf::Object>().load("./battle-text.json", true);
		pMetadata["text settings"]["string"] = message;
		battleMessage->initialize();

	//	std::cout << message << std::endl;

		return "true";
	});
	this->addHook("world:Battle.RemoveOnCrit.%UID%", [&](const uf::stl::string& event) -> uf::stl::string {
		if ( critCutInBackground && critCutInBackground->getUid() != 0 ) {
			critCutInBackground->destroy();
			this->removeChild(*critCutInBackground);
			delete critCutInBackground;	
			critCutInBackground = NULL;
		}
		if ( critCutIn && critCutIn->getUid() != 0 ) {
			critCutIn->destroy();
			this->removeChild(*critCutIn);
			delete critCutIn;	
			critCutIn = NULL;
		}
		return "true";
	});
	this->addHook("world:Battle.OnCrit.%UID%", [&](const uf::stl::string& event) -> uf::stl::string {
		uf::Serializer json = event;
		if ( critCutInBackground && critCutInBackground->getUid() != 0 ) {
			critCutInBackground->destroy();
			this->removeChild(*critCutInBackground);
			delete critCutInBackground;	
			critCutInBackground = NULL;
		}
		if ( critCutIn && critCutIn->getUid() != 0 ) {
			critCutIn->destroy();
			this->removeChild(*critCutIn);
			delete critCutIn;	
			critCutIn = NULL;
		}
		{
			critCutInBackground = new ext::Gui;
			this->addChild(*critCutInBackground);
			critCutInBackground->as<uf::Object>().load("./critCutInBackground.json", true);
			critCutInBackground->initialize();
		}
		{
			critCutIn = new ext::Gui;
			this->addChild(*critCutIn);
			uf::Serializer& pMetadata = critCutIn->getComponent<uf::Serializer>();

			uf::Serializer cardData = masterDataGet("Card", json["id"].as<uf::stl::string>());
			pMetadata["system"]["assets"][0] = "/smtsamo/ci/ci_"+ cardData["filename"].as<uf::stl::string>() +".png";
			if ( !uf::io::exists( pMetadata["system"]["assets"][0].as<uf::stl::string>() ) ) pMetadata["system"]["assets"][0] = ext::json::null();
			critCutIn->as<uf::Object>().load("./critCutIn.json", true);
			critCutIn->initialize();
		}

		playSound(*this, "crit");
		this->queueHook("world:Battle.RemoveOnCrit.%UID%", ext::json::null(), 1.0);

		return "true";
	});
	this->addHook("world:Battle.Update.%UID%", [&](const uf::stl::string& event) -> uf::stl::string {
		uf::Serializer json = event;
	
		{
			float turns = json["battle"]["turn"]["counter"].as<float>();
			if ( turnCounters && turnCounters->getUid() != 0 ) {
				this->removeChild(*turnCounters);
				turnCounters->destroy();
				delete turnCounters;
			}
			{
				turnCounters = new ext::Gui;
				this->addChild(*turnCounters);
				turnCounters->as<uf::Object>().load("./turnCounters.json", true);
				turnCounters->initialize();
			}
			// show full counters
			float i = 0;
			while ( turns > 0 ) {
				ext::Gui* counter = new ext::Gui;
				turnCounters->addChild(*counter);
				counter->as<uf::Object>().load("./turnCounter.json", true);
				counter->initialize();
				pod::Transform<>& pTransform = counter->getComponent<pod::Transform<>>();
				pTransform.position.x -= 0.142716f * (i++);

				uf::Serializer& pMetadata = counter->getComponent<uf::Serializer>();
				if ( json["battle"]["turn"]["phase"] == "enemy" ) {
					pMetadata["color"][0] = 1.0f;
					pMetadata["color"][1] = 0.4f;
					pMetadata["color"][2] = 0.4f;
					pMetadata["color"][3] = 1.0f;
				} else {
					pMetadata["color"][0] = 0.4f;
					pMetadata["color"][1] = 0.0f;
					pMetadata["color"][2] = 0.8f;
					pMetadata["color"][3] = 1.0f;
				}

			//	if ( turns < 1.0f && turns > 0 ) pMetadata["uv"][0] = 0.5f;
				if ( turns < 1.0f && turns > 0 ) {
					pMetadata["half counter"] = true;
					if ( json["battle"]["turn"]["phase"] == "enemy" ) {
						pMetadata["color"][0] = 1.0f;
						pMetadata["color"][1] = 0.7f;
						pMetadata["color"][2] = 0.7f;
						pMetadata["color"][3] = 1.0f;
					} else {
						pMetadata["color"][0] = 0.4f;
						pMetadata["color"][1] = 0.3f;
						pMetadata["color"][2] = 0.9f;
						pMetadata["color"][3] = 1.0f;
					}
				}
				turns = turns - 1.0f;
			}
		}
	
		
		int i = 0;

	//	for ( auto& member : json["battle"]["transients"] ) {
		ext::json::forEach(metadata["battle"]["transients"], [&](ext::json::Value& member, bool breaks){
			if ( ext::json::isNull( member["uid"] ) ) return;
			if ( member["type"] == "enemy" ) return;
			if ( member["hp"].as<float>() <= 0 ) return;
			if ( i >= 4 ) {
				breaks = true;
				return;
			}
			uf::Serializer cardData = masterDataGet("Card", member["id"].as<uf::stl::string>());
			uf::stl::string name = cardData["filename"].as<uf::stl::string>();
			if ( member["skin"].is<uf::stl::string>() ) name += "_" + member["skin"].as<uf::stl::string>();
			uf::stl::string filename = "https://cdn..xyz//unity/Android/icon/icon_" + name + ".png";
			if ( cardData["internal"].as<bool>() ) {
				filename = "/smtsamo/icon/icon_" + name + ".png";
			}

			ext::Gui* partyMemberButton = NULL;
			pod::Transform<> bTransform;

			// completely recreate if different transient
			if ( partyMemberButtons[i] != NULL ) {
				partyMemberButton = partyMemberButtons[i];
				uf::Serializer& pMetadata = partyMemberButton->getComponent<uf::Serializer>();
				if ( pMetadata[""]["id"] != member["id"] ) {
					this->removeChild(*partyMemberButton);
					partyMemberButton->destroy();
					delete partyMemberButton;
					partyMemberButton = NULL;
				} else {
					pod::Transform<>& transform = partyMemberButton->getComponent<pod::Transform<>>();
				 	bTransform = transform;
				}
			}

			// update
			if ( partyMemberButton ) {
				{
					ext::Gui* partyMemberText = (ext::Gui*) partyMemberButton->findByName("Menu: Party Member Text");
					if ( partyMemberText ) {
						partyMemberButton->removeChild(*partyMemberText);
						partyMemberText->destroy();
						delete partyMemberText;
						partyMemberText = NULL;
					}
					partyMemberText = new ext::Gui;
					partyMemberButton->addChild(*partyMemberText);
					uf::Serializer& pMetadata = partyMemberText->getComponent<uf::Serializer>();
					partyMemberText->as<uf::Object>().load("./partyMemberText.json", true);
					pMetadata["text settings"]["string"] = "" + colorString("00FF00") + "" + member["hp"].as<uf::stl::string>() + "\n" + colorString("0000FF") + "" + member["mp"].as<uf::stl::string>();
					pMetadata[""] = member;
					pod::Transform<>& transform = partyMemberText->getComponent<pod::Transform<>>();
					transform.position = bTransform.position;
					transform.position.x += (-0.781352 - -0.694634) + 0.1;
					partyMemberText->initialize();
				}
			
				ext::Gui* partyMemberHP = (ext::Gui*) partyMemberButton->findByName("Menu: Party Member HP Bar");
				if ( partyMemberHP ) {
					uf::Serializer& pMetadata = partyMemberHP->getComponent<uf::Serializer>();
					float percentage = 1.0f;
					percentage = member["hp"].as<float>() / member["max hp"].as<float>();
					// uv[0, 0] -> uv[09.43, 23.81]
					// uv[1, 1] -> uv[84.91, 68.25]
					pMetadata["uv"][2] = (percentage * 0.8491) + 0.0660;
				//	pMetadata["uv"][3] = (percentage * 0.6808) + (1 - 0.0952);
				}
				ext::Gui* partyMemberMP = (ext::Gui*) partyMemberButton->findByName("Menu: Party Member MP Bar");
				if ( partyMemberMP ) {
					uf::Serializer& pMetadata = partyMemberMP->getComponent<uf::Serializer>();
					float percentage = 1.0f;
					percentage = member["mp"].as<float>() / member["max mp"].as<float>();
					// uv[0, 0] -> uv[09.43, 23.81]
					// uv[1, 1] -> uv[84.91, 68.25]
					pMetadata["uv"][2] = (percentage * 0.8491) + 0.0943;
				//	pMetadata["uv"][3] = (percentage * 0.6825) + (1 - 0.2381);
				}
			// recreate
			} else {
				{
					partyMemberButton = new ext::Gui;
					this->addChild(*partyMemberButton);
					partyMemberButton->as<uf::Object>().load("./partyMemberButton.json", true);
					
					pod::Transform<>& transform = partyMemberButton->getComponent<pod::Transform<>>();
					transform.position.y += (i * 0.45);
				 	bTransform = transform;

					partyMemberButton->initialize();

					uf::Serializer& pMetadata = partyMemberButton->getComponent<uf::Serializer>();
					pMetadata[""] = member;

					partyMemberButtons[i] = partyMemberButton;
				}
				{
					ext::Gui* partyMemberIconShadow = NULL;
					partyMemberIconShadow = new ext::Gui;
					partyMemberButton->addChild(*partyMemberIconShadow);
					uf::Serializer& pMetadata = partyMemberIconShadow->getComponent<uf::Serializer>();
					pMetadata["system"]["assets"][0] = filename;
					partyMemberIconShadow->as<uf::Object>().load("./partyMemberIconShadow.json", true);
					pMetadata[""] = member;
					pod::Transform<>& transform = partyMemberIconShadow->getComponent<pod::Transform<>>();
					transform.position = bTransform.position;
					transform.position.x += (-0.781352 - -0.694634) + 0.01;
					transform.position.y -= 0.01;
					partyMemberIconShadow->initialize();
				}
				{
					ext::Gui* partyMemberIcon = NULL;
					partyMemberIcon = new ext::Gui;
					partyMemberButton->addChild(*partyMemberIcon);
					uf::Serializer& pMetadata = partyMemberIcon->getComponent<uf::Serializer>();
					pMetadata["system"]["assets"][0] = filename;
					partyMemberIcon->as<uf::Object>().load("./partyMemberIcon.json", true);
					pMetadata[""] = member;
					pod::Transform<>& transform = partyMemberIcon->getComponent<pod::Transform<>>();
					transform.position = bTransform.position;
					transform.position.x += (-0.781352 - -0.694634);
					partyMemberIcon->initialize();
				}
				{
					ext::Gui* partyMemberText = NULL;
					partyMemberText = new ext::Gui;
					partyMemberButton->addChild(*partyMemberText);
					uf::Serializer& pMetadata = partyMemberText->getComponent<uf::Serializer>();
					partyMemberText->as<uf::Object>().load("./partyMemberText.json", true);
					pMetadata["text settings"]["string"] = "" + colorString("FF0000") + "" + member["hp"].as<uf::stl::string>() + "\n" + colorString("0000FF") + "" + member["mp"].as<uf::stl::string>();
					pMetadata[""] = member;
					pod::Transform<>& transform = partyMemberText->getComponent<pod::Transform<>>();
					transform.position = bTransform.position;
					transform.position.x += (-0.781352 - -0.694634) + 0.1;
					partyMemberText->initialize();
				}
				{
					ext::Gui* partyMemberBar = new ext::Gui;
					partyMemberButton->addChild(*partyMemberBar);
					partyMemberBar->as<uf::Object>().load("./partyMemberBars.json", true);
					pod::Transform<>& transform = partyMemberBar->getComponent<pod::Transform<>>();
					transform.position = bTransform.position;
					transform.position.x = 0.742573;
					transform.position.y += 0.1;
					partyMemberBar->initialize();
				}
				{
					ext::Gui* partyMemberHP = new ext::Gui;
					partyMemberButton->addChild(*partyMemberHP);
					partyMemberHP->as<uf::Object>().load("./partyMemberHP.json", true);
					pod::Transform<>& transform = partyMemberHP->getComponent<pod::Transform<>>();
					transform.position = bTransform.position;
					transform.position.x = 0.742573;
					transform.position.y += 0.1;
					partyMemberHP->initialize();

					uf::Serializer& pMetadata = partyMemberHP->getComponent<uf::Serializer>();
					float percentage = 1.0f;
					percentage = member["hp"].as<float>() / member["max hp"].as<float>();
					// uv[0, 0] -> uv[09.43, 23.81]
					// uv[1, 1] -> uv[84.91, 68.25]
					pMetadata["uv"][2] = (percentage * 0.8491) + 0.0660;
				//	pMetadata["uv"][3] = (percentage * 0.6808) + (1 - 0.0952);
				}
				{
					ext::Gui* partyMemberMP = new ext::Gui;
					partyMemberButton->addChild(*partyMemberMP);
					partyMemberMP->as<uf::Object>().load("./partyMemberMP.json", true);
					pod::Transform<>& transform = partyMemberMP->getComponent<pod::Transform<>>();
					transform.position = bTransform.position;
					transform.position.x = 0.742573;
					transform.position.y += 0.1;
					partyMemberMP->initialize();

					uf::Serializer& pMetadata = partyMemberMP->getComponent<uf::Serializer>();
					float percentage = 1.0f;
					percentage = member["mp"].as<float>() / member["max mp"].as<float>();
					// uv[0, 0] -> uv[09.43, 23.81]
					// uv[1, 1] -> uv[84.91, 68.25]
					pMetadata["uv"][2] = (percentage * 0.8491) + 0.0943;
				//	pMetadata["uv"][3] = (percentage * 0.6825) + (1 - 0.2381);
				}
			}
			++i;
		});
		for ( ; i < 4; ++i ) {
			if ( partyMemberButtons[i] != NULL ) {
				ext::Gui*& partyMemberButton = partyMemberButtons[i];
				this->removeChild(*partyMemberButton);
				partyMemberButton->destroy();
				delete partyMemberButton;
				partyMemberButton = NULL;
			}
		}
		return "true";
	});

	this->addHook("menu:Close.%UID%", [&]( const uf::stl::string& event ) -> uf::stl::string {		
		// kill
		this->getParent().removeChild(*this);
		this->destroy();
		delete this;
		return "true";
	});

	stats = BattleStats();
}
void ext::GuiBattle::tick() {
	if ( !battleManager ) return;

	uf::Scene& scene = uf::scene::getCurrentScene();
	uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();
	uf::Serializer& bMetadata = this->getComponent<uf::Serializer>();

	static float alpha = 0.0f;
	static uf::Timer<long long> timer(false);
	static float timeout = 1.0f;
	if ( !timer.running() ) timer.start();
	static uf::stl::string queuedAction;

	std::function<void(uf::Serializer&)> postParseResult = [&]( uf::Serializer& result ) {
		if ( result["end"].as<bool>() ) bMetadata["system"]["closing"] = true;
		if ( !ext::json::isNull( result["timeout"] ) ) timeout = result["timeout"].as<float>();
		else timeout = 1;
		// modify timeout based on string length
		if ( result["message"].is<uf::stl::string>() && !ext::json::isNull( result["target"] ) ) {
			uint64_t size = result["message"].as<uf::stl::string>().size();
			float modified = size * 0.025f;
			if ( timeout < modified ) timeout = modified;
		}

		uf::Serializer payload;
		payload["message"] = result["message"];
		this->callHook("world:Battle.Message.%UID%", payload);

		timer.reset();
	};

	std::function<void(uf::Serializer)> renderSkillOptions = [&]( uf::Serializer member ) {
		if ( battleOptions ) {
			this->removeChild(*battleOptions);
			battleOptions->destroy();
			delete battleOptions;
			battleOptions = NULL;
		}
		if ( ext::json::isNull( member["uid"] ) ) {
			stats.previousMember = stats.currentMember;
			return;
		}
		stats.currentMember = member;
		uf::stl::string string = "";
		std::size_t counter = 0;
		std::size_t start = stats.skill.selection;
		stats.actions.invalids.clear();
		stats.currentMember["skills"] = ext::json::array();
		for ( int i = 0; i < member["skills"].size(); ++i ) {
			uf::stl::string id = member["skills"][i].as<uf::stl::string>();
			uf::Serializer skillData = masterDataGet("Skill", id);
			bool add = false;
			if ( skillData["type"].as<int>() > 0 && skillData["type"].as<int>() < 16 ) add = true;
			if ( add ) stats.currentMember["skills"].emplace_back(id);
		}

		uf::stl::string skillDescription = "";

		for ( int i = start; i < stats.currentMember["skills"].size(); ++i ) {
			uf::stl::string id = stats.currentMember["skills"][i].as<uf::stl::string>();
			uf::stl::string text = "";
			if ( std::find( stats.skill.invalids.begin(), stats.skill.invalids.end(), id ) != stats.skill.invalids.end() ) continue;
			if ( ++counter > stats.skill.selectionsMax ) break;
			uf::Serializer cardData = masterDataGet("Card", member["id"].as<uf::stl::string>());
			uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].as<uf::stl::string>());
			uf::Serializer skillData = masterDataGet("Skill", id);
			if ( id != "0" ) {
				text = skillData["name"].as<uf::stl::string>();
			} else {
				text = "攻撃";
			}
			if ( i != stats.skill.selection ) {
				text = "" + colorString("FFFFFF") + "" + text;
			}
			else {
				text = "" + colorString("FF0000") + "" + text;
				skillDescription = skillData["effect"].as<uf::stl::string>();
				if ( skillData["mp"].is<double>() ) {
					int64_t cost = skillData["mp"].as<int>();
					skillDescription = skillData["effect"].as<uf::stl::string>() + "\nCost: " + colorString("0000FF") + ""+std::to_string(cost)+" MP" + colorString("FFFFFF") + "";
				}
				if ( skillData["hp%"].is<double>() ) {
					int64_t cost = skillData["hp%"].as<int>();
					cost = member["max hp"].as<float>() * ( cost / 100.0f );
					skillDescription = skillData["effect"].as<uf::stl::string>() + "\nCost: " + colorString("00FF00") + ""+std::to_string(cost)+" HP" + colorString("FFFFFF") + "";
				}
			}
			string += "\n" + text;
		}
		if ( stats.currentMember["skills"].size() > stats.skill.selectionsMax ) {
			for ( int i = 0; i < stats.currentMember["skills"].size(); ++i ) {
				uf::stl::string id = stats.currentMember["skills"][i].as<uf::stl::string>();
				uf::stl::string text = "";
				if ( std::find( stats.skill.invalids.begin(), stats.skill.invalids.end(), id ) != stats.skill.invalids.end() ) continue;
				if ( ++counter > stats.skill.selectionsMax ) break;
				if ( id != "0" ) {
					uf::Serializer cardData = masterDataGet("Card", member["id"].as<uf::stl::string>());
					uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].as<uf::stl::string>());
					uf::Serializer skillData = masterDataGet("Skill", id);

					text = skillData["name"].as<uf::stl::string>();
				} else {
					uf::Serializer cardData = masterDataGet("Card", member["id"].as<uf::stl::string>());
					uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].as<uf::stl::string>());

					text = "攻撃";
				}
				if ( i != stats.skill.selection ) text = "" + colorString("FFFFFF") + "" + text;
				else text = "" + colorString("FF0000") + "" + text;
				string += "\n" + text;
			}
		}
		if ( string != "" ) {
			string = string.substr(1);
			battleOptions = new ext::Gui;
			this->addChild(*battleOptions);
			uf::Serializer& pMetadata = battleOptions->getComponent<uf::Serializer>();
			battleOptions->as<uf::Object>().load("./battle-option.json", true);
			pMetadata["text settings"]["string"] = string;
			pod::Transform<>& pTransform = battleOptions->getComponent<pod::Transform<>>();
			battleOptions->initialize();
		}
		if ( skillDescription != "" ) {
			uf::Serializer payload;
			payload["message"] = skillDescription;
			this->callHook("world:Battle.Message.%UID%", payload);
		}
	};
	std::function<void(uf::Serializer)> renderCommandOptions = [&]( uf::Serializer actions ) {
		if ( commandOptions ) {
			this->removeChild(*commandOptions);
			commandOptions->destroy();
			delete commandOptions;
			commandOptions = NULL;
		}
		if ( ext::json::isNull( actions ) ) {
			stats.actions.selection = 0;
			queuedAction = "";
			return;
		}
		stats.actions.current = actions;
		uf::stl::string string = "";
		std::size_t counter = 0;
		std::size_t start = stats.actions.selection;
		auto keys = ext::json::keys( actions );
		for ( int i = start; i < keys.size(); ++i ) {
			uf::stl::string key = getKeyFromIndex( actions, i );
			uf::stl::string text = actions[key].as<uf::stl::string>();
			if ( std::find( stats.actions.invalids.begin(), stats.actions.invalids.end(), key ) != stats.actions.invalids.end() ) continue;
			if ( ++counter > stats.actions.selectionsMax ) break;
			if ( i != stats.actions.selection ) text = "" + colorString("FFFFFF") + "" + text;
			else text = "" + colorString("FF0000") + "" + text;
			string += "\n" + text;
		}
		if ( keys.size() > stats.actions.selectionsMax ) {
			for ( int i = 0; i < keys.size(); ++i ) {
				uf::stl::string key = getKeyFromIndex( actions, i );
				uf::stl::string text = actions[key].as<uf::stl::string>();
				if ( std::find( stats.actions.invalids.begin(), stats.actions.invalids.end(), key ) != stats.actions.invalids.end() ) continue;
				if ( ++counter > stats.actions.selectionsMax ) break;
				if ( i != stats.actions.selection ) text = "" + colorString("FFFFFF") + "" + text;
				else text = "" + colorString("FF0000") + "" + text;
				string += "\n" + text;
			}
		}
		if ( string != "" ) {
			string = string.substr(1);
			commandOptions = new ext::Gui;
			this->addChild(*commandOptions);
			uf::Serializer& pMetadata = commandOptions->getComponent<uf::Serializer>();
			commandOptions->as<uf::Object>().load("./battle-option.json", true);
			pMetadata["text settings"]["string"] = string;
			pod::Transform<>& pTransform = commandOptions->getComponent<pod::Transform<>>();
			commandOptions->initialize();
		}
	};
	std::function<void(uf::Serializer)> renderTargetOptions = [&]( uf::Serializer actions ) {
		if ( targetOptions ) {
			this->removeChild(*targetOptions);
			targetOptions->destroy();
			delete targetOptions;
			targetOptions = NULL;
		}
		if ( ext::json::isNull( actions ) ) {
			stats.skill.selection = 0;
			stats.targets.selection = 0;
			return;
		}
		stats.targets.current = actions;
		uf::stl::string string = "";
		std::size_t counter = 0;
		std::size_t start = stats.targets.selection;
		stats.skill.invalids.clear();
		stats.actions.invalids.clear();
		uf::stl::string targetDescription = "";
		for ( int i = start; i < actions.size(); ++i ) {
			uf::Serializer cardData = masterDataGet("Card", actions[i]["id"].as<uf::stl::string>());
			uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].as<uf::stl::string>());
			uf::stl::string text = charaData["name"].as<uf::stl::string>();
			if ( std::find( stats.targets.invalids.begin(), stats.targets.invalids.end(), text ) != stats.targets.invalids.end() ) continue;
			if ( ++counter > stats.targets.selectionsMax ) break;
			if ( i != stats.targets.selection ) text = "" + colorString("FFFFFF") + "" + text;
			else text = "" + colorString("FF0000") + "" + text;
			string += "\n" + text;
		}
		if ( actions.size() > stats.targets.selectionsMax ) {
			for ( int i = 0; i < actions.size(); ++i ) {
				uf::Serializer cardData = masterDataGet("Card", actions[i]["id"].as<uf::stl::string>());
				uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].as<uf::stl::string>());
				uf::stl::string text = charaData["name"].as<uf::stl::string>();
				if ( std::find( stats.targets.invalids.begin(), stats.targets.invalids.end(), text ) != stats.targets.invalids.end() ) continue;
				if ( ++counter > stats.targets.selectionsMax ) break;
				if ( i != stats.targets.selection ) text = "" + colorString("FFFFFF") + "" + text;
				else text = "" + colorString("FF0000") + "" + text;
				string += "\n" + text;
			}
		}
		if ( string != "" ) {
			string = string.substr(1);
			targetOptions = new ext::Gui;
			this->addChild(*targetOptions);
			uf::Serializer& pMetadata = targetOptions->getComponent<uf::Serializer>();
			targetOptions->as<uf::Object>().load("./battle-option.json", true);
			pMetadata["text settings"]["string"] = string;
			pod::Transform<>& pTransform = targetOptions->getComponent<pod::Transform<>>();
			targetOptions->initialize();
		}
		if ( queuedAction == "analyze" ) {
			uf::Serializer payload;
			payload["uid"] = stats.targets.current[stats.targets.selection]["uid"].as<uf::stl::string>();
		//	if ( payload["uid"].as<uf::stl::string>() == "")
			payload["action"] = "analyze";
			uf::Serializer result = battleManager->callHook("world:Battle.Action.%UID%", payload)[0].as<uf::Serializer>();

			targetDescription = result["message"].as<uf::stl::string>();
		}
		if ( targetDescription != "" ) {
			uf::Serializer payload;
			payload["message"] = targetDescription;
			this->callHook("world:Battle.Message.%UID%", payload);
		}
	};


	if ( !stats.initialized ) {
		stats.initialized = true;
		uf::Serializer& metadata = this->getComponent<uf::Serializer>();
		this->addHook("world:Battle.Turn.%UID%", [&](const uf::stl::string& event) -> uf::stl::string {
			uf::Serializer json = event;
			uf::Serializer payload;
			payload["action"] = "turn-start";
			if ( !ext::json::isNull( stats.previousMember["uid"] ) ) payload["uid"] = stats.previousMember["uid"].as<size_t>() + 1;
			auto hookCall = battleManager->callHook("world:Battle.Action.%UID%", payload);
			if ( hookCall.empty() ) {
				metadata["system"]["closing"] = true;
				return "false";
			}
			uf::Serializer result = hookCall[0].as<uf::Serializer>();

			if ( result["end"].as<bool>() ) {
				if ( ext::json::isObject( metadata ) ) metadata["system"]["closing"] = true;
				return "false";
			}

			postParseResult(result);
			if ( ext::json::isNull( result["uid"] ) ) return "false";
			uint64_t uid = result["uid"].as<size_t>();

			renderCommandOptions(uf::stl::string(""));
			renderSkillOptions(uf::stl::string(""));
			renderTargetOptions(uf::stl::string(""));

			payload = uf::Serializer{};
			payload["uid"] = uid;
			if ( result[""]["type"] == "enemy" ) {
				payload["action"] = "enemy-attack";
			} else if ( result["skipped"].as<bool>() ) {
				payload["action"] = "forced-pass";
			} else {
				stats.currentMember = result[""];
				renderCommandOptions(result["actions"]);
				return "true";
			}
			result = battleManager->callHook("world:Battle.Action.%UID%", payload)[0].as<uf::Serializer>();
			postParseResult(result);
			stats.previousMember = uf::Serializer{};
			timer.reset();
			stats.state = "waiting";
			this->queueHook("world:Battle.Turn.%UID%", ext::json::null(), timeout);
			return "true";
		});
		this->queueHook("world:Battle.Turn.%UID%");
	}

	{
		uf::Serializer& metadata = this->getComponent<uf::Serializer>();
		if ( metadata["system"]["closing"].as<bool>() ) {
			if ( alpha >= 1.0f ) {
				uf::Asset assetLoader;
				uf::stl::string canonical = assetLoader.load(uf::io::root+"/audio/ui/menu close.ogg");
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

	if ( circleIn ) {
		pod::Transform<>& transform = circleIn->getComponent<pod::Transform<>>();
		static float time = 0.0f;
		float speed = 0.0125f;
		uf::Serializer& metadata = circleIn->getComponent<uf::Serializer>();
		if ( metadata["hovered"].as<bool>() ) speed = -0.25f;
		time += uf::physics::time::delta * speed;
		transform.orientation = uf::quaternion::axisAngle( { 0.0f, 0.0f, 1.0f }, time );

		static pod::Vector3f start = { transform.position.x, -2.0f, 0.0f };
		static pod::Vector3f end = transform.position;
		static float delta = 0.0f;
		if ( !metadata["initialized"].as<bool>() ) delta = 0.0f;
		metadata["initialized"] = true;
		if ( delta >= 1 ) delta = 1;
		else {
			delta += uf::physics::time::delta * 1.5f;
			transform.position = uf::vector::lerp( start, end, delta );
		}
		metadata["color"][3] = alpha * 0.1f;
	}
	if ( circleOut ) {
		pod::Transform<>& transform = circleOut->getComponent<pod::Transform<>>();
		uf::Serializer& metadata = circleOut->getComponent<uf::Serializer>();
		static float time = 0.0f;
		float speed = 0.0125f;
		if ( metadata["hovered"].as<bool>() ) speed = 0.25f;
		time += uf::physics::time::delta * speed;
		transform.orientation = uf::quaternion::axisAngle( { 0.0f, 0.0f, 1.0f }, time );

		static pod::Vector3f start = { transform.position.x, 2.0f, 0.0f };
		static pod::Vector3f end = transform.position;
		static float delta = 0.0f;
		if ( !metadata["initialized"].as<bool>() ) delta = 0.0f;
		metadata["initialized"] = true;
		if ( delta >= 1 ) delta = 1;
		else {
			delta += uf::physics::time::delta * 1.5f;
			transform.position = uf::vector::lerp( start, end, delta );
		}
		metadata["color"][3] = alpha * 0.1f;
	}
	if ( turnCounters ) { 
		static float time = 0.0f;
		time += uf::physics::time::delta;
		for ( uf::Entity* pointer : turnCounters->getChildren() ) {
			uf::Serializer& pMetadata = pointer->getComponent<uf::Serializer>();
			float coeff = 2.0f;
	//		if ( pMetadata["half counter"].as<bool>() ) coeff = 4.0f;
		//	pMetadata["color"][3] = sin(time * coeff) * 0.25f + 0.75f;
			pMetadata["color"][3] = std::min( 1.0f, (float) sin(time * coeff) * 0.5f + 1.0f );
		}
	}
	if ( optionsBackground ) {
		pod::Transform<>& transform = optionsBackground->getComponent<pod::Transform<>>();
		uf::Serializer& metadata = optionsBackground->getComponent<uf::Serializer>();

		static pod::Vector3f start = { transform.position.x, 2.0f, 0.0f };
		static pod::Vector3f end = transform.position;
		static float delta = 0.0f;
		if ( !metadata["initialized"].as<bool>() ) delta = 0.0f;
		metadata["initialized"] = true;
		if ( delta >= 1 ) delta = 1;
		else {
			delta += uf::physics::time::delta * 1.5f;
			transform.position = uf::vector::lerp( start, end, delta );
		}
		metadata["color"][3] = alpha;
		if ( !commandOptions && !battleOptions && !targetOptions ) {
			metadata["color"][3] = 0.1f;
		}
	}

	for ( auto* pointerButton : partyMemberButtons ) {
		if ( !pointerButton ) continue;
		if ( pointerButton->getUid() == 0 ) continue;
		for ( auto* pointer : pointerButton->getChildren() ) {
			if ( !pointer ) continue;
			if ( pointer->getUid() == 0 ) continue;
			if ( pointer->getName() == "Menu: Party Member Icon" ) {
				pod::Transform<>& transform = pointer->getComponent<pod::Transform<>>();
				uf::Serializer& metadata = pointer->getComponent<uf::Serializer>();
				metadata["color"][3] = alpha;
				if ( metadata["clicked"].as<bool>() && timer.elapsed().asDouble() >= timeout ) {
				}
				static float color = 0.0f;
			//	std::cout << pointer << ": " << metadata << ": " << stats.currentMember << std::endl;
				if ( metadata[""]["uid"] == stats.currentMember["uid"] ) {
					color += uf::physics::time::delta * 4.0f;
					float factor = (1 - (1.0f + cos(color)) / 8.0f);
					metadata["color"][0] = 1.0f * factor;
					metadata["color"][1] = 1.0f * factor;
					metadata["color"][2] = 0.6f * factor;

					if ( partyMemberCommandCircle && partyMemberCommandCircle->getUid() != 0 ) {
						uf::Serializer& metadata = partyMemberCommandCircle->getComponent<uf::Serializer>();
						pod::Transform<>& pTransform = partyMemberCommandCircle->getComponent<pod::Transform<>>();
						if ( pTransform.position.x == 0 && pTransform.position.y == 0 ) {
							pTransform.position.x = transform.position.x;
							pTransform.position.y = transform.position.y;
						}
						metadata["system"]["lerp"]["delta"] = 0;
						metadata["system"]["lerp"]["start"][0] = pTransform.position.x;
						metadata["system"]["lerp"]["start"][1] = pTransform.position.y;
						metadata["system"]["lerp"]["start"][2] = pTransform.position.z;
						metadata["system"]["lerp"]["end"][0] = transform.position.x;
						metadata["system"]["lerp"]["end"][1] = transform.position.y;
						metadata["system"]["lerp"]["end"][2] = transform.position.z;
					}
				} else {
					metadata["color"][0] = 1.0f;
					metadata["color"][2] = 1.0f;
					metadata["color"][2] = 1.0f;
				}
			} else if ( pointer->getName() == "Menu: Party Member Text" ) {
				pod::Transform<>& transform = pointer->getComponent<pod::Transform<>>();
				uf::Serializer& metadata = pointer->getComponent<uf::Serializer>();
				metadata["color"][3] = alpha;
				if ( metadata["clicked"].as<bool>() && timer.elapsed().asDouble() >= timeout ) {
				}
				static float color = 0.0f;
			//	std::cout << pointer << ": " << metadata << ": " << stats.currentMember << std::endl;
				if ( metadata[""]["uid"] == stats.currentMember["uid"] ) {
					color += uf::physics::time::delta * 4.0f;
					float factor = (1 - (1.0f + cos(color)) / 8.0f);
					metadata["color"][0] = 1.0f * factor;
					metadata["color"][1] = 1.0f * factor;
					metadata["color"][2] = 0.6f * factor;
				} else {
					metadata["color"][0] = 1.0f;
					metadata["color"][2] = 1.0f;
					metadata["color"][2] = 1.0f;
				}
			}
		}
	}
	/* particles */ for ( auto it = particles.begin(); it != particles.end(); ++it ) {
		ext::Gui* particle = *it;
		if ( !particle ) { particles.erase( it ); return; }
		if ( particle->getUid() == 0 ) { particles.erase( it ); return; }
		uf::Serializer& metadata = particle->getComponent<uf::Serializer>();
		pod::Transform<>& transform = particle->getComponent<pod::Transform<>>();
		// state 1
	//	std::cout << particle << ": " <<  metadata["system"]["percent"] << ": " << metadata["text settings"]["string"] <<  std::endl;
	//	std::cout << transform.position.x << ", " << transform.position.y << ", " << transform.position.z << std::endl;
	#if !UF_NO_EXCEPTIONS
		try
	#endif
		{
			if ( !ext::json::isObject( metadata["system"] ) ) {
				this->removeChild(*particle);
				particle->destroy();
				delete particle;
				*it = NULL;
				continue;
			}
			if ( metadata["system"]["percent"] < 0.25f ) {
				transform.position.y -= uf::physics::time::delta / 2.0f;
			} else if ( metadata["system"]["percent"] < 0.5f ) {
				transform.position.y += uf::physics::time::delta / 2.0f;
			} else if ( metadata["system"]["percent"] < 1.0f ) {
			} else {
				this->removeChild(*particle);
				particle->destroy();
				delete particle;
				*it = NULL;
				continue;
			}
			metadata["system"]["percent"] = metadata["system"]["percent"].as<float>() + uf::physics::time::delta;
		}
	#if !UF_NO_EXCEPTIONS
		catch ( ... ) {
			if ( particle ) {
				this->removeChild(*particle);
				particle->destroy();
				delete particle;
				*it = NULL;
			}
		}
	#endif
	}

	for ( auto* pointer : partyMemberButtons ) {
		if ( !pointer ) continue;
		if ( pointer->getUid() == 0 ) continue;
		pod::Transform<>& transform = pointer->getComponent<pod::Transform<>>();
		uf::Serializer& metadata = pointer->getComponent<uf::Serializer>();
		metadata["color"][3] = alpha;
	}

	if ( partyMemberCommandCircle && partyMemberCommandCircle->getUid() != 0 ) {
		pod::Transform<>& transform = partyMemberCommandCircle->getComponent<pod::Transform<>>();
		uf::Serializer& metadata = partyMemberCommandCircle->getComponent<uf::Serializer>();
		static float time = 0.0f;
		float speed = 0.5f;
		if ( metadata["hovered"].as<bool>() ) speed = 0.75f;
		time += uf::physics::time::delta * speed;
		transform.orientation = uf::quaternion::axisAngle( { 0.0f, 0.0f, 1.0f }, time );
		metadata["color"][3] = alpha;

		{
			pod::Vector3f start = {
				metadata["system"]["lerp"]["start"][0].as<float>(),
				metadata["system"]["lerp"]["start"][1].as<float>(),
				metadata["system"]["lerp"]["start"][2].as<float>(),
			};
			pod::Vector3f end = {
				metadata["system"]["lerp"]["end"][0].as<float>(),
				metadata["system"]["lerp"]["end"][1].as<float>(),
				metadata["system"]["lerp"]["end"][2].as<float>(),
			};
			float delta = metadata["system"]["lerp"]["delta"].as<float>();
			if ( delta >= 1 ) delta = 1;
			else {
				delta += uf::physics::time::delta * 8.0f;
				transform.position = uf::vector::lerp( start, end, delta );
			}
			metadata["system"]["lerp"]["delta"] = delta;
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
		bool select = uf::Window::isKeyPressed("Enter");
	} keys;

	if ( targetOptions ) {
		bool delta = false;
		bool invalid = false;
		pod::Transform<>& transform = targetOptions->getComponent<pod::Transform<>>();
		uf::Serializer& metadata = targetOptions->getComponent<uf::Serializer>();
		metadata["color"][3] = alpha;
		for ( uf::Entity* pointer : targetOptions->getChildren()  ) {
			uf::Serializer& cMetadata = pointer->getComponent<uf::Serializer>();
			cMetadata["text settings"]["color"][3] = alpha;
		}
		static bool released = true;
		if ( keys.up ) { 
			if ( released ) {
				delta = true;
				released = false;
				do {
					--stats.targets.selection;
					if ( stats.targets.selection < 0 ) { stats.targets.selection = stats.targets.current.size() - 1; }
				} while ( std::find( stats.targets.invalids.begin(), stats.targets.invalids.end(), stats.targets.current[stats.targets.selection]["id"].as<uf::stl::string>() ) != stats.targets.invalids.end() );
			}
		}
		else if ( keys.down ) {
			if ( released ) {
				delta = true;
				released = false;
				do {
					++stats.targets.selection;
					if ( stats.targets.selection >= stats.targets.current.size() ) { stats.targets.selection = 0; }
				} while ( std::find( stats.targets.invalids.begin(), stats.targets.invalids.end(), stats.targets.current[stats.targets.selection]["id"].as<uf::stl::string>() ) != stats.targets.invalids.end() );
			}
		}
		else released = true;
		if ( stats.targets.selection < 0 ) { stats.targets.selection = stats.targets.current.size() - 1; }
		else if ( stats.targets.selection >= stats.targets.current.size() ) { stats.targets.selection = 0; }
		if ( queuedAction == "analyze" ) keys.select = false;
		if ( !released ) {
			if ( invalid ) {
				playSound(*this, "invalid select");
			} else if ( delta ) {
				playSound(*this, "menu flip");
				renderTargetOptions(stats.targets.current);
			}
		} else if ( keys.back && timer.elapsed().asDouble() >= timeout ) {
			playSound(*this, "menu close");
			renderTargetOptions(uf::stl::string(""));
			if ( queuedAction == "analyze" ) {
				renderSkillOptions(uf::stl::string(""));
				renderCommandOptions(stats.actions.current);
			} else renderSkillOptions(stats.currentMember);
			timer.reset();
		} else if ( keys.select && timer.elapsed().asDouble() >= timeout ) {
			playSound(*this, "menu close");
			
			uf::Serializer payload;
			payload["action"] = "member-attack";
			payload["uid"] = stats.currentMember["uid"];
			payload["target"] = ext::json::array();
			payload["target"].emplace_back(stats.targets.current[stats.targets.selection]["uid"]);
			payload["skill"] = stats.currentMember["skills"][stats.skill.selection];

			uf::Serializer result = battleManager->callHook("world:Battle.Action.%UID%", payload)[0].as<uf::Serializer>();
			postParseResult(result);

			renderTargetOptions(uf::stl::string(""));
			renderCommandOptions(uf::stl::string(""));
			renderSkillOptions(uf::stl::string(""));

			timer.reset();
			stats.state = "waiting";
			if ( result["invalid"].as<bool>() ) {
			//	stats.actions.invalids.push_back(action);
				renderCommandOptions(stats.actions.current);
			//	this->queueHook("world:Battle.Turn.%UID%", ext::json::null(), timeout);
			} else {
				this->queueHook("world:Battle.Turn.%UID%", ext::json::null(), timeout);
			}
		}
	} else if ( battleOptions ) {
		bool delta = false;
		bool invalid = false;
		pod::Transform<>& transform = battleOptions->getComponent<pod::Transform<>>();
		uf::Serializer& metadata = battleOptions->getComponent<uf::Serializer>();
		metadata["color"][3] = alpha;
		for ( uf::Entity* pointer : battleOptions->getChildren()  ) {
			uf::Serializer& cMetadata = pointer->getComponent<uf::Serializer>();
			cMetadata["text settings"]["color"][3] = alpha;
		}
		static bool released = true;
		if ( keys.up ) { 
			if ( released ) {
				delta = true;
				released = false;
				do {
					--stats.skill.selection;
					if ( stats.skill.selection < 0 ) { stats.skill.selection = stats.currentMember["skills"].size()- 1; }
				} while ( std::find( stats.skill.invalids.begin(), stats.skill.invalids.end(), stats.currentMember["skills"][stats.skill.selection].as<uf::stl::string>() ) != stats.skill.invalids.end() );
			}
		}
		else if ( keys.down ) {
			if ( released ) {
				delta = true;
				released = false;
				do {
					++stats.skill.selection;
					if ( stats.skill.selection >= stats.currentMember["skills"].size()) { stats.skill.selection = 0; }
				} while ( std::find( stats.skill.invalids.begin(), stats.skill.invalids.end(), stats.currentMember["skills"][stats.skill.selection].as<uf::stl::string>() ) != stats.skill.invalids.end() );
			}
		}
		else released = true;
		if ( !released ) {
			if ( invalid ) {
				playSound(*this, "menu close");
			} else if ( delta ) {
				playSound(*this, "menu flip");
				renderSkillOptions(stats.currentMember);
			}
		} else if ( keys.back && timer.elapsed().asDouble() >= timeout ) {
			playSound(*this, "menu close");
			renderSkillOptions(uf::stl::string(""));
			renderCommandOptions(stats.actions.current);
			timer.reset();
		} else if ( keys.select && timer.elapsed().asDouble() >= timeout ) {
			playSound(*this, "menu close");
			bool allRanged = false;
			renderSkillOptions(uf::stl::string(""));
			renderCommandOptions(uf::stl::string(""));


			uf::Serializer payload;
			payload["action"] = "member-targets";
			payload["uid"] = stats.currentMember["uid"];
			payload["skill"] = stats.currentMember["skills"][stats.skill.selection];
			uf::Serializer result = battleManager->callHook("world:Battle.Action.%UID%", payload)[0].as<uf::Serializer>();

			uf::stl::string skillId = stats.currentMember["skills"][stats.skill.selection].as<uf::stl::string>();
			uf::Serializer skillData = masterDataGet("Skill", skillId);
			if ( result["range"].is<double>() ) skillData["range"] = result["range"];
			if ( skillData["range"].as<int>() > 1 ) {
				uf::Serializer payload;
				payload["action"] = "member-attack";
				payload["uid"] = stats.currentMember["uid"];
				payload["skill"] = stats.currentMember["skills"][stats.skill.selection];
				uf::Serializer result = battleManager->callHook("world:Battle.Action.%UID%", payload)[0].as<uf::Serializer>();
				postParseResult(result);
				timer.reset();
				stats.state = "waiting";
				if ( result["invalid"].as<bool>() ) {
				//	stats.actions.invalids.push_back(action);
					renderCommandOptions(stats.actions.current);
				//	this->queueHook("world:Battle.Turn.%UID%", ext::json::null(), timeout);
				} else {
					this->queueHook("world:Battle.Turn.%UID%", ext::json::null(), timeout);
				}
			} else {
				postParseResult(result);
				renderTargetOptions(result["targets"]);
			}
		}
	} else if ( commandOptions ) {
/*
		bool delta = false;
		bool invalid = false;
		pod::Transform<>& transform = commandOptions->getComponent<pod::Transform<>>();
		uf::Serializer& metadata = commandOptions->getComponent<uf::Serializer>();
		metadata["color"][3] = alpha;
		for ( uf::Entity* pointer : commandOptions->getChildren()  ) {
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
				} while ( std::find( stats.actions.invalids.begin(), stats.actions.invalids.end(), stats.actions.current[stats.actions.selection].as<uf::stl::string>() ) != stats.actions.invalids.end() );
			}
		}
		else if ( keys.down ) {
			if ( released ) {
				delta = true;
				released = false;
				do {
					++stats.actions.selection;
					if ( stats.actions.selection >= stats.actions.current.size() ) { stats.actions.selection = 0; }
				} while ( std::find( stats.actions.invalids.begin(), stats.actions.invalids.end(), stats.actions.current[stats.actions.selection].as<uf::stl::string>() ) != stats.actions.invalids.end() );
			}
		}
		else released = true;
		if ( stats.actions.selection < 0 ) { stats.actions.selection = stats.actions.current.size() - 1; }
		else if ( stats.actions.selection >= stats.actions.current.size() ) { stats.actions.selection = 0; }
		if ( !released ) {
			if ( invalid ) {
				playSound(*this, "invalid select");
			} else if ( delta ) {
				playSound(*this, "menu flip");
				renderCommandOptions(stats.actions.current);
			}
		} else if ( keys.back && timer.elapsed().asDouble() >= timeout ) {
			playSound(*this, "invalid select");
			timer.reset();
		} else if ( keys.select && timer.elapsed().asDouble() >= timeout ) {
			playSound(*this, "menu close");
			uf::Serializer payload;
			uf::stl::string action = stats.actions.current[stats.actions.selection].as<uf::stl::string>();
			payload["action"] = action;
			if ( payload["action"] == "member-attack" ) {
				payload["uid"] = stats.currentMember["uid"];
				payload["skill"] = stats.currentMember["skills"][stats.skill.selection];
			}
			uf::Serializer result = battleManager->callHook("world:Battle.Action.%UID%", payload)[0].as<uf::Serializer>();

			postParseResult(result);
			timer.reset();

			renderCommandOptions(uf::stl::string(""));

			stats.state = "waiting";
			if ( result["invalid"].as<bool>() ) {
				stats.actions.invalids.push_back(action);
				renderCommandOptions(stats.actions.current);
			//	this->queueHook("world:Battle.Turn.%UID%", ext::json::null(), timeout);
			} else if ( payload["action"] == "member-attack" ) {
				renderSkillOptions(uf::stl::string(""));
				stats.state = "waiting";
				this->queueHook("world:Battle.Turn.%UID%", ext::json::null(), timeout);
			} else if ( payload["action"] == "member-skill" ) {
				renderSkillOptions(stats.currentMember);
			} else if ( payload["action"] == "pass" ) {
				stats.previousMember = stats.currentMember;
				this->queueHook("world:Battle.Turn.%UID%", ext::json::null(), timeout);
			}
		}
	}
*/
		bool delta = false;
		bool invalid = false;
		pod::Transform<>& transform = commandOptions->getComponent<pod::Transform<>>();
		uf::Serializer& metadata = commandOptions->getComponent<uf::Serializer>();
		metadata["color"][3] = alpha;
		for ( uf::Entity* pointer : commandOptions->getChildren()  ) {
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
				renderCommandOptions(stats.actions.current);
			}
		} else if ( keys.back && timer.elapsed().asDouble() >= timeout ) {
			playSound(*this, "invalid select");
			timer.reset();
		} else if ( keys.select && timer.elapsed().asDouble() >= timeout ) {
			playSound(*this, "menu close");
			uf::Serializer payload;
			uf::stl::string action = getKeyFromIndex(stats.actions.current, stats.actions.selection);
			action = uf::io::extension(action);
			payload["action"] = action;
			payload["uid"] = stats.currentMember["uid"];
		
			if ( payload["action"] == "member-attack" ) {
			//	payload["skill"] = stats.currentMember["skills"][0];
				payload["skill"] = 0;
			}
		
			if ( payload["action"] == "analyze" ) {
				payload["action"] = "member-targets";
			}
			uf::Serializer result = battleManager->callHook("world:Battle.Action.%UID%", payload)[0].as<uf::Serializer>();

		//	std::cout << payload << ": " << result << std::endl;

			postParseResult(result);
			timer.reset();

			renderCommandOptions(uf::stl::string(""));
			queuedAction = action;

			stats.state = "waiting";
			if ( result["invalid"].as<bool>() ) {
				stats.actions.invalids.push_back(action);
				renderCommandOptions(stats.actions.current);
			} else if ( payload["action"] == "member-attack" ) {
				renderSkillOptions(uf::stl::string(""));
				stats.state = "waiting";
				this->queueHook("world:Battle.Turn.%UID%", ext::json::null(), timeout);
			} else if ( payload["action"] == "member-skill" ) {
				renderSkillOptions(stats.currentMember);
			} else if ( payload["action"] == "member-targets" ) {
				renderTargetOptions(result["targets"]);
			} else if ( payload["action"] == "pass" ) {
				stats.previousMember = stats.currentMember;
				this->queueHook("world:Battle.Turn.%UID%", ext::json::null(), timeout);
			} else {
				renderCommandOptions(stats.actions.current);
			}
		}
	}
}

void ext::GuiBattle::render() {
	ext::Gui::render();
}

void ext::GuiBattle::destroy() {
	ext::Gui::destroy();
}
#endif