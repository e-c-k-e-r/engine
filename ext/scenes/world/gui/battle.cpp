#include "battle.h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/window/window.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/utils/string/ext.h>
// #include <uf/gl/glyph/glyph.h>
#include "../world.h"
#include "..//battle.h"
#include <uf/engine/asset/asset.h>

#include <unordered_map>

#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/graphics/gui.h>

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

	std::vector<ext::Gui*> partyMemberButtons(5);
//	std::vector<ext::Gui*> partyMemberIcons(5);
//	std::vector<ext::Gui*> partyMemberTexts(5);
	std::vector<ext::Gui*> particles;

	ext::HousamoBattle* battleManager;

	ext::World* world;

	uf::Serializer masterTableGet( const std::string& table ) {
		if ( !world ) world = (ext::World*) uf::Entity::globalFindByName("World");
		if ( !world ) return std::string();
		uf::Serializer& mastertable = world->getComponent<uf::Serializer>();
		return mastertable["system"]["mastertable"][table];
	}
	uf::Serializer masterDataGet( const std::string& table, const std::string& key ) {
		if ( !world ) world = (ext::World*) uf::Entity::globalFindByName("World");
		if ( !world ) return std::string();
		uf::Serializer& mastertable = world->getComponent<uf::Serializer>();
		return mastertable["system"]["mastertable"][table][key];
	}
	inline int64_t parseInt( const std::string& str ) {
		return atoi(str.c_str());
	}
	inline std::string colorString( const std::string& hex ) {
		return "%#" + hex + "%";
	}

	void playSound( ext::GuiBattle& entity, const std::string& id, const std::string& key ) {
		if ( !world ) world = (ext::World*) uf::Entity::globalFindByName("World");
		if ( !world ) return;
		uf::Serializer& masterdata = world->getComponent<uf::Serializer>();

		uf::Serializer cardData = masterDataGet("Card", id);
		uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].asString());
		std::string name = charaData["filename"].asString();

		if ( name == "none" ) return;

		std::string url = "https://cdn..xyz//unity/Android/voice/voice_" + name + "_"+key+".ogg";
		if ( charaData["internal"].asBool() ) {
			url = "/smtsamo/voice/voice_" + name + "_" + key + ".ogg";
		}

		uf::Asset& assetLoader = world->getComponent<uf::Asset>();
		assetLoader.cache(url, "asset:Cache.Voice." + std::to_string(entity.getUid()));
	}
	void playSound( ext::GuiBattle& entity, std::size_t uid, const std::string& key ) {
		if ( !world ) world = (ext::World*) uf::Entity::globalFindByName("World");
		if ( !world ) return;
		uf::Serializer& pMetadata = entity.getComponent<uf::Serializer>();
		uf::Serializer& masterdata = world->getComponent<uf::Serializer>();

		uf::Entity*  = world->findByUid(uid);
		if ( ! ) return;
		uf::Serializer& metadata = ->getComponent<uf::Serializer>();
		std::string id = metadata[""]["id"].asString();
		playSound( entity, id, key );
	}
	void playSound( ext::GuiBattle& entity, const std::string& key ) {
		if ( !world ) world = (ext::World*) uf::Entity::globalFindByName("World");
		if ( !world ) return;
		uf::Serializer& metadata = entity.getComponent<uf::Serializer>();
		uf::Serializer& masterdata = world->getComponent<uf::Serializer>();

		std::string url = "./data/audio/ui/" + key + ".ogg";

		uf::Asset& assetLoader = world->getComponent<uf::Asset>();
		assetLoader.cache(url, "asset:Cache.SFX." + std::to_string(entity.getUid()));
	}
	void playMusic( ext::GuiBattle& entity, const std::string& filename ) {
		if ( !world ) world = (ext::World*) uf::Entity::globalFindByName("World");
		if ( !world ) return;
		uf::Asset& assetLoader = world->getComponent<uf::Asset>();
		uf::Serializer& masterdata = world->getComponent<uf::Serializer>();

		assetLoader.load(filename, "asset:Load." + std::to_string(world->getUid()));
	}
	std::string getKeyFromIndex( uf::Serializer& object, uint64_t index ) {
		uint64_t i = 0;
		std::string key = "";
		for ( auto it = object.begin(); it != object.end(); ++it ) {
			if ( i++ == index ) key = it.key().asString();
		}
		return key;
	}

	uf::Audio sfx;
	uf::Audio voice;

	struct BattleStats {
		struct {
			int selection = 0;
			int selectionsMax = 3;
			std::vector<std::string> invalids;
		} skill;
		struct {
			int selection = 0;
			int selectionsMax = 3;
			std::vector<std::string> invalids;
			uf::Serializer current;
		} actions;
		struct {
			int selection = 0;
			int selectionsMax = 3;
			std::vector<std::string> invalids;
			uf::Serializer current;
		} targets;
		uf::Serializer currentMember;
		uf::Serializer previousMember;
		std::string state = "open";
		bool initialized = false;
	};
	static BattleStats stats;
}


void ext::GuiBattle::initialize() {
	ext::Gui::initialize();

	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	battleManager = (ext::HousamoBattle*) this->getRootParent().findByName( "Battle Manager" );

	partyMemberButtons = std::vector<ext::Gui*>(5);

	this->addHook( "asset:Cache.SFX." + std::to_string(this->getUid()), [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		
		std::string filename = json["filename"].asString();

		if ( uf::string::extension(filename) != "ogg" ) return "false";

		if ( filename == "" ) return "false";
		uf::Serializer& masterdata = world->getComponent<uf::Serializer>();
		uf::Audio& sfx = this->getComponent<uf::SoundEmitter>().add(filename);
	/*
		uf::Audio& sfx = this->getComponent<uf::Audio>();
		if ( sfx.playing() ) sfx.stop();
		sfx = uf::Audio();
		sfx.load(filename);
	*/
		sfx.setVolume(masterdata["volumes"]["sfx"].asFloat());
		sfx.play();

		return "true";
	});

	this->addHook( "asset:Cache.Voice." + std::to_string(this->getUid()), [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		
		std::string filename = json["filename"].asString();

		if ( uf::string::extension(filename) != "ogg" ) return "false";

		if ( filename == "" ) return "false";
		uf::Serializer& masterdata = world->getComponent<uf::Serializer>();
		uf::Audio& voice = this->getComponent<uf::Audio>();
		if ( voice.playing() ) voice.stop();
		voice = uf::Audio();
		voice.load(filename);
		voice.setVolume(masterdata["volumes"]["voice"].asFloat());
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
		battleMessage->load("./battle-text.json", true);
		battleMessage->initialize();
	}
	/* Magic Circle Outter */ {
		circleOut = new ext::Gui;
		this->addChild(*circleOut);
		circleOut->load("./circle-out.json", true);
		circleOut->initialize();
	}
	/* Magic Circle Inner */ {
		circleIn = new ext::Gui;
		this->addChild(*circleIn);
		circleIn->load("./circle-in.json", true);
		circleIn->initialize();
	}
	/* Command Circle */ {
		partyMemberCommandCircle = new ext::Gui;
		this->addChild(*partyMemberCommandCircle);
		partyMemberCommandCircle->load("./partyMemberCommandCircle.json", true);
		partyMemberCommandCircle->initialize();
	}
	/* Command Circle */ {
		optionsBackground = new ext::Gui;
		this->addChild(*optionsBackground);
		optionsBackground->load("./optionsBackground.json", true);
		optionsBackground->initialize();
	}
	/* Turn Counter Container */ {
		turnCounters = new ext::Gui;
		this->addChild(*turnCounters);
		turnCounters->load("./turnCounters.json", true);
		turnCounters->initialize();
	}	

	this->addHook("world:Battle.TurnStart.%UID%", [&](const std::string& event) -> std::string {
		uf::Serializer json = event;

		
		// turn start, play sfx
		if ( json["battle"]["turn"]["start of turn"].asBool() ) {
			playSound(*this, "turn");
		}
		
		return "true";
	});
	this->addHook("world:Battle.Damage.%UID%", [&](const std::string& event) -> std::string {
		uf::Serializer json = event;

		std::string uid = json["target"]["uid"].asString();
		std::string damage = json["target"]["damage"].asString();

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
					if ( pMetadata[""]["uid"].asString() != uid ) continue;
					pTransform = pointer->getComponent<pod::Transform<>>();
					pTransform.position.y -= (0.1);
				}
			}
		}
		ext::Gui* particle = new ext::Gui;
		this->addChild(*particle);
		uf::Serializer& pMetadata = particle->getComponent<uf::Serializer>();
		particle->load("./damageText.json", true);
		std::string color = json["color"].isString() ? json["color"].asString() : "FF0000";
		pMetadata["text settings"]["string"] = "%#"+color+"%" + damage;
		pod::Transform<>& transform = particle->getComponent<pod::Transform<>>();
		transform = pTransform;
		pMetadata["system"]["percent"] = 0;
		particle->initialize();
		particles.push_back(particle);

		return "true";
	});
	this->addHook("world:Battle.Message.%UID%", [&](const std::string& event) -> std::string {
		uf::Serializer json = event;
		std::string message = json["message"].asString();

		if ( battleMessage ) {
			battleMessage->destroy();
			this->removeChild(*battleMessage);
			delete battleMessage;	
		}
		battleMessage = new ext::Gui;
		this->addChild(*battleMessage);
		uf::Serializer& pMetadata = battleMessage->getComponent<uf::Serializer>();
		battleMessage->load("./battle-text.json", true);
		pMetadata["text settings"]["string"] = message;
		battleMessage->initialize();

	//	std::cout << message << std::endl;

		return "true";
	});
	this->addHook("world:Battle.RemoveOnCrit.%UID%", [&](const std::string& event) -> std::string {
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
	this->addHook("world:Battle.OnCrit.%UID%", [&](const std::string& event) -> std::string {
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
			critCutInBackground->load("./critCutInBackground.json", true);
			critCutInBackground->initialize();
		}
		{
			critCutIn = new ext::Gui;
			this->addChild(*critCutIn);
			uf::Serializer& pMetadata = critCutIn->getComponent<uf::Serializer>();

			uf::Serializer cardData = masterDataGet("Card", json["id"].asString());
			pMetadata["system"]["assets"][0] = "/smtsamo/ci/ci_"+ cardData["filename"].asString() +".png";
			if ( !uf::string::exists( pMetadata["system"]["assets"][0].asString() ) ) pMetadata["system"]["assets"][0] = Json::nullValue;
			critCutIn->load("./critCutIn.json", true);
			critCutIn->initialize();
		}

		playSound(*this, "crit");
		this->queueHook("world:Battle.RemoveOnCrit.%UID%", "", 1.0);

		return "true";
	});
	this->addHook("world:Battle.Update.%UID%", [&](const std::string& event) -> std::string {
		uf::Serializer json = event;
	
		{
			float turns = json["battle"]["turn"]["counter"].asFloat();
			if ( turnCounters && turnCounters->getUid() != 0 ) {
				this->removeChild(*turnCounters);
				turnCounters->destroy();
				delete turnCounters;
			}
			{
				turnCounters = new ext::Gui;
				this->addChild(*turnCounters);
				turnCounters->load("./turnCounters.json", true);
				turnCounters->initialize();
			}
			// show full counters
			float i = 0;
			while ( turns > 0 ) {
				ext::Gui* counter = new ext::Gui;
				turnCounters->addChild(*counter);
				counter->load("./turnCounter.json", true);
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

		for ( auto& member : json["battle"]["transients"] ) {
			if ( member["uid"].isNull() ) continue;
			if ( member["type"] == "enemy" ) continue;
			if ( member["hp"].asFloat() <= 0 ) continue;
			if ( i >= 4 ) break;
			uf::Serializer cardData = masterDataGet("Card", member["id"].asString());
			std::string name = cardData["filename"].asString();
			if ( member["skin"].isString() ) name += "_" + member["skin"].asString();
			std::string filename = "https://cdn..xyz//unity/Android/icon/icon_" + name + ".png";
			if ( cardData["internal"].asBool() ) {
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
					partyMemberText->load("./partyMemberText.json", true);
					pMetadata["text settings"]["string"] = "" + colorString("00FF00") + "" + member["hp"].asString() + "\n" + colorString("0000FF") + "" + member["mp"].asString();
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
					percentage = member["hp"].asFloat() / member["max hp"].asFloat();
					// uv[0, 0] -> uv[09.43, 23.81]
					// uv[1, 1] -> uv[84.91, 68.25]
					pMetadata["uv"][2] = (percentage * 0.8491) + 0.0660;
				//	pMetadata["uv"][3] = (percentage * 0.6808) + (1 - 0.0952);
				}
				ext::Gui* partyMemberMP = (ext::Gui*) partyMemberButton->findByName("Menu: Party Member MP Bar");
				if ( partyMemberMP ) {
					uf::Serializer& pMetadata = partyMemberMP->getComponent<uf::Serializer>();
					float percentage = 1.0f;
					percentage = member["mp"].asFloat() / member["max mp"].asFloat();
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
					partyMemberButton->load("./partyMemberButton.json", true);
					
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
					partyMemberIconShadow->load("./partyMemberIconShadow.json", true);
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
					partyMemberIcon->load("./partyMemberIcon.json", true);
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
					partyMemberText->load("./partyMemberText.json", true);
					pMetadata["text settings"]["string"] = "" + colorString("FF0000") + "" + member["hp"].asString() + "\n" + colorString("0000FF") + "" + member["mp"].asString();
					pMetadata[""] = member;
					pod::Transform<>& transform = partyMemberText->getComponent<pod::Transform<>>();
					transform.position = bTransform.position;
					transform.position.x += (-0.781352 - -0.694634) + 0.1;
					partyMemberText->initialize();
				}
				{
					ext::Gui* partyMemberBar = new ext::Gui;
					partyMemberButton->addChild(*partyMemberBar);
					partyMemberBar->load("./partyMemberBars.json", true);
					pod::Transform<>& transform = partyMemberBar->getComponent<pod::Transform<>>();
					transform.position = bTransform.position;
					transform.position.x = 0.742573;
					transform.position.y += 0.1;
					partyMemberBar->initialize();
				}
				{
					ext::Gui* partyMemberHP = new ext::Gui;
					partyMemberButton->addChild(*partyMemberHP);
					partyMemberHP->load("./partyMemberHP.json", true);
					pod::Transform<>& transform = partyMemberHP->getComponent<pod::Transform<>>();
					transform.position = bTransform.position;
					transform.position.x = 0.742573;
					transform.position.y += 0.1;
					partyMemberHP->initialize();

					uf::Serializer& pMetadata = partyMemberHP->getComponent<uf::Serializer>();
					float percentage = 1.0f;
					percentage = member["hp"].asFloat() / member["max hp"].asFloat();
					// uv[0, 0] -> uv[09.43, 23.81]
					// uv[1, 1] -> uv[84.91, 68.25]
					pMetadata["uv"][2] = (percentage * 0.8491) + 0.0660;
				//	pMetadata["uv"][3] = (percentage * 0.6808) + (1 - 0.0952);
				}
				{
					ext::Gui* partyMemberMP = new ext::Gui;
					partyMemberButton->addChild(*partyMemberMP);
					partyMemberMP->load("./partyMemberMP.json", true);
					pod::Transform<>& transform = partyMemberMP->getComponent<pod::Transform<>>();
					transform.position = bTransform.position;
					transform.position.x = 0.742573;
					transform.position.y += 0.1;
					partyMemberMP->initialize();

					uf::Serializer& pMetadata = partyMemberMP->getComponent<uf::Serializer>();
					float percentage = 1.0f;
					percentage = member["mp"].asFloat() / member["max mp"].asFloat();
					// uv[0, 0] -> uv[09.43, 23.81]
					// uv[1, 1] -> uv[84.91, 68.25]
					pMetadata["uv"][2] = (percentage * 0.8491) + 0.0943;
				//	pMetadata["uv"][3] = (percentage * 0.6825) + (1 - 0.2381);
				}
			}
			++i;
		};
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

	this->addHook("menu:Close.%UID%", [&]( const std::string& event ) -> std::string {		
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

	ext::World& world = this->getRootParent<ext::World>();
	uf::Serializer& masterdata = world.getComponent<uf::Serializer>();
	uf::Serializer& bMetadata = this->getComponent<uf::Serializer>();

	static float alpha = 0.0f;
	static uf::Timer<long long> timer(false);
	static float timeout = 1.0f;
	if ( !timer.running() ) timer.start();
	static std::string queuedAction;

	std::function<void(uf::Serializer&)> postParseResult = [&]( uf::Serializer& result ) {
		if ( result["end"].asBool() ) bMetadata["system"]["closing"] = true;
		if ( !result["timeout"].isNull() ) timeout = result["timeout"].asFloat();
		else timeout = 1;
		// modify timeout based on string length
		if ( result["message"].isString() && !result["target"].isNull() ) {
			uint64_t size = result["message"].asString().size();
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
		if ( member["uid"].isNull() ) {
			stats.previousMember = stats.currentMember;
			return;
		}
		stats.currentMember = member;
		std::string string = "";
		std::size_t counter = 0;
		std::size_t start = stats.skill.selection;
		stats.actions.invalids.clear();
		stats.currentMember["skills"] = Json::arrayValue;
		for ( int i = 0; i < member["skills"].size(); ++i ) {
			std::string id = member["skills"][i].asString();
			uf::Serializer skillData = masterDataGet("Skill", id);
			bool add = false;
			if ( skillData["type"].asInt() > 0 && skillData["type"].asInt() < 16 ) add = true;
			if ( add ) stats.currentMember["skills"].append(id);
		}

		std::string skillDescription = "";

		for ( int i = start; i < stats.currentMember["skills"].size(); ++i ) {
			std::string id = stats.currentMember["skills"][i].asString();
			std::string text = "";
			if ( std::find( stats.skill.invalids.begin(), stats.skill.invalids.end(), id ) != stats.skill.invalids.end() ) continue;
			if ( ++counter > stats.skill.selectionsMax ) break;
			uf::Serializer cardData = masterDataGet("Card", member["id"].asString());
			uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].asString());
			uf::Serializer skillData = masterDataGet("Skill", id);
			if ( id != "0" ) {
				text = skillData["name"].asString();
			} else {
				text = "攻撃";
			}
			if ( i != stats.skill.selection ) {
				text = "" + colorString("FFFFFF") + "" + text;
			}
			else {
				text = "" + colorString("FF0000") + "" + text;
				skillDescription = skillData["effect"].asString();
				if ( skillData["mp"].isNumeric() ) {
					int64_t cost = skillData["mp"].asInt64();
					skillDescription = skillData["effect"].asString() + "\nCost: " + colorString("0000FF") + ""+std::to_string(cost)+" MP" + colorString("FFFFFF") + "";
				}
				if ( skillData["hp%"].isNumeric() ) {
					int64_t cost = skillData["hp%"].asInt64();
					cost = member["max hp"].asFloat() * ( cost / 100.0f );
					skillDescription = skillData["effect"].asString() + "\nCost: " + colorString("00FF00") + ""+std::to_string(cost)+" HP" + colorString("FFFFFF") + "";
				}
			}
			string += "\n" + text;
		}
		if ( stats.currentMember["skills"].size() > stats.skill.selectionsMax ) {
			for ( int i = 0; i < i < stats.currentMember["skills"].size(); ++i ) {
				std::string id = stats.currentMember["skills"][i].asString();
				std::string text = "";
				if ( std::find( stats.skill.invalids.begin(), stats.skill.invalids.end(), id ) != stats.skill.invalids.end() ) continue;
				if ( ++counter > stats.skill.selectionsMax ) break;
				if ( id != "0" ) {
					uf::Serializer cardData = masterDataGet("Card", member["id"].asString());
					uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].asString());
					uf::Serializer skillData = masterDataGet("Skill", id);

					text = skillData["name"].asString();
				} else {
					uf::Serializer cardData = masterDataGet("Card", member["id"].asString());
					uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].asString());

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
			battleOptions->load("./battle-option.json", true);
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
		if ( actions.isNull() ) {
			stats.actions.selection = 0;
			queuedAction = "";
			return;
		}
		stats.actions.current = actions;
		std::string string = "";
		std::size_t counter = 0;
		std::size_t start = stats.actions.selection;
		auto keys = actions.getMemberNames();
		for ( int i = start; i < keys.size(); ++i ) {
			std::string key = getKeyFromIndex( actions, i );
			std::string text = actions[key].asString();
			if ( std::find( stats.actions.invalids.begin(), stats.actions.invalids.end(), key ) != stats.actions.invalids.end() ) continue;
			if ( ++counter > stats.actions.selectionsMax ) break;
			if ( i != stats.actions.selection ) text = "" + colorString("FFFFFF") + "" + text;
			else text = "" + colorString("FF0000") + "" + text;
			string += "\n" + text;
		}
		if ( keys.size() > stats.actions.selectionsMax ) {
			for ( int i = 0; i < keys.size(); ++i ) {
				std::string key = getKeyFromIndex( actions, i );
				std::string text = actions[key].asString();
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
			commandOptions->load("./battle-option.json", true);
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
		if ( actions.isNull() ) {
			stats.skill.selection = 0;
			stats.targets.selection = 0;
			return;
		}
		stats.targets.current = actions;
		std::string string = "";
		std::size_t counter = 0;
		std::size_t start = stats.targets.selection;
		stats.skill.invalids.clear();
		stats.actions.invalids.clear();
		std::string targetDescription = "";
		for ( int i = start; i < actions.size(); ++i ) {
			uf::Serializer cardData = masterDataGet("Card", actions[i]["id"].asString());
			uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].asString());
			std::string text = charaData["name"].asString();
			if ( std::find( stats.targets.invalids.begin(), stats.targets.invalids.end(), text ) != stats.targets.invalids.end() ) continue;
			if ( ++counter > stats.targets.selectionsMax ) break;
			if ( i != stats.targets.selection ) text = "" + colorString("FFFFFF") + "" + text;
			else text = "" + colorString("FF0000") + "" + text;
			string += "\n" + text;
		}
		if ( actions.size() > stats.targets.selectionsMax ) {
			for ( int i = 0; i < actions.size(); ++i ) {
				uf::Serializer cardData = masterDataGet("Card", actions[i]["id"].asString());
				uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].asString());
				std::string text = charaData["name"].asString();
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
			targetOptions->load("./battle-option.json", true);
			pMetadata["text settings"]["string"] = string;
			pod::Transform<>& pTransform = targetOptions->getComponent<pod::Transform<>>();
			targetOptions->initialize();
		}
		if ( queuedAction == "analyze" ) {
			uf::Serializer payload;
			payload["uid"] = stats.targets.current[stats.targets.selection]["uid"].asString();
		//	if ( payload["uid"].asString() == "")
			payload["action"] = "analyze";
			uf::Serializer result = battleManager->callHook("world:Battle.Action.%UID%", payload)[0];

			targetDescription = result["message"].asString();
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
		this->addHook("world:Battle.Turn.%UID%", [&](const std::string& event) -> std::string {
			uf::Serializer json = event;
			uf::Serializer payload;
			payload["action"] = "turn-start";
			if ( !stats.previousMember["uid"].isNull() ) payload["uid"] = stats.previousMember["uid"].asUInt64() + 1;
			auto hookCall = battleManager->callHook("world:Battle.Action.%UID%", payload);
			if ( hookCall.empty() ) {
				metadata["system"]["closing"] = true;
				return "false";
			}
			uf::Serializer result = hookCall[0];

			if ( result["end"].asBool() ) {
				if ( metadata.isObject() ) metadata["system"]["closing"] = true;
				return "false";
			}

			postParseResult(result);
			if ( result["uid"].isNull() ) return "false";
			uint64_t uid = result["uid"].asUInt64();

			renderCommandOptions(std::string(""));
			renderSkillOptions(std::string(""));
			renderTargetOptions(std::string(""));

			payload = uf::Serializer{};
			payload["uid"] = uid;
			if ( result[""]["type"] == "enemy" ) {
				payload["action"] = "enemy-attack";
			} else if ( result["skipped"].asBool() ) {
				payload["action"] = "forced-pass";
			} else {
				stats.currentMember = result[""];
				renderCommandOptions(result["actions"]);
				return "true";
			}
			result = battleManager->callHook("world:Battle.Action.%UID%", payload)[0];
			postParseResult(result);
			stats.previousMember = uf::Serializer{};
			timer.reset();
			stats.state = "waiting";
			this->queueHook("world:Battle.Turn.%UID%", "", timeout);
			return "true";
		});
		this->queueHook("world:Battle.Turn.%UID%");
	}

	{
		uf::Serializer& metadata = this->getComponent<uf::Serializer>();
		if ( metadata["system"]["closing"].asBool() ) {
			if ( alpha >= 1.0f ) {
				uf::Asset assetLoader;
				std::string canonical = assetLoader.load("./data/audio/ui/menu close.ogg");
				uf::Audio& sfx = assetLoader.get<uf::Audio>(canonical);
				sfx.setVolume(masterdata["volumes"]["sfx"].asFloat());
				sfx.play();
			}
			if ( alpha <= 0.0f ) {
				alpha = 0.0f;
				metadata["system"]["closing"] = false;
				metadata["system"]["closed"] = true;
			} else alpha -= uf::physics::time::delta;
			metadata["color"][3] = alpha;
		} else if ( metadata["system"]["closed"].asBool() ) {
			uf::Object& parent = this->getParent<uf::Object>();
			parent.getComponent<uf::Serializer>()["system"]["closed"] = true;
			this->destroy();
			parent.removeChild(*this);
		} else {
			if ( !metadata["initialized"].asBool() ) alpha = 0.0f;
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
		if ( metadata["hovered"].asBool() ) speed = -0.25f;
		time += uf::physics::time::delta * speed;
		transform.orientation = uf::quaternion::axisAngle( { 0.0f, 0.0f, 1.0f }, time );

		static pod::Vector3f start = { transform.position.x, -2.0f, 0.0f };
		static pod::Vector3f end = transform.position;
		static float delta = 0.0f;
		if ( !metadata["initialized"].asBool() ) delta = 0.0f;
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
		if ( metadata["hovered"].asBool() ) speed = 0.25f;
		time += uf::physics::time::delta * speed;
		transform.orientation = uf::quaternion::axisAngle( { 0.0f, 0.0f, 1.0f }, time );

		static pod::Vector3f start = { transform.position.x, 2.0f, 0.0f };
		static pod::Vector3f end = transform.position;
		static float delta = 0.0f;
		if ( !metadata["initialized"].asBool() ) delta = 0.0f;
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
	//		if ( pMetadata["half counter"].asBool() ) coeff = 4.0f;
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
		if ( !metadata["initialized"].asBool() ) delta = 0.0f;
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
				if ( metadata["clicked"].asBool() && timer.elapsed().asDouble() >= timeout ) {
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
				if ( metadata["clicked"].asBool() && timer.elapsed().asDouble() >= timeout ) {
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
		try {
			if ( !metadata["system"].isObject() ) {
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
			metadata["system"]["percent"] = metadata["system"]["percent"].asFloat() + uf::physics::time::delta;
		} catch ( ... ) {
			if ( particle ) {
				this->removeChild(*particle);
				particle->destroy();
				delete particle;
				*it = NULL;
			}
		}
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
		if ( metadata["hovered"].asBool() ) speed = 0.75f;
		time += uf::physics::time::delta * speed;
		transform.orientation = uf::quaternion::axisAngle( { 0.0f, 0.0f, 1.0f }, time );
		metadata["color"][3] = alpha;

		{
			pod::Vector3f start = {
				metadata["system"]["lerp"]["start"][0].asFloat(),
				metadata["system"]["lerp"]["start"][1].asFloat(),
				metadata["system"]["lerp"]["start"][2].asFloat(),
			};
			pod::Vector3f end = {
				metadata["system"]["lerp"]["end"][0].asFloat(),
				metadata["system"]["lerp"]["end"][1].asFloat(),
				metadata["system"]["lerp"]["end"][2].asFloat(),
			};
			float delta = metadata["system"]["lerp"]["delta"].asFloat();
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
				} while ( std::find( stats.targets.invalids.begin(), stats.targets.invalids.end(), stats.targets.current[stats.targets.selection]["id"].asString() ) != stats.targets.invalids.end() );
			}
		}
		else if ( keys.down ) {
			if ( released ) {
				delta = true;
				released = false;
				do {
					++stats.targets.selection;
					if ( stats.targets.selection >= stats.targets.current.size() ) { stats.targets.selection = 0; }
				} while ( std::find( stats.targets.invalids.begin(), stats.targets.invalids.end(), stats.targets.current[stats.targets.selection]["id"].asString() ) != stats.targets.invalids.end() );
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
			renderTargetOptions(std::string(""));
			if ( queuedAction == "analyze" ) {
				renderSkillOptions(std::string(""));
				renderCommandOptions(stats.actions.current);
			} else renderSkillOptions(stats.currentMember);
			timer.reset();
		} else if ( keys.select && timer.elapsed().asDouble() >= timeout ) {
			playSound(*this, "menu close");
			
			uf::Serializer payload;
			payload["action"] = "member-attack";
			payload["uid"] = stats.currentMember["uid"];
			payload["target"] = Json::arrayValue;
			payload["target"].append(stats.targets.current[stats.targets.selection]["uid"]);
			payload["skill"] = stats.currentMember["skills"][stats.skill.selection];

			uf::Serializer result = battleManager->callHook("world:Battle.Action.%UID%", payload)[0];
			postParseResult(result);

			renderTargetOptions(std::string(""));
			renderCommandOptions(std::string(""));
			renderSkillOptions(std::string(""));

			timer.reset();
			stats.state = "waiting";
			if ( result["invalid"].asBool() ) {
			//	stats.actions.invalids.push_back(action);
				renderCommandOptions(stats.actions.current);
			//	this->queueHook("world:Battle.Turn.%UID%", "", timeout);
			} else {
				this->queueHook("world:Battle.Turn.%UID%", "", timeout);
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
				} while ( std::find( stats.skill.invalids.begin(), stats.skill.invalids.end(), stats.currentMember["skills"][stats.skill.selection].asString() ) != stats.skill.invalids.end() );
			}
		}
		else if ( keys.down ) {
			if ( released ) {
				delta = true;
				released = false;
				do {
					++stats.skill.selection;
					if ( stats.skill.selection >= stats.currentMember["skills"].size()) { stats.skill.selection = 0; }
				} while ( std::find( stats.skill.invalids.begin(), stats.skill.invalids.end(), stats.currentMember["skills"][stats.skill.selection].asString() ) != stats.skill.invalids.end() );
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
			renderSkillOptions(std::string(""));
			renderCommandOptions(stats.actions.current);
			timer.reset();
		} else if ( keys.select && timer.elapsed().asDouble() >= timeout ) {
			playSound(*this, "menu close");
			bool allRanged = false;
			renderSkillOptions(std::string(""));
			renderCommandOptions(std::string(""));


			uf::Serializer payload;
			payload["action"] = "member-targets";
			payload["uid"] = stats.currentMember["uid"];
			payload["skill"] = stats.currentMember["skills"][stats.skill.selection];
			uf::Serializer result = battleManager->callHook("world:Battle.Action.%UID%", payload)[0];

			std::string skillId = stats.currentMember["skills"][stats.skill.selection].asString();
			uf::Serializer skillData = masterDataGet("Skill", skillId);
			if ( result["range"].isNumeric() ) skillData["range"] = result["range"];
			if ( skillData["range"].asInt64() > 1 ) {
				uf::Serializer payload;
				payload["action"] = "member-attack";
				payload["uid"] = stats.currentMember["uid"];
				payload["skill"] = stats.currentMember["skills"][stats.skill.selection];
				uf::Serializer result = battleManager->callHook("world:Battle.Action.%UID%", payload)[0];
				postParseResult(result);
				timer.reset();
				stats.state = "waiting";
				if ( result["invalid"].asBool() ) {
				//	stats.actions.invalids.push_back(action);
					renderCommandOptions(stats.actions.current);
				//	this->queueHook("world:Battle.Turn.%UID%", "", timeout);
				} else {
					this->queueHook("world:Battle.Turn.%UID%", "", timeout);
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
				} while ( std::find( stats.actions.invalids.begin(), stats.actions.invalids.end(), stats.actions.current[stats.actions.selection].asString() ) != stats.actions.invalids.end() );
			}
		}
		else if ( keys.down ) {
			if ( released ) {
				delta = true;
				released = false;
				do {
					++stats.actions.selection;
					if ( stats.actions.selection >= stats.actions.current.size() ) { stats.actions.selection = 0; }
				} while ( std::find( stats.actions.invalids.begin(), stats.actions.invalids.end(), stats.actions.current[stats.actions.selection].asString() ) != stats.actions.invalids.end() );
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
			std::string action = stats.actions.current[stats.actions.selection].asString();
			payload["action"] = action;
			if ( payload["action"] == "member-attack" ) {
				payload["uid"] = stats.currentMember["uid"];
				payload["skill"] = stats.currentMember["skills"][stats.skill.selection];
			}
			uf::Serializer result = battleManager->callHook("world:Battle.Action.%UID%", payload)[0];

			postParseResult(result);
			timer.reset();

			renderCommandOptions(std::string(""));

			stats.state = "waiting";
			if ( result["invalid"].asBool() ) {
				stats.actions.invalids.push_back(action);
				renderCommandOptions(stats.actions.current);
			//	this->queueHook("world:Battle.Turn.%UID%", "", timeout);
			} else if ( payload["action"] == "member-attack" ) {
				renderSkillOptions(std::string(""));
				stats.state = "waiting";
				this->queueHook("world:Battle.Turn.%UID%", "", timeout);
			} else if ( payload["action"] == "member-skill" ) {
				renderSkillOptions(stats.currentMember);
			} else if ( payload["action"] == "pass" ) {
				stats.previousMember = stats.currentMember;
				this->queueHook("world:Battle.Turn.%UID%", "", timeout);
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
			std::string action = getKeyFromIndex(stats.actions.current, stats.actions.selection);
			action = uf::string::extension(action);
			payload["action"] = action;
			payload["uid"] = stats.currentMember["uid"];
		
			if ( payload["action"] == "member-attack" ) {
			//	payload["skill"] = stats.currentMember["skills"][0];
				payload["skill"] = 0;
			}
		
			if ( payload["action"] == "analyze" ) {
				payload["action"] = "member-targets";
			}
			uf::Serializer result = battleManager->callHook("world:Battle.Action.%UID%", payload)[0];

		//	std::cout << payload << ": " << result << std::endl;

			postParseResult(result);
			timer.reset();

			renderCommandOptions(std::string(""));
			queuedAction = action;

			stats.state = "waiting";
			if ( result["invalid"].asBool() ) {
				stats.actions.invalids.push_back(action);
				renderCommandOptions(stats.actions.current);
			} else if ( payload["action"] == "member-attack" ) {
				renderSkillOptions(std::string(""));
				stats.state = "waiting";
				this->queueHook("world:Battle.Turn.%UID%", "", timeout);
			} else if ( payload["action"] == "member-skill" ) {
				renderSkillOptions(stats.currentMember);
			} else if ( payload["action"] == "member-targets" ) {
				renderTargetOptions(result["targets"]);
			} else if ( payload["action"] == "pass" ) {
				stats.previousMember = stats.currentMember;
				this->queueHook("world:Battle.Turn.%UID%", "", timeout);
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