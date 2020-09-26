#include "battle.h"

#include ".h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/graphic/mesh.h>
#include <uf/utils/window/window.h>
#include <uf/utils/camera/camera.h>

#include <uf/utils/renderer/renderer.h>


#include <uf/utils/image/image.h>
#include <uf/utils/audio/audio.h>

#include "../gui/battle.h"
#include <uf/engine/asset/asset.h>

namespace {
	ext::Gui* gui;
	std::string previousMusic;

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

	inline std::string colorString( const std::string& hex ) {
		return "%#" + hex + "%";
	}

}

namespace {
	void playSound( ext::HousamoBattle& entity, const std::string& id, const std::string& key ) {
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();

		uf::Serializer cardData = masterDataGet("Card", id);
		uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].asString());
		std::string name = charaData["filename"].asString();

		if ( name == "none" ) return;

		std::string url = "https://cdn..xyz//unity/Android/voice/voice_" + name + "_"+key+".ogg";
		if ( charaData["internal"].asBool() ) {
			url = "./data/smtsamo/voice/voice_" + name + "_" + key + ".ogg";
		}

		uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
		assetLoader.cache(url, "asset:Cache.Voice." + std::to_string(entity.getUid()));
	}
	void playSound( ext::HousamoBattle& entity, std::size_t uid, const std::string& key ) {
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& pMetadata = entity.getComponent<uf::Serializer>();
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();

		uf::Entity*  = scene.findByUid(uid);
		if ( ! ) return;
		uf::Serializer& metadata = ->getComponent<uf::Serializer>();
		std::string id = metadata[""]["id"].asString();
		playSound( entity, id, key );
	}
	void playSound( ext::HousamoBattle& entity, const std::string& key ) {
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& metadata = entity.getComponent<uf::Serializer>();
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();

		std::string url = "./data/audio/ui/" + key + ".ogg";

		uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
		assetLoader.cache(url, "asset:Cache.SFX." + std::to_string(entity.getUid()));
	}
	void playMusic( ext::HousamoBattle& entity, const std::string& filename ) {
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();

		uf::Audio& audio = scene.getComponent<uf::Audio>();
		if ( audio.playing() ) {
			uf::Serializer& metadata = scene.getComponent<uf::Serializer>();
			metadata["previous bgm"]["filename"] = audio.getFilename();
			metadata["previous bgm"]["timestamp"] = audio.getTime();
		}

		assetLoader.load(filename, "asset:Load." + std::to_string(scene.getUid()));
	}

	uf::Audio sfx;
	uf::Audio voice;
	std::vector<ext::Housamo*> transients;
}
void ext::HousamoBattle::initialize() {	
	uf::Object::initialize();

	gui = NULL;
	transients.clear();

	uf::Scene& scene = uf::scene::getCurrentScene();
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
//	std::vector<ext::Housamo>& transients = this->getComponent<std::vector<ext::Housamo>>();

	this->m_name = "Battle Manager";

	// asset loading
	this->addHook( "asset:Cache.Voice.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& metadata = this->getComponent<uf::Serializer>();
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();
		
		std::string filename = json["filename"].asString();

		if ( uf::string::extension(filename) != "ogg" ) return "false";

		if ( filename == "" ) return "false";
		if ( voice.playing() ) voice.stop();
		voice = uf::Audio();
		voice.load(filename);
		voice.setVolume(masterdata["volumes"]["voice"].asFloat());
		voice.play();

		return "true";
	});
	this->addHook( "asset:Cache.SFX.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();
		
		std::string filename = json["filename"].asString();

		if ( uf::string::extension(filename) != "ogg" ) return "false";

		if ( filename == "" ) return "false";
		if ( sfx.playing() ) sfx.stop();
		sfx = uf::Audio();
		sfx.load(filename);
		sfx.setVolume(masterdata["volumes"]["sfx"].asFloat());
		sfx.play();

		return "true";
	});
	this->addHook( "asset:Music.Load.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		std::string filename = "";
		if ( json["music"].isArray() ) {
			int ri = floor(rand() % json["music"].size());
			filename = json["music"][ri].asString();
		} else if ( json["music"].isString() ) {
			filename = json["music"].asString();
		}
		if ( filename != "" ) playMusic(*this, filename);
		return "true";
	});

	// bind events
	this->addHook( "world:Battle.Start.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;

		metadata["battle"] = json["battle"];
		{
			uf::Serializer payload;
			payload["music"] = metadata["battle"]["music"];
			this->queueHook( "asset:Music.Load.%UID%", payload );
		}
		
		// generate battle data
	
		uf::Serializer actions; actions.readFromFile("./data/entities/gui/battle/actions.json");
		metadata["actions"] = actions;
		if ( metadata["actions"].isNull() ) {
			metadata["actions"]["00.member-attack"] = "攻撃";
			metadata["actions"]["01.member-skill"] = "スキル";
			metadata["actions"]["02.inventory"] = "アイテム";
			metadata["actions"]["03.talk"] = "話し掛ける";
			metadata["actions"]["04.transients"] = "転光生";
			metadata["actions"]["05.escape"] = "脱走";
			metadata["actions"]["06.analyze"] = "Analyze";
			metadata["actions"]["07.pass"] = "Pass";
		}

		// spawn transients accordingly
		for ( auto& id : metadata["battle"]["player"]["party"] ) {
			uint64_t uid = transients.size();
			auto& member = metadata["battle"]["player"]["transients"][id.asString()];
			member["uid"] = uid;

			ext::Housamo* transient = new ext::Housamo;
			transients.push_back(transient);
			this->addChild(*transient);

			uf::Serializer& pMetadata = transient->getComponent<uf::Serializer>();
			pMetadata[""] = member;
			pMetadata[""]["uid"] = uid;
			pMetadata[""]["type"] = "player";
			pMetadata[""]["initialized"] = false;
			transient->initialize();
			metadata["battle"]["transients"][std::to_string(uid)] = pMetadata[""];
		}
		for ( auto& id : metadata["battle"]["enemy"]["party"] ) {
			uint64_t uid = transients.size();
			auto& member = metadata["battle"]["enemy"]["transients"][id.asString()];
			member["uid"] = uid;

			ext::Housamo* transient = new ext::Housamo;
			transients.push_back(transient);
			this->addChild(*transient);

			uf::Serializer& pMetadata = transient->getComponent<uf::Serializer>();
			pMetadata[""] = member;
			pMetadata[""]["uid"] = uid;
			pMetadata[""]["type"] = "enemy";
			pMetadata[""]["initialized"] = false;
			transient->initialize();
			metadata["battle"]["transients"][std::to_string(uid)] = pMetadata[""];
		}

		this->queueHook("world:Battle.Gui.%UID%");

		uf::Serializer payload;
		for ( auto& member : metadata["battle"]["transients"] ) {
			for ( auto& skillId : member["skills"]  ) {
				uf::Serializer skillData = masterDataGet("Skill", skillId.asString());
				if ( skillData["type"].asInt64() != 16 ) continue;
				for ( auto& status : skillData["statuses"] ) {
					std::string target = status["target"].asString();
					std::string statusId = status["id"].asString();
					uf::Serializer statusData = masterDataGet("Status", statusId);

					float r = (rand() % 100) / 100.0;
					if ( statusData["proc"].isNumeric() ) {
						// failed
						if ( r > statusData["proc"].asFloat() / 100.0f ) {
							target = "";
						}
					}
					// apply status
					uf::Serializer targets;								
					if ( target == "self" ) {
						targets.append(member["uid"].asString());
					} else if ( target == "enemy" ) {
						for ( auto& tmember :  metadata["battle"]["transients"] ) {
							if ( tmember["type"] == member["type"] ) continue;
							targets.append(tmember["uid"].asString());
						}
					} else if ( target == "ally" ) {
						for ( auto& tmember :  metadata["battle"]["transients"] ) {
							if ( tmember["type"] != member["type"] ) continue;
							targets.append(tmember["uid"].asString());
						}
					}
					for ( auto& target : targets ) {
						uf::Serializer statusPayload;
						statusPayload["id"] = statusId;
						statusPayload["modifier"] = status["modifier"];

						statusPayload["turns"] = status["turns"].isNumeric() ? status["turns"] : statusData["turns"];
						metadata["battle"]["transients"][target.asString()]["statuses"].append(statusPayload);

						uf::Serializer cardData = masterDataGet( "Card", metadata["battle"]["transients"][target.asString()]["id"].asString() );
						uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].asString());

						payload["message"] = payload["message"].asString() + "\nApplied "+colorString("FF0000") + "" + statusData["name"].asString() + ""+colorString("FFFFFF") + " to " + charaData["name"].asString();
					}
				}
			}
		}
		
		payload["battle"] = metadata["battle"];
		return payload;
	});
	this->addHook( "world:Battle.Gui.%UID%", [&](const std::string& event)->std::string{	
		ext::Gui* guiManager = (ext::Gui*) this->getRootParent().findByName("Gui Manager");
		ext::Gui* guiMenu = new ext::GuiBattle;
		guiManager->addChild(*guiMenu);
		guiMenu->load("./scenes/world/gui/battle/menu.json");
		guiMenu->initialize();

		uf::Serializer payload;
		payload["battle"] = metadata["battle"];
		guiMenu->queueHook("world:Battle.Start.%UID%", payload);

		gui = guiMenu;

		return "true";
	});
	// json.action and json.uid
	this->addHook( "world:Battle.Action.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		
		std::string hookName = "";
		uf::Serializer payload;

		std::string action = json["action"].asString();
		
		// dead check
		struct {
			struct {
				int dead = 0;
				int total = 0;
			} player, enemy;
		} counts;
		for ( auto& member : metadata["battle"]["transients"] ) {
			bool dead = member["hp"].asInt64() <= 0;
			if ( member["type"] == "player" ) {
				++counts.player.total;
				if ( dead ) ++counts.player.dead;
			} else {
				++counts.enemy.total;
				if ( dead ) ++counts.enemy.dead;
			}
		}
		
		if ( counts.player.total > 0 && counts.player.total == counts.player.dead ) {
			hookName = "world:Battle.End.%UID%";
			payload["won"] = false;
		} else if ( counts.enemy.total > 0 && counts.enemy.total == counts.enemy.dead  ) {
			hookName = "world:Battle.End.%UID%";
			payload["won"] = true;
		} else if ( action == "turn-start" ) {
			payload = json;
			hookName = "world:Battle.Turn.%UID%";
		} else if ( action == "member-turn" ) {
			payload = json;
			hookName = "world:Battle.Get.%UID%";
		} else if ( action == "member-skill" ) {
			payload = json;
			hookName = "world:Battle.Get.%UID%";
			// test
		} else if ( action == "member-attack" ) {
			payload = json;
			hookName = "world:Battle.Attack.%UID%";
			metadata["battle"]["turn"]["decrement"] = 1.0f;
		} else if ( action == "member-targets" ) {
			payload = json;
			hookName = "world:Battle.GetTargets.%UID%";
		} else if ( action == "enemy-attack" ) {
			payload = this->callHook("world:Battle.AI.%UID%", json)[0];
			hookName = "world:Battle.Attack.%UID%";
			metadata["battle"]["turn"]["decrement"] = 1.0f;
		} else if ( action == "escape" ) {
			payload = json;
			hookName = "world:Battle.Escape.%UID%";
			metadata["battle"]["turn"]["decrement"] = 1.0f;
		} else if ( action == "talk" ) {
			payload = json;
			hookName = "world:Battle.Talk.%UID%";
			metadata["battle"]["turn"]["decrement"] = 1.0f;
		} else if ( action == "pass" ) {
			payload["uid"] = json["uid"];
			payload["message"] = "Waiting...";
			payload["timeout"] = 2.0f;
			// take half
			metadata["battle"]["turn"]["decrement"] = 0.5f;
		} else if ( action == "force-pass" ) {
			payload["uid"] = json["uid"];
			payload["message"] = "Immobilized...";
			payload["timeout"] = 2.0f;
		} else if ( action == "analyze" ) {
			payload = json;
			hookName = "world:Battle.Get.%UID%";
		} else {
			payload["message"] = "Invalid command.";
			payload["timeout"] = 2.0f;
			payload["invalid"] = true;
		}

		// this->callHook("world:Battle.AI.%UID%", json)[0];

		{
			auto& turnState = metadata["battle"]["turn"];
			float decrement = turnState["decrement"].asFloat();
			turnState["counter"] = turnState["counter"].asFloat() - decrement;
			turnState["decrement"] = 0;
		}

		{
			for ( auto it = metadata["battle"]["transients"].begin(); it != metadata["battle"]["transients"].end(); ++it ) {
				std::string key = it.key().asString();
				if ( key == "" || (*it)["type"].isNull() ) metadata["battle"].removeMember(key);
			}
		}

		/* remove dead */ if ( false ) {
			for ( auto it = metadata["battle"]["transients"].begin(); it != metadata["battle"]["transients"].end(); ++it ) {
				std::string key = it.key().asString();
				if ( metadata["battle"]["transients"][key]["hp"].asInt64() > 0 ) continue;
			//	metadata["battle"]["transients"].removeMember(key);
			}
			// std::cout << metadata["battle"]["transients"] << std::endl;
		}

		if ( gui ) {
			uf::Serializer payload;
			payload["battle"] = metadata["battle"];
			gui->queueHook("world:Battle.Update.%UID%", payload);
		}

		if ( hookName == "" ) return payload;

		uf::Serializer result;
		result = this->callHook(hookName, payload)[0];
		return result;
	});

	this->addHook( "world:Battle.AI.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		// default to attack
		std::string queuedSkillId = "0";
		if ( json["uid"] == "" ) {
			json["skill"] = "0";
			return json;
		}
		auto& member = metadata["battle"]["transients"][json["uid"].asString()];
		// deduce member state
		struct {
			std::vector<std::string> ailments;
			int ndas = 0;
			float factor = 0.0f;
		} stats;
		for ( auto& status : member["statuses"] ) {
			std::string id = status["id"].asString();
			if ( id == "0" ) continue;
			stats.ailments.push_back(id);
			if ( id == "6" ) ++stats.ndas;
		}

		uf::Serializer possibilities;
		possibilities = Json::arrayValue;
		for ( auto& skillId : member["skills"] ) {
			uf::Serializer skillData = masterDataGet("Skill", skillId.asString());
			// not recarms
			std::cout << skillData["name"] << ": ";
			if ( skillId == "81" || skillId == "82" ) {
				std::cout << "\tBad skill ID" << std::endl;
				continue;
			}
			// invalid skill type
			if ( skillData["type"].asInt() < 1 || skillData["type"].asInt() > 15 ) {
				std::cout << "\tBad skill type" << std::endl;
				continue;
			}
			// no MP
			if ( skillData["mp"].asFloat() > member["mp"].asFloat() ) {
				std::cout << "\tNot enough MP" << std::endl;
				continue;
			}
			// no HP
			if ( skillData["hp%"].asFloat() / 100.0f * member["max hp"].asFloat() >= member["hp"].asFloat() ) {
				std::cout << "\tNot enough HP" << std::endl;
				continue;
			}
			// do not use same skills
			if ( std::find(member["used skills"].begin(), member["used skills"].end(), skillId.asString()) != member["used skills"].end() ) {
				std::cout << "\tRecently Used" << std::endl;
				continue;
			}
			// grants additional turns
			if ( skillData["turns+"].isNumeric() ) {
				queuedSkillId = skillId.asString();
				std::cout << "\tCan gain turns" << std::endl;
				break;
			// try and heal if under 30%
			} else if ( member["hp"].asFloat() / member["max hp"].asFloat() < 0.3f ) {
				if ( skillData["type"] != "14" ) continue;
				if ( stats.factor < skillData["power"].asFloat() ) {
					queuedSkillId = skillId.asString();
					stats.factor = skillData["power"].asFloat();
					std::cout << "\tCan heal" << std::endl;
				}
			// remove debuffs
			} else if ( skillId == "200" ) {
				if ( stats.ndas * 75.0f > stats.factor ) {
					queuedSkillId = skillId.asString();
					stats.factor = stats.ndas * 75.0f;
					std::cout << "\tCan Dekunda" << std::endl;
				}
				continue;
			} else {
				// remove negative ailment (fear, etc)
				for ( auto& status : skillData["statuses"] ) {
					std::string statusId = status["id"].asString();
					uf::Serializer statusData = masterDataGet("Status", statusId);
					for ( auto it = status["modifier"].begin(); it != status["modifier"].end(); ++it ) {
						statusData[it.key().asString()] = *it;
					}
					// remove statuses
					if ( statusData["type"] == "remove" && std::find( stats.ailments.begin(), stats.ailments.end(), statusId ) != stats.ailments.end() ) {
						queuedSkillId = skillId.asString();
						stats.factor = 150.0f;
						std::cout << "\tCan remove status " << statusData["name"] << std::endl;
					}
				}
				std::cout << "\tRandom" << std::endl;
				// no heal spells
				if ( skillData["type"] == "14" ) continue;
				// let RNG pick
				possibilities.append(skillId);
			}
		}
		// let RNG pick
		std::cout << queuedSkillId << ": " << possibilities << std::endl;
		if ( queuedSkillId == "0" && possibilities.size() > 0 ) {
			possibilities.append("0");
			int ri = floor(rand() % possibilities.size());
			queuedSkillId = possibilities[ri].asString();
			auto queuedSkillData = masterDataGet( "Skill", queuedSkillId );
			std::cout << "Picked " << queuedSkillData["name"].asString() << std::endl;
		}
		json["skill"] = queuedSkillId;
		return json;
	});
	this->addHook( "world:Battle.GetTargets.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		uf::Serializer payload;

		payload["targets"] = Json::arrayValue;
		auto& member = metadata["battle"]["transients"][json["uid"].asString()];
		if ( json["skill"].isString() ) {
			std::string skillId = json["skill"].asString();
			if ( json["skill"] == "0" ) {
				for ( auto& status : member["statuses"] ) {
					std::string statusId = status["id"].asString();
					uf::Serializer statusData = masterDataGet("Status", statusId);
					for ( auto it = status["modifier"].begin(); it != status["modifier"].end(); ++it ) {
						statusData[it.key().asString()] = *it;
					}
					if ( statusData["skill"] != skillId ) continue;
					if ( statusData["range"].isNumeric() ) json["range"] = statusData["range"];
				}
			} else if ( !json["type"].isString() ) {
				uf::Serializer skillData = masterDataGet( "Skill", skillId );
				json["type"] = skillData["target"].asString();
				if ( json["type"] == "" ) json["type"] = "enemy";
			}
		} else {
			json["type"] = "all";
		}

		if ( json["type"].isString() ) {
			if ( json["type"] == "self" ) {
				payload["targets"].append( metadata["battle"]["transients"][json["uid"].asString()] );
			} else {
				for ( auto& m : metadata["battle"]["transients"] ) {
					if ( json["type"] == "ally" && m["type"] != member["type"] ) continue;
					if ( json["type"] == "enemy" && m["type"] == member["type"] ) continue;
					payload["targets"].append( m );
				}
			}
		}
		return payload;
	});
	this->addHook( "world:Battle.Escape.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer payload;
		float r = (rand() % 100) / 100.0;
		if ( r > 0.9 ) {
			payload["message"] = "Cannot escape!";
			payload["timeout"] = 2.0f;
			payload["invalid"] = true;
		} else {
			payload["message"] = "Escaped!";
			payload["timeout"] = 2.0f;
			this->queueHook("world:Battle.End.%UID%", payload, 2.0f);
		}
		return payload;
	});
	this->addHook( "world:Battle.Talk.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer payload;
		float r = (rand() % 100) / 100.0;
		payload["message"] = "";
		payload["timeout"] = 2.0f;
		payload["invalid"] = true;

		uf::Serializer target;
		std::string name;

		for ( auto& member : metadata["battle"]["transients"] ) {
			if ( member["type"] != "enemy" ) continue;
			target = member;
			size_t uid = member["uid"].asUInt64();
			uf::Serializer cardData = masterDataGet("Card", member["id"].asString());
			uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].asString());
			name = charaData["name"].asString();
			break;
		}
		if ( target ) {
			playSound(*this, target["id"].asString(), r > 0.5 ? "turn" : "battlestart");
			payload["message"] = ""+colorString("FF0000") + "" + name + ""+colorString("FFFFFF") + " speaks.";
		}

		return payload;
	});
	this->addHook( "world:Battle.OnHit.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		uint64_t uid = json["uid"].asUInt64();
		ext::Housamo* transient = transients.at(uid);
		uf::Serializer& member = transient->getComponent<uf::Serializer>();

		uint64_t entid = metadata["battle"]["enemy"]["uid"].asUInt64();
		uf::Entity*  = scene.findByUid(entid);
		if ( ! ) return "false";
		uf::Serializer& hMetadata = ->getComponent<uf::Serializer>();
		uint64_t phase = json["phase"].asUInt64();
		// start color
		pod::Vector4f color = { 1, 1, 1, 0 };
		if ( phase == 0 ) {
			color = (member[""]["type"] == "enemy") ?  pod::Vector4f{ 1.0f, 0, 0, 0.6f } : pod::Vector4f{ 1.0f, 1.0f, 1.0f, 0.6f };
		}
		hMetadata["color"][0] = color[0];
		hMetadata["color"][1] = color[1];
		hMetadata["color"][2] = color[2];
		hMetadata["color"][3] = color[3];
		return "true";
	});
	this->addHook( "world:Battle.Turn.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;

		bool found = false;
		uint64_t turnPlayer = json["uid"].asUInt64();
		std::string name = "";
		auto& turnState = metadata["battle"]["turn"];
		{
			if ( turnState["counter"].asFloat() <= 0 ) {
				if ( turnState["phase"] == "player" ) turnState["phase"] = "enemy";
				else turnState["phase"] = "player";
				int counter = 0;
				for ( auto& member : metadata["battle"]["transients"] ) {
					if ( member["type"] != turnState["phase"] ) continue;

					bool locked = false;
					for ( auto& status : member["statuses"] ) {
						std::string statusId = status["id"].asString();
						uf::Serializer statusData = masterDataGet("Status", statusId);
						for ( auto it = status["modifier"].begin(); it != status["modifier"].end(); ++it ) {
							statusData[it.key().asString()] = *it;
						}

						if ( statusData["lock"].asBool() ) locked = true;
					}
					if ( locked ) continue;

					++counter;
				}
				turnState["counter"] = counter;
				turnState["start of turn"] = true;

				if ( gui ) {
					uf::Serializer payload;
					payload["battle"] = metadata["battle"];
					gui->queueHook("world:Battle.TurnStart.%UID%", payload);
				}
			} else {
				turnState["start of turn"] = false;
			}
		}

		std::function<bool()> getTurnPlayer = [&]()->bool{
			for ( auto& member : metadata["battle"]["transients"] ) {
				if ( member["hp"].asInt64() <= 0 ) continue;
			
				bool locked = false;
				for ( auto& status : member["statuses"] ) {
					std::string statusId = status["id"].asString();
					uf::Serializer statusData = masterDataGet("Status", statusId);
					for ( auto it = status["modifier"].begin(); it != status["modifier"].end(); ++it ) {
						statusData[it.key().asString()] = *it;
					}

					if ( statusData["lock"].asBool() ) locked = true;
				}
				if ( locked ) continue;
			
				if ( turnPlayer > member["uid"].asUInt64() ) continue;
				if ( member["type"] != turnState["phase"] ) continue;
				turnPlayer = member["uid"].asUInt64();
				return true;
			}
			return false;
		};

		uf::Serializer payload;
		payload["message"] = "An error occurred!";

		if ( !getTurnPlayer() ) {
			turnPlayer = 0;
			if ( !getTurnPlayer() ) {
				std::cout << metadata["battle"]["transients"] << std::endl;
				payload["end"] = true;
				return payload;
			}
		}

		payload["uid"] = turnPlayer;
		auto& member = metadata["battle"]["transients"][payload["uid"].asString()];
		uf::Serializer cardData = masterDataGet("Card", member["id"].asString());
		uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].asString());
		name = charaData["name"].asString();
		payload["message"] = "What will "+colorString("FF0000") + "" + name + ""+colorString("FFFFFF") + " do?";

		// 
		if ( member["type"] == "enemy" ) {
			payload["message"] = ""+colorString("FF0000") + "" + name + ""+colorString("FFFFFF") + "'s turn...";
			payload["timeout"] = 2.0f;
		}

		bool skipped = false;
		// tick down statuses
		if ( turnState["start of turn"].asBool() ) {
			/* remove dead */ if ( false ) {
				for ( auto it = metadata["battle"]["transients"].begin(); it != metadata["battle"]["transients"].end(); ++it ) {
					std::string key = it.key().asString();
					if ( metadata["battle"]["transients"][key]["hp"].asInt64() > 0 ) continue;
				//	metadata["battle"]["transients"].removeMember(key);
				}
			//	std::cout << metadata["battle"]["transients"] << std::endl;
			}
			for ( auto& member : metadata["battle"]["transients"] ) {
				// clear OPT used skills
				member["used skills"] = Json::arrayValue;
				if ( member["type"] != turnState["phase"] ) continue;
				auto& statuses = member["statuses"];
//				std::cout << "Before: " << member["uid"].asString() << ": " << statuses << std::endl;
				for ( int i = 0; i < statuses.size(); ++i ) {
					auto& status = statuses[i];
					uf::Serializer statusData = masterDataGet("Status", status["id"].asString());
					for ( auto it = status["modifier"].begin(); it != status["modifier"].end(); ++it ) {
						statusData[it.key().asString()] = *it;
					}
					
					uf::Serializer cardData = masterDataGet( "Card", member["id"].asString() );
					uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].asString());
					
					if ( statusData["hp"].isNumeric() ) {
						int64_t hp = statusData["hp"].asInt64();
						member["hp"] = member["hp"].asInt64() + hp;
						payload["message"] = payload["message"].asString() + "\n"+colorString("FF0000") + "" + charaData["name"].asString() +  ""+colorString("FFFFFF") + " gained "+colorString("00FF00") + ""+ std::to_string(hp) +" HP"+colorString("FFFFFF") + " from "+colorString("FF0000") + "" + statusData["name"].asString() + ""+colorString("FFFFFF") + "";
					}
					if ( statusData["hp%"].isNumeric() ) {
						int64_t hp = member["max hp"].asFloat() * (statusData["hp%"].asInt64() / 100.0f);
						if ( member["max hp"].asInt64() < member["hp"].asInt64() + hp ) hp = member["max hp"].asInt64() - member["hp"].asInt64();
						member["hp"] = member["hp"].asInt64() + hp;
						if ( hp > 0 ) payload["message"] = payload["message"].asString() + "\n"+colorString("FF0000") + "" + charaData["name"].asString() +  ""+colorString("FFFFFF") + " gained "+colorString("00FF00") + ""+ std::to_string(hp) +" HP"+colorString("FFFFFF") + " from "+colorString("FF0000") + "" + statusData["name"].asString() + ""+colorString("FFFFFF") + "";
					}
					if ( statusData["mp"].isNumeric() ) {
						int64_t mp = statusData["mp"].asInt64();
						member["mp"] = member["mp"].asInt64() + mp;
						if ( member["max mp"].asInt64() < member["mp"].asInt64() + mp ) mp = member["max mp"].asInt64() - member["mp"].asInt64();
						payload["message"] = payload["message"].asString() + "\n"+colorString("FF0000") + "" + charaData["name"].asString() +  ""+colorString("FFFFFF") + " gained "+colorString("0000FF") + ""+ std::to_string(mp) +" MP"+colorString("FFFFFF") + " from "+colorString("FF0000") + "" + statusData["name"].asString() + ""+colorString("FFFFFF") + "";
					}
					if ( statusData["lock"].isNumeric() ) {
						skipped = true;
					}
				}
				uf::Serializer newStatuses;
				for ( int i = statuses.size() - 1; i >= 0; --i ) {
					auto& status = statuses[i];
					uf::Serializer statusData = masterDataGet("Status", status["id"].asString());
					if ( !status["turns"].isNull() ) {
						status["turns"] = status["turns"].asInt64() - 1;
						if ( status["turns"].asInt64() < 0 ) {
							if ( statusData["id"] != "0" ) payload["message"] = payload["message"].asString() + "\n"+colorString("FF0000") + "" + statusData["name"].asString() + ""+colorString("FFFFFF") + " wore off.";
						} else {
							newStatuses.append(status);
						}
					} else {
						newStatuses.append(status);
					}
				}
				statuses = newStatuses;
//				std::cout << "After: " << member["uid"].asString() << ": " << statuses << std::endl;
			}
		}

		{
			for ( auto it = metadata["battle"]["transients"].begin(); it != metadata["battle"]["transients"].end(); ++it ) {
				std::string key = it.key().asString();
				if ( key == "" || (*it)["type"].isNull() ) metadata["battle"].removeMember(key);
			}
		}

		if ( gui ) {
			uf::Serializer payload;
			payload["battle"] = metadata["battle"];
			gui->queueHook("world:Battle.Update.%UID%", payload);
		}

		if ( skipped ) payload["skipped"] = true;
	/*
		std::vector<std::string> list = {
			"member-attack",
			"member-skill",
			"inventory",
			"talk",
			"transients",
			"escape",
			"pass",
			"analyze",
		};
		payload["actions"]["list"] = Json::arrayValue;
		payload["actions"]["names"]["member-attack"] = "攻撃";
		payload["actions"]["names"]["member-skill"] = "スキル";
		payload["actions"]["names"]["inventory"] = "アイテム";
		payload["actions"]["names"]["talk"] = "話し掛ける";
		payload["actions"]["names"]["transients"] = "転光生";
		payload["actions"]["names"]["escape"] = "脱走";
		payload["actions"]["names"]["analyze"] = "Analyze";
		payload["actions"]["names"]["pass"] = "Pass";
		for ( auto& x : list ) payload["actions"]["list"].append(x);
	*/
		payload["actions"] = metadata["actions"];
		payload[""] = member;
		return payload;
	});
	this->addHook( "world:Battle.Attack.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;

		std::string skillId = json["skill"].asString();
		uf::Serializer skillData = masterDataGet("Skill", skillId);

//		ext::Housamo* transient = transients.at(uid);
		auto& member = metadata["battle"]["transients"][json["uid"].asString()];

		uf::Serializer payload;
		payload["message"] = "";
		// modify skill data based on statuses
		for ( auto& status : member["statuses"] ) {
			std::string statusId = status["id"].asString();
			uf::Serializer statusData = masterDataGet("Status", statusId);
			for ( auto it = status["modifier"].begin(); it != status["modifier"].end(); ++it ) {
				statusData[it.key().asString()] = *it;
			}
			if ( statusData["skill"] != skillId ) continue;
			if ( statusData["range"].isNumeric() ) skillData["range"] = statusData["range"];
		}
		// select random target
		if ( json["target"].isNull() ) {
			if ( skillData["target"].isNull() ) skillData["target"] = "enemy";
			if ( skillData["target"] == "self" ) {
				json["target"].append( member["uid"].asUInt64() );
			} else if ( skillData["range"].isNumeric() && skillData["range"].asInt64() > 1 ) {
				json["target"] = Json::arrayValue;
				for ( auto& m : metadata["battle"]["transients"] ) {
					if ( skillData["target"] == "ally" && m["type"] != member["type"] ) continue;
					if ( skillData["target"] != "ally" && m["type"] == member["type"] ) continue;
					json["target"].append( m["uid"].asUInt64() );
				}
			} else {
				std::vector<uint64_t> targets;
				for ( auto& m : metadata["battle"]["transients"] ) {
					if ( skillData["target"] == "ally" && m["type"] != member["type"] ) continue;
					if ( skillData["target"] != "ally" && m["type"] == member["type"] ) continue;
					targets.push_back( m["uid"].asUInt64() );
				}
				int ri = floor(rand() % targets.size());
				json["target"] = Json::arrayValue;
				json["target"].append(targets.at(ri));
			}
		}

		uf::Serializer elementData;
		uf::Serializer cardData = masterDataGet("Card", member["id"].asString());
		uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].asString());
		std::string elementId;
		
		float basePower = member["type"] == "enemy" ? 1.2f : 1.0f;

		if ( skillId != "0" ) {
			payload["message"] = payload["message"].asString() + "\n"+colorString("FF0000") + "" + charaData["name"].asString() + ""+colorString("FFFFFF") + ": "+colorString("7777FF") + "" + skillData["name"].asString() + colorString("FFFFFF");

			elementId = skillData["type"].asString();
			elementData = masterDataGet("Element", elementId);
		} else {
			payload["message"] = payload["message"].asString() + "\n"+colorString("FF0000") + "" + charaData["name"].asString() + ""+colorString("FFFFFF") + ": "+colorString("7777FF") + "攻撃" + colorString("FFFFFF");

			elementId = "0";
			if ( cardData["weapon_type"] == "1" ) elementId = "1"; // slash
			if ( cardData["weapon_type"] == "2" ) elementId = "3"; // strike
			if ( cardData["weapon_type"] == "3" ) elementId = "2"; // pierce
			elementData = masterDataGet("Element", elementId);
		}

		struct {
			float damage = 0.0f;
		} delays;

		struct {
			bool endTurn = false;
			int loseTurn = 0;
		} turnEffects;

		std::function<void(uf::Serializer&)> showMessage = [&]( uf::Serializer& json ) {
			uf::Serializer payload;
			payload["color"] = json["color"];
			payload["target"]["uid"] = json["uid"];
			payload["target"]["damage"] = json["message"];
			gui->queueHook("world:Battle.Damage.%UID%", payload, delays.damage);
			delays.damage += 0.25f;
		};
		
		{
			int64_t mp = member["mp"].asInt64();
			int64_t hp = member["hp"].asInt64();
			int64_t mpCost = skillData["mp"].asInt64();
			int64_t hpCost = skillData["hp%"].asInt64();

			for ( auto& status : member["statuses"] ) {
				std::string statusId = status["id"].asString();
				uf::Serializer statusData = masterDataGet("Status", statusId);
				for ( auto it = status["modifier"].begin(); it != status["modifier"].end(); ++it ) {
					statusData[it.key().asString()] = *it;
				}

				if ( !statusData["type"].isNull() && elementData["type"] != statusData["type"] ) continue;
				if ( !statusData["attributes"].isNull() ) {
					bool found = false;
					for ( auto& value : statusData["attributes"] ) {
						if ( value == elementData["id"] ) found = true;
					}
					if ( !found ) continue;
				}

				if ( hpCost > 0 && statusData["cost"].isNumeric() ) {
					hpCost -= statusData["cost"].asInt64();
				}
			}

			hpCost = member["max hp"].asFloat() * ( hpCost / 100.0f );

			if ( mp < mpCost ) {
				payload["message"] = "Not enough MP to use `"+colorString("FF0000") + ""+skillData["name"].asString()+""+colorString("FFFFFF") + "`.";
				payload["timeout"] = 2.0f;
				if ( member["type"] != "enemy" ) {
					payload["invalid"] = true;
				}
				return payload;
			} else if ( mpCost > 0 ) {
				metadata["battle"]["transients"][member["uid"].asString()]["mp"] = mp - mpCost;
			}
			if ( hp < hpCost ) {
				payload["message"] = "Not enough HP to use `"+colorString("FF0000") + ""+skillData["name"].asString()+""+colorString("FFFFFF") + "`.";
				payload["timeout"] = 2.0f;
				if ( member["type"] != "enemy" ) {
					payload["invalid"] = true;
					return payload;
				} else {
					skillId = "0";
					skillData = masterDataGet("Skill", skillId);
				}
			} else if ( hpCost > 0 ) {
				metadata["battle"]["transients"][member["uid"].asString()]["hp"] = hp - hpCost;
				{
					uf::Serializer payload;
					payload["uid"] = member["uid"];
					payload["message"] = hpCost;
					payload["color"] = "FF0000";
					showMessage( payload );
				}
			}
		}

		if ( skillData["turns+"].isNumeric() ) {
			member["used skills"].append(skillId);
			payload["message"] = payload["message"].asString() + "\nGained "+colorString("FF0000") + ""+skillData["turns+"].asString()+""+colorString("FFFFFF") + " additional turns!";
			metadata["battle"]["turn"]["counter"] = metadata["battle"]["turn"]["counter"].asFloat() + skillData["turns+"].asFloat();
		}

		for ( auto& targetId : json["target"] ) {
			uf::Serializer target = metadata["battle"]["transients"][targetId.asString()];

			float power = basePower;
			float morePower = 0;
			float probability = skillData["proc"].isNumeric() ? skillData["proc"].asInt64() / 100.0f : 1.0f;
			// modify probability from statuses
			for ( auto& status : metadata["battle"]["transients"][json["uid"].asString()]["statuses"] ) {
				std::string statusId = status["id"].asString();
				uf::Serializer statusData = masterDataGet("Status", statusId);
				for ( auto it = status["modifier"].begin(); it != status["modifier"].end(); ++it ) {
					statusData[it.key().asString()] = *it;
				}

				if ( !statusData["type"].isNull() && elementData["type"] != statusData["type"] ) continue;
				if ( !statusData["attributes"].isNull() ) {
					bool found = false;
					for ( auto& value : statusData["attributes"] ) {
						if ( value == elementData["id"] ) found = true;
					}
					if ( !found ) continue;
				}

				if ( statusData["proc+"].isNumeric() ) {
					float moreProb = statusData["proc+"].asFloat() / 100.0f;
					probability += moreProb;
					payload["message"] = payload["message"].asString() + "\n"+colorString("FF00FF") + "" + std::to_string((int)(moreProb*100)) + "%"+colorString("FF0000") + " more probability"+colorString("FFFFFF") + "";
				}
			}
			// find attack modifying statuses on attacker
			for ( auto& status : metadata["battle"]["transients"][json["uid"].asString()]["statuses"] ) {
				std::string statusId = status["id"].asString();
				uf::Serializer statusData = masterDataGet("Status", statusId);
				for ( auto it = status["modifier"].begin(); it != status["modifier"].end(); ++it ) {
					statusData[it.key().asString()] = *it;
				}

				if ( !statusData["type"].isNull() && elementData["type"] != statusData["type"] ) continue;
				if ( !statusData["attributes"].isNull() ) {
					bool found = false;
					for ( auto& value : statusData["attributes"] ) {
						if ( value == elementData["id"] ) found = true;
					}
					if ( !found ) continue;
				}
				if ( !statusData["ailments"].isNull() ) continue;


				if ( statusData["power"].isNumeric() ) {
					float adjust = statusData["power"].asFloat() / 100.0f;
					morePower += adjust;
				//	payload["message"] = payload["message"].asString() + "\n"+colorString("FF00FF") + "" + std::to_string((int)(adjust*100)) + "%"+colorString("FF0000") + " more power"+colorString("FFFFFF") + "";
				}
			}
			// find defense modifying statuses on target
			for ( auto& status : metadata["battle"]["transients"][targetId.asString()]["statuses"] ) {
				std::string statusId = status["id"].asString();
				uf::Serializer statusData = masterDataGet("Status", statusId);
				for ( auto it = status["modifier"].begin(); it != status["modifier"].end(); ++it ) {
					statusData[it.key().asString()] = *it;
				}

				if ( !statusData["type"].isNull() && elementData["type"] != statusData["type"] ) continue;
				if ( !statusData["attributes"].isNull() ) {
					bool found = false;
					for ( auto& value : statusData["attributes"] ) {
						if ( value == elementData["id"] ) found = true;
					}
					if ( !found ) continue;
				}

				if ( statusData["defense"].isNumeric() ) {
					float adjust = statusData["defense"].asFloat() / 100.0f;
					morePower -= adjust;
				//	payload["message"] = payload["message"].asString() + "\n"+colorString("FF00FF") + "" + std::to_string((int)(adjust*100)) + "%"+colorString("FF0000") + " more defense"+colorString("FFFFFF") + "";
				}
			}
		//	if ( morePower != 0 ) payload["message"] = payload["message"].asString() + "\n"+colorString("FF00FF") + "" + std::to_string((int)(morePower*100)) + "%"+colorString("FF0000") + " power modifier"+colorString("FFFFFF") + "";

			float r = (rand() % 100) / 100.0;
			// missed
			if ( r > probability ) {
				uf::Serializer payload;
				payload["target"]["uid"] = target["uid"];
				payload["target"]["damage"] = "Miss";
				gui->queueHook("world:Battle.Damage.%UID%", payload);
				continue;
			}

			bool repel = false;
			std::function<void(const std::string&)> affinityCheck = [&]( const std::string& targetId ) {
				auto& target = metadata["battle"]["transients"][targetId];
				uf::Serializer cardData = masterDataGet("Card", target["id"].asString());
				uf::Serializer affinity = cardData["affinity"][elementId];
				// affinity change check
				for ( auto& status : target["statuses"] ) {
					std::string statusId = status["id"].asString();
					uf::Serializer statusData = masterDataGet("Status", statusId);
					for ( auto it = status["modifier"].begin(); it != status["modifier"].end(); ++it ) {
						statusData[it.key().asString()] = *it;
					}

					if ( !statusData["type"].isNull() && elementData["type"] != statusData["type"] ) continue;
					if ( !statusData["attributes"].isNull() ) {
						bool found = false;
						for ( auto& value : statusData["attributes"] ) {
							if ( value == elementData["id"] ) found = true;
						}
						if ( !found ) continue;
					}

					if ( !statusData["affinity"].isNull() ) {
						if ( !statusData["affinity"][elementId].isNull() ) affinity = statusData["affinity"][elementId];
					//	payload["message"] = payload["message"].asString() + "\naffinity modifier."+colorString("FFFFFF") + "";
					}
				}
				if ( !affinity.isNull() ) {
					std::string affinityStr = affinity.asString();
					if ( repel && affinityStr == "repel" ) affinityStr = "null";
					if ( affinityStr == "weak" ) { payload["message"] = payload["message"].asString() + "\n"+colorString("FF0000") + "Weak!\n"; morePower *= 2.0f; }
					else if ( affinityStr == "strong" ) { payload["message"] = payload["message"].asString() + "\n"+colorString("FF0000") + "Strong!\n"; morePower *= 0.5f; }
					else if ( affinityStr == "drain" ) { turnEffects.endTurn = true; payload["message"] = payload["message"].asString() + "\n"+colorString("FF0000") + "Drain!\n"; morePower *= -1.0f; }
					else if ( affinityStr == "null" ) { turnEffects.loseTurn = 2; payload["message"] = payload["message"].asString() + "\n"+colorString("FF0000") + "Nulled!\n"; morePower *= 0; }
					else if ( affinityStr == "repel" ) { turnEffects.endTurn = true; payload["message"] = payload["message"].asString() + "\n"+colorString("FF0000") + "Repelled!\n"; repel = true;
						// check again
						affinityCheck(json["uid"].asString());
					} else {
						affinityStr = "";
					}
					if ( affinityStr != "" ) {
						uf::Serializer payload;
						payload["uid"] = targetId;
						payload["message"] = affinityStr;
						payload["color"] = "FF0000";
						showMessage( payload );
					}
				}
			}; affinityCheck(target["uid"].asString());

			power += morePower;
			
			// damage calc
			float damage = 0.0f; {
				float calcDamage = 0.0f;
				// normal attack
				if ( elementData["type"] == "Physical"  ) {
					if ( skillData["hp%"].isNumeric() ) {
						// max hp * power
						calcDamage = (member["max hp"].asFloat() * skillData["power"].asFloat() * 0.00114f );
					} else {
						// ( lv + str ) * power / 15
						calcDamage = (member["lv"].asFloat() + member["str"].asFloat()) * skillData["power"].asFloat() / 15.0f;
					}
				} else if ( elementData["type"] == "Magic" ) {
					if ( member["lv"].asFloat() < 30 ) {
						calcDamage *= skillData["power"].asFloat() * member["lv"].asFloat() * ( 2 * member["mag"].asFloat() + 70.0f ) / 1000.0f;
					} else if ( member["lv"].asFloat() < 160 ) {
						calcDamage = 3.0f * skillData["power"].asFloat() * ( 2 * member["mag"].asFloat() + 70.0f - ( 0.4f *  member["lv"].asFloat() ) ) / 100.0f;
					} else {
						calcDamage = 3.0f * skillData["power"].asFloat() * ( 2 * member["mag"].asFloat() + 5 ) / 100.0f;
					}
				} else {
					std::cout << elementData << ": " << skillData << std::endl;
				}
				damage = (int) (calcDamage * power);
			}

			if ( skillData["hits"].isArray() ) {
				float low = skillData["hits"][0].asFloat();
				float high = skillData["hits"][1].asFloat();
				int hits = low + r * (high - low);
				damage *= hits;
			}

			// hama/mudo instakill
			bool instakill = false;
			if ( elementId == "12" || elementId == "13" ) {
				instakill = true;
			}

			bool critted = false;
			std::function<void(const std::string&)> applyDamage = [&]( const std::string& targetId ) {
				auto& target = metadata["battle"]["transients"][targetId];
				uf::Serializer cardData = masterDataGet("Card", target["id"].asString());
				uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].asString());
				int curHp = target["hp"].asFloat();
				bool isCrit = ((rand() % 100) / 100.0) < 0.1;
				if ( damage == 0 ) isCrit = false;
				float oldDamage = damage;
				if ( isCrit ) {
					damage *= 1.5;
					critted = true;
				}

				// is a heal
				if ( elementId == "14" ) {
					float power = skillData["power"].asFloat() / 100.0f * (morePower == 0 ? 1 : morePower);
					std::cout << target["max hp"].asFloat() << ", " << power << std::endl;
					damage = -1 * std::abs(target["max hp"].asFloat() * power);
				}

				if ( instakill ) damage = curHp;
				target["hp"] = (int) (curHp -= damage);
				if ( target["hp"].asFloat() > target["max hp"].asFloat() ) target["hp"] = target["max hp"].asFloat();

				std::cout << power << ", " << damage << ", " << morePower << (isCrit ? " / crit" : "") << std::endl;

				{

					uf::Serializer payload;
					payload["uid"] = target["uid"];
					payload["crit"] = isCrit;
					payload["message"] = "";
					if ( damage > 0 ) {
						payload["color"] = "FF0000";
						payload["message"] = (int) damage;
						
						if ( skillData["drains"].asBool() ) {
							float ratio = damage / target["max hp"].asFloat();
							int mpDrain = target["max mp"].asFloat() * ratio;
							if ( target["mp"].asFloat() > mpDrain ) mpDrain = target["mp"].asFloat();
							target["mp"] = target["mp"].asFloat() - mpDrain;
							payload["message"] = payload["message"].asString() + "\n  " + colorString("0000FF") + std::to_string(mpDrain);
							
							showMessage( payload );

							payload["uid"] = member["uid"];
							payload["color"] = "00FF00";
						}
					// heal
					} else if ( damage < 0 ) {
						payload["message"] = (int) -damage;
						payload["color"] = "00FF00";
					}
					showMessage( payload );
				}

				damage = oldDamage;

				if ( curHp <= 0 ) {
					payload["message"] = payload["message"].asString() + "\n"+colorString("FF0000") + ""+ charaData["name"].asString() +" "+colorString("FFFFFF") + "died!";
					target["hp"] = 0;

					uf::Serializer statusPayload;
					statusPayload["id"] = "21";
					target["statuses"] = Json::arrayValue;
					target["statuses"].append(statusPayload);
				}
			}; applyDamage(repel ? json["uid"].asString() : target["uid"].asString());
			// counter statuses
			if ( !repel ) {
				for ( auto& status : metadata["battle"]["transients"][target["uid"].asString()]["statuses"] ) {
					std::string statusId = status["id"].asString();
					if ( statusId != "19" ) continue;
					uf::Serializer statusData = masterDataGet("Status", statusId);
					for ( auto it = status["modifier"].begin(); it != status["modifier"].end(); ++it ) {
						statusData[it.key().asString()] = *it;
					}

					if ( !statusData["type"].isNull() && elementData["type"] != statusData["type"] ) continue;
					if ( !statusData["attributes"].isNull() ) {
						bool found = false;
						for ( auto& value : statusData["attributes"] ) {
							if ( value == elementData["id"] ) found = true;
						}
						if ( !found ) continue;
					}
					if ( statusData["proc"].isNumeric() ) {
						float r = (rand() % 100) / 100.0;
						if ( r <= statusData["proc"].asFloat() / 100.0f ) {
							applyDamage(json["uid"].asString());
							payload["message"] = payload["message"].asString() + "\nCountered!";
						}
					}
				}
			}
			// apply status
			{
				for ( auto& status : skillData["statuses"] ) {
					std::string target = status["target"].asString();
					std::string statusId = status["id"].asString();
					uf::Serializer statusData = masterDataGet("Status", statusId);
					if ( skillData["type"].asInt64() == 16 ) continue;
					float r = (rand() % 100) / 100.0;
					if ( repel ) {
						if ( target == "enemy" ) target = "self";
						else target = "";
					}
					if ( statusData["proc"].isNumeric() ) {
						// failed
						if ( r > statusData["proc"].asFloat() / 100.0f ) {
							target = "";
						}
					}

					uf::Serializer targets;
					if ( target == "self" ) {
						targets.append(json["uid"].asString());
				//	} else if ( target == "enemy" || target == "ally" ) {
					} else {
						targets.append(targetId.asString());
					}
					for ( auto& target : targets ) {
						// remove status(es)
						if ( status["type"] == "remove" ) {
							for ( auto& statusC : metadata["battle"]["transients"][target.asString()]["statuses"] ) {
								if ( statusC["id"] == statusId ) {
									statusC = Json::nullValue;
									statusC["id"] = 0;
									statusC["turns"] = -1;
								}
							}
							continue;
						}

						uf::Serializer statusPayload;
						statusPayload["id"] = statusId;
						statusPayload["modifier"] = status["modifier"];

						statusPayload["turns"] = status["turns"].isNumeric() ? status["turns"] : statusData["turns"];
						metadata["battle"]["transients"][target.asString()]["statuses"].append(statusPayload);

						for ( auto it = status["modifier"].begin(); it != status["modifier"].end(); ++it ) {
							statusData[it.key().asString()] = *it;
						}

						uf::Serializer cardData = masterDataGet( "Card", metadata["battle"]["transients"][target.asString()]["id"].asString() );
						uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].asString());

					//	payload["message"] = payload["message"].asString() + "\nApplied "+colorString("FF0000") + "" + statusData["name"].asString() + ""+colorString("FFFFFF") + " to " + charaData["name"].asString();
						{
							uf::Serializer payload;
							payload["uid"] = targetId;
							payload["message"] = skillData["name"].asString();
							payload["color"] = "9900FF";
							showMessage( payload );
						}
					}
				}
			}

			if ( damage > 0 ) {
				float r = (rand() % 100) / 100.0;
			//	if ( r < 0.5 ) {
				if ( r < 1.0 ) {
					playSound(*this, member["id"].asString(), r > 0.25 ? "attack" : "skill");
				} else {
					playSound(*this, target["id"].asString(), r > 0.75 ? "damagedsmall" : "damagedlarge");
				}
				for ( int i = 0; i < 16; ++i ) {
					uf::Serializer payload;
					payload["uid"] = target["uid"];
					payload["phase"] = i % 2 == 0 ? 0 : 1;
					this->queueHook("world:Battle.OnHit.%UID%", payload, 0.05f * i);
				}
			}
			if ( critted ) {
				uf::Serializer payload;
				payload["uid"] = json["uid"];
				payload["id"] = member["id"];
				gui->queueHook("world:Battle.OnCrit.%UID%", payload, 0.1f);
			}
		}

		/* heal self */
		if ( skillData["add hp"].isNumeric() ) {
			int heal = skillData["add hp"].asFloat() / 100.0f * member["max hp"].asFloat();
			if ( heal + member["hp"].asFloat() >= member["max hp"].asFloat() ) heal = member["max hp"].asFloat() - member["hp"].asFloat();
			member["hp"] = member["hp"].asFloat() + heal;

			uf::Serializer payload;
			payload["uid"] = member["uid"];
			payload["message"] = "";
			payload["color"] = "00FF00";
			payload["message"] = (int) heal;
			showMessage( payload );	
		}

		if ( turnEffects.endTurn ) {
			turnEffects.loseTurn = 99;
		}
		if ( turnEffects.loseTurn > 0 ) {
			auto& turnState = metadata["battle"]["turn"];
			float decrement = turnState["decrement"].asFloat();
			turnState["counter"] = turnState["counter"].asFloat() - turnEffects.loseTurn;
			turnState["decrement"] = 0;
		}

		payload["uid"] = json["uid"];
		payload["target"] = json["target"];
		payload[""]["id"] = member["id"];
		payload[""]["skill"] = skillId;
		payload["timeout"] = 2.0f;
		return payload;
	});
	this->addHook( "world:Battle.Get.%UID%", [&](const std::string& event)->std::string{
		uf::Serializer json = event;
		auto& member = metadata["battle"]["transients"][json["uid"].asString()];
	/*
		ext::Housamo* transient = transients.at(uid);
		uf::Serializer& member = transient->getComponent<uf::Serializer>();
	*/
		std::string message;

		uf::Serializer cardData = masterDataGet("Card", member["id"].asString());
		uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].asString());
		uf::Serializer affinity = cardData["affinity"];
		std::string name = charaData["name"].asString();
		if ( member["type"] == "player" ) message += colorString("00FF00");
		else if ( member["type"] == "enemy" ) message += colorString("FF0000");
		else  message += colorString("AAAAAA");


		message += name + colorString("FFFFFF") + ": " + member["type"].asString();
		
		message += "\n" + colorString("FF0000") + "HP: " + member["hp"].asString();
		message += "\n" + colorString("0000FF") + "MP: " + member["mp"].asString();


		message += "\n" + colorString("9900FF") + "Ailments" + colorString("FFFFFF") + ":";
		for ( auto& status : member["statuses"] ) {
			std::string statusId = status["id"].asString();
			uf::Serializer statusData = masterDataGet("Status", statusId);
			for ( auto it = status["modifier"].begin(); it != status["modifier"].end(); ++it ) {
				statusData[it.key().asString()] = *it;
			}
			if ( statusId == "0" ) continue;
			// affinity change check
			for ( auto it = statusData["affinity"].begin(); it != statusData["affinity"].end(); ++it ) {
				affinity[it.key().asString()] = *it;
			}
			message += "\n\t" + statusData["name"].asString() + (status["turns"].isNull() ? "" : " (Turns: " + status["turns"].asString() + ")" );
		}

		message += "\n" + colorString("9900FF") + "Affinities" + colorString("FFFFFF") + ":";

		for ( auto it = affinity.begin(); it != affinity.end(); ++it ) {
			std::string elementId = it.key().asString();
			std::string value = it->asString();
			uf::Serializer elementData = masterDataGet("Element", elementId);
			message += "\n\t" + elementData["name"].asString() + ": " + value;
		}

		uf::Serializer payload;
		payload[""] = member;
	//	payload["timeout"] = 0.1f;
		payload["message"] = message;
		return payload;
	});	
	this->addHook( "world:Battle.End.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		// play music
	/*
		std::cout << "Previous BGM: " << previousMusic << std::endl;
		playMusic(*this, previousMusic);
	*/
		uf::Scene& scene = uf::scene::getCurrentScene();
		scene.callHook("world:Music.LoadPrevious.%UID%");
		gui->callHook("menu:Close.%UID%");
	
		// cleanup
		for ( ext::Housamo* pointer : transients ) {
			pointer->destroy();
			this->removeChild(*pointer);
			delete pointer;
		}
		transients.clear();

		if ( json["won"].asBool() ) {
			// play death sound
		//	playSound(*this, metadata["battle"]["enemy"]["uid"].asUInt64(), "killed");
			// kill
			{
				uf::Scene& scene = uf::scene::getCurrentScene();
				std::size_t uid = metadata["battle"]["enemy"]["uid"].asUInt64();
				uf::Entity*  = scene.findByUid(uid);
				if (  ) {
					uf::Object& parent = ->getParent<uf::Object>();
					->destroy();
					parent.removeChild(*);
				}
			}
		}

		{
			uf::Serializer payload;
			payload["battle"] = metadata["battle"];
			this->callHook("world:Battle.End", payload);
		}

		uf::Serializer payload;
		payload["end"] = true;
		return payload;
	/*
		if ( json["action"].asString() == "killed" ) {
			// play death sound
			playSound(*this, metadata["battle"]["enemy"]["uid"].asUInt64(), "killed");
			// kill
			{
				uf::Scene& scene = uf::scene::getCurrentScene();
				std::size_t uid = metadata["battle"]["enemy"]["uid"].asUInt64();
				uf::Entity*  = scene.findByUid(uid);
				if (  ) {
					uf::Object& parent = ->getParent<uf::Object>();
					->destroy();
					parent.removeChild(*);
				}
			}
		}

		// clear battle data on player
		{
			uf::Scene& scene = uf::scene::getCurrentScene();
			std::size_t uid = metadata["battle"]["player"]["uid"].asUInt64();
			uf::Entity*  = scene.findByUid(uid);
			if (  ) {
				uf::Serializer& pMetadata = ->getComponent<uf::Serializer>();
				pMetadata["system"].removeMember("battle");
			}
		}

		uf::Serializer payload;
		payload["end"] = true;
		return payload;
		return "true";
	*/
	});
}

void ext::HousamoBattle::tick() {	
	uf::Object::tick();
}

void ext::HousamoBattle::destroy() {
	uf::Object::destroy();
}
void ext::HousamoBattle::render() {	
	uf::Object::render();
}