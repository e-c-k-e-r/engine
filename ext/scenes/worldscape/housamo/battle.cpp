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
		uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].as<std::string>());
		std::string name = charaData["filename"].as<std::string>();

		if ( name == "none" ) return;

		std::string url = "https://cdn..xyz//unity/Android/voice/voice_" + name + "_"+key+".ogg";
		if ( charaData["internal"].as<bool>() ) {
			url = uf::io::root+"/smtsamo/voice/voice_" + name + "_" + key + ".ogg";
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
		std::string id = metadata[""]["id"].as<std::string>();
		playSound( entity, id, key );
	}
	void playSound( ext::HousamoBattle& entity, const std::string& key ) {
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& metadata = entity.getComponent<uf::Serializer>();
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();

		std::string url = uf::io::root+"/audio/ui/" + key + ".ogg";

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

	::gui = NULL;
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
		std::string filename = "";
		if ( ext::json::isArray( json["music"] ) ) {
			int ri = floor(rand() % json["music"].size());
			filename = json["music"][ri].as<std::string>();
		} else if ( json["music"].is<std::string>() ) {
			filename = json["music"].as<std::string>();
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
	
		uf::Serializer actions; actions.readFromFile(uf::io::root+"/entities/gui/battle/actions.json");
		metadata["actions"] = actions;
		if ( ext::json::isNull( metadata["actions"] ) ) {
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
	//	for ( auto& id : metadata["battle"]["player"]["party"] ) {
		ext::json::forEach(metadata["battle"]["player"]["party"], [&](ext::json::Value& id){
			uint64_t uid = transients.size();
			auto& member = metadata["battle"]["player"]["transients"][id.as<std::string>()];
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
		});
	//	for ( auto& id : metadata["battle"]["enemy"]["party"] ) {
		ext::json::forEach(metadata["battle"]["enemy"]["party"], [&](ext::json::Value& id){
			uint64_t uid = transients.size();
			auto& member = metadata["battle"]["enemy"]["transients"][id.as<std::string>()];
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
		});

		this->queueHook("world:Battle.Gui.%UID%");

		uf::Serializer payload;
	//	for ( auto& member : metadata["battle"]["transients"] ) {
		ext::json::forEach(metadata["skills"], [&](ext::json::Value& member){
		//	for ( auto& skillId : member["skills"]  ) {
			ext::json::forEach(member["skills"], [&](ext::json::Value& skillId){
				uf::Serializer skillData = masterDataGet("Skill", skillId.as<std::string>());
				if ( skillData["type"].as<int>() != 16 ) return;
			//	for ( auto& status : skillData["statuses"] ) {
				ext::json::forEach(skillData["status"], [&](ext::json::Value& status){
					std::string target = status["target"].as<std::string>();
					std::string statusId = status["id"].as<std::string>();
					uf::Serializer statusData = masterDataGet("Status", statusId);

					float r = (rand() % 100) / 100.0;
					if ( statusData["proc"].is<double>() ) {
						// failed
						if ( r > statusData["proc"].as<float>() / 100.0f ) {
							target = "";
						}
					}
					// apply status
					uf::Serializer targets;								
					if ( target == "self" ) {
						targets.emplace_back(member["uid"].as<std::string>());
					} else if ( target == "enemy" ) {
					//	for ( auto& tmember :  metadata["battle"]["transients"] ) {
						ext::json::forEach(metadata["battle"]["transients"], [&](ext::json::Value& tmember){
							if ( tmember["type"] == member["type"] ) return;
							targets.emplace_back(tmember["uid"].as<std::string>());
						});
					} else if ( target == "ally" ) {
					//	for ( auto& tmember :  metadata["battle"]["transients"] ) {
						ext::json::forEach(metadata["battle"]["transients"], [&](ext::json::Value& tmember){
							if ( tmember["type"] != member["type"] ) return;
							targets.emplace_back(tmember["uid"].as<std::string>());
						});
					}
				//	for ( auto& target : targets ) {
					ext::json::forEach(targets, [&](ext::json::Value& target){
						uf::Serializer statusPayload;
						statusPayload["id"] = statusId;
						statusPayload["modifier"] = status["modifier"];

						statusPayload["turns"] = status["turns"].is<double>() ? status["turns"] : statusData["turns"];
						metadata["battle"]["transients"][target.as<std::string>()]["statuses"].emplace_back(statusPayload);

						uf::Serializer cardData = masterDataGet( "Card", metadata["battle"]["transients"][target.as<std::string>()]["id"].as<std::string>() );
						uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].as<std::string>());

						payload["message"] = payload["message"].as<std::string>() + "\nApplied "+colorString("FF0000") + "" + statusData["name"].as<std::string>() + ""+colorString("FFFFFF") + " to " + charaData["name"].as<std::string>();
					});
				});
			});
		});
		
		payload["battle"] = metadata["battle"];
		return payload;
	});
	this->addHook( "world:Battle.Gui.%UID%", [&](const std::string& event)->std::string{	
		ext::Gui* guiManager = (ext::Gui*) this->globalFindByName("Gui Manager");
		ext::Gui* guiMenu = new ext::GuiBattle;
		guiManager->addChild(*guiMenu);
		guiMenu->as<uf::Object>().load("./scenes/world/gui/battle/menu.json");
		guiMenu->initialize();

		uf::Serializer payload;
		payload["battle"] = metadata["battle"];
		guiMenu->queueHook("world:Battle.Start.%UID%", payload);

		::gui = guiMenu;

		return "true";
	});
	// json.action and json.uid
	this->addHook( "world:Battle.Action.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		
		std::string hookName = "";
		uf::Serializer payload;

		std::string action = json["action"].as<std::string>();
		
		// dead check
		struct {
			struct {
				int dead = 0;
				int total = 0;
			} player, enemy;
		} counts;
	//	for ( auto& member : metadata["battle"]["transients"] ) {
		ext::json::forEach(metadata["battle"]["transients"], [&](ext::json::Value& member){
			bool dead = member["hp"].as<int>() <= 0;
			if ( member["type"] == "player" ) {
				++counts.player.total;
				if ( dead ) ++counts.player.dead;
			} else {
				++counts.enemy.total;
				if ( dead ) ++counts.enemy.dead;
			}
		});
		
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
			payload = this->callHook("world:Battle.AI.%UID%", json)[0].as<uf::Serializer>();
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
			float decrement = turnState["decrement"].as<float>();
			turnState["counter"] = turnState["counter"].as<float>() - decrement;
			turnState["decrement"] = 0;
		}

		{
		//	for ( auto it = metadata["battle"]["transients"].begin(); it != metadata["battle"]["transients"].end(); ++it ) {
		//		std::string key = it.key();
			ext::json::forEach(metadata["battle"]["transients"], [&](const std::string& key, ext::json::Value& member){
				if ( key == "" || ext::json::isNull( member["type"] ) ) metadata["battle"].erase(key);
			});
		}

		/* remove dead */ if ( false ) {
		//	for ( auto it = metadata["battle"]["transients"].begin(); it != metadata["battle"]["transients"].end(); ++it ) {
		//		std::string key = it.key();
			ext::json::forEach(metadata["battle"]["transients"], [&](const std::string& key, ext::json::Value& member){
				if ( metadata["battle"]["transients"][key]["hp"].as<int>() > 0 ) return;
			//	metadata["battle"]["transients"].erase(key);
			});
			// std::cout << metadata["battle"]["transients"] << std::endl;
		}

		if ( ::gui ) {
			uf::Serializer payload;
			payload["battle"] = metadata["battle"];
			::gui->queueHook("world:Battle.Update.%UID%", payload);
		}

		if ( hookName == "" ) return payload;

		uf::Serializer result;
		result = this->callHook(hookName, payload)[0].as<uf::Serializer>();
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
		auto& member = metadata["battle"]["transients"][json["uid"].as<std::string>()];
		// deduce member state
		struct {
			std::vector<std::string> ailments;
			int ndas = 0;
			float factor = 0.0f;
		} stats;
	//	for ( auto& status : member["statuses"] ) {
		ext::json::forEach(metadata["statuses"], [&](ext::json::Value& status){
			std::string id = status["id"].as<std::string>();
			if ( id == "0" ) return;
			stats.ailments.push_back(id);
			if ( id == "6" ) ++stats.ndas;
		});

		uf::Serializer possibilities;
		possibilities = ext::json::array();
	//	for ( auto& skillId : member["skills"] ) {
		ext::json::forEach(metadata["skills"], [&](ext::json::Value& skillId, bool breaks){
			uf::Serializer skillData = masterDataGet("Skill", skillId.as<std::string>());
			// not recarms
			std::cout << skillData["name"] << ": ";
			if ( skillId == "81" || skillId == "82" ) {
				std::cout << "\tBad skill ID" << std::endl;
				return;
			}
			// invalid skill type
			if ( skillData["type"].as<int>() < 1 || skillData["type"].as<int>() > 15 ) {
				std::cout << "\tBad skill type" << std::endl;
				return;
			}
			// no MP
			if ( skillData["mp"].as<float>() > member["mp"].as<float>() ) {
				std::cout << "\tNot enough MP" << std::endl;
				return;
			}
			// no HP
			if ( skillData["hp%"].as<float>() / 100.0f * member["max hp"].as<float>() >= member["hp"].as<float>() ) {
				std::cout << "\tNot enough HP" << std::endl;
				return;
			}
			// do not use same skills
			if ( std::find(member["used skills"].begin(), member["used skills"].end(), skillId.as<std::string>()) != member["used skills"].end() ) {
				std::cout << "\tRecently Used" << std::endl;
				return;
			}
			// grants additional turns
			if ( skillData["turns+"].is<double>() ) {
				queuedSkillId = skillId.as<std::string>();
				std::cout << "\tCan gain turns" << std::endl;
				breaks = true;
				return;
			// try and heal if under 30%
			} else if ( member["hp"].as<float>() / member["max hp"].as<float>() < 0.3f ) {
				if ( skillData["type"] != "14" ) return;
				if ( stats.factor < skillData["power"].as<float>() ) {
					queuedSkillId = skillId.as<std::string>();
					stats.factor = skillData["power"].as<float>();
					std::cout << "\tCan heal" << std::endl;
				}
			// remove debuffs
			} else if ( skillId == "200" ) {
				if ( stats.ndas * 75.0f > stats.factor ) {
					queuedSkillId = skillId.as<std::string>();
					stats.factor = stats.ndas * 75.0f;
					std::cout << "\tCan Dekunda" << std::endl;
				}
				return;
			} else {
				// remove negative ailment (fear, etc)
			//	for ( auto& status : skillData["statuses"] ) {
				ext::json::forEach(skillData["statuses"], [&](ext::json::Value& status){
					std::string statusId = status["id"].as<std::string>();
					uf::Serializer statusData = masterDataGet("Status", statusId);
					for ( auto it = status["modifier"].begin(); it != status["modifier"].end(); ++it ) {
						statusData[it.key()] = *it;
					}
					// remove statuses
					if ( statusData["type"] == "remove" && std::find( stats.ailments.begin(), stats.ailments.end(), statusId ) != stats.ailments.end() ) {
						queuedSkillId = skillId.as<std::string>();
						stats.factor = 150.0f;
						std::cout << "\tCan remove status " << statusData["name"] << std::endl;
					}
				});
				std::cout << "\tRandom" << std::endl;
				// no heal spells
				if ( skillData["type"] == "14" ) return;
				// let RNG pick
				possibilities.emplace_back(skillId);
			}
		});
		// let RNG pick
		std::cout << queuedSkillId << ": " << possibilities << std::endl;
		if ( queuedSkillId == "0" && possibilities.size() > 0 ) {
			possibilities.emplace_back("0");
			int ri = floor(rand() % possibilities.size());
			queuedSkillId = possibilities[ri].as<std::string>();
			auto queuedSkillData = masterDataGet( "Skill", queuedSkillId );
			std::cout << "Picked " << queuedSkillData["name"].as<std::string>() << std::endl;
		}
		json["skill"] = queuedSkillId;
		return json;
	});
	this->addHook( "world:Battle.GetTargets.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		uf::Serializer payload;

		payload["targets"] = ext::json::array();
		auto& member = metadata["battle"]["transients"][json["uid"].as<std::string>()];
		if ( json["skill"].is<std::string>() ) {
			std::string skillId = json["skill"].as<std::string>();
			if ( json["skill"] == "0" ) {
			//	for ( auto& status : member["statuses"] ) {
				ext::json::forEach(member["statuses"], [&](ext::json::Value& status){
					std::string statusId = status["id"].as<std::string>();
					uf::Serializer statusData = masterDataGet("Status", statusId);
					for ( auto it = status["modifier"].begin(); it != status["modifier"].end(); ++it ) {
						statusData[it.key()] = *it;
					}
					if ( statusData["skill"] != skillId ) return;
					if ( statusData["range"].is<double>() ) json["range"] = statusData["range"];
				});
			} else if ( !json["type"].is<std::string>() ) {
				uf::Serializer skillData = masterDataGet( "Skill", skillId );
				json["type"] = skillData["target"].as<std::string>();
				if ( json["type"] == "" ) json["type"] = "enemy";
			}
		} else {
			json["type"] = "all";
		}

		if ( json["type"].is<std::string>() ) {
			if ( json["type"] == "self" ) {
				payload["targets"].emplace_back( metadata["battle"]["transients"][json["uid"].as<std::string>()] );
			} else {
				for ( auto& m : metadata["battle"]["transients"] ) {
					if ( json["type"] == "ally" && m["type"] != member["type"] ) continue;
					if ( json["type"] == "enemy" && m["type"] == member["type"] ) continue;
					payload["targets"].emplace_back( m );
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

	//	for ( auto& member : metadata["battle"]["transients"] ) {
		ext::json::forEach(metadata["battle"]["transients"], [&](ext::json::Value& member, bool breaks){
			if ( member["type"] != "enemy" ) return;
			target = (ext::json::Value&) member;
			size_t uid = member["uid"].as<size_t>();
			uf::Serializer cardData = masterDataGet("Card", member["id"].as<std::string>());
			uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].as<std::string>());
			name = charaData["name"].as<std::string>();
			breaks = true;
		});
		if ( target ) {
			playSound(*this, target["id"].as<std::string>(), r > 0.5 ? "turn" : "battlestart");
			payload["message"] = ""+colorString("FF0000") + "" + name + ""+colorString("FFFFFF") + " speaks.";
		}

		return payload;
	});
	this->addHook( "world:Battle.OnHit.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		uint64_t uid = json["uid"].as<size_t>();
		ext::Housamo* transient = transients.at(uid);
		uf::Serializer& member = transient->getComponent<uf::Serializer>();

		uint64_t entid = metadata["battle"]["enemy"]["uid"].as<size_t>();
		uf::Entity*  = scene.findByUid(entid);
		if ( ! ) return "false";
		uf::Serializer& hMetadata = ->getComponent<uf::Serializer>();
		uint64_t phase = json["phase"].as<size_t>();
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
		uint64_t turnPlayer = json["uid"].as<size_t>();
		std::string name = "";
		auto& turnState = metadata["battle"]["turn"];
		{
			if ( turnState["counter"].as<float>() <= 0 ) {
				if ( turnState["phase"] == "player" ) turnState["phase"] = "enemy";
				else turnState["phase"] = "player";
				int counter = 0;
			//	for ( auto& member : metadata["battle"]["transients"] ) {
				ext::json::forEach(metadata["battle"]["transients"], [&](ext::json::Value& member){
					if ( member["type"] != turnState["phase"] ) return;

					bool locked = false;
				//	for ( auto& status : member["statuses"] ) {
					ext::json::forEach(member["statuses"], [&](ext::json::Value& status){
						std::string statusId = status["id"].as<std::string>();
						uf::Serializer statusData = masterDataGet("Status", statusId);
						for ( auto it = status["modifier"].begin(); it != status["modifier"].end(); ++it ) {
							statusData[it.key()] = *it;
						}

						if ( statusData["lock"].as<bool>() ) locked = true;
					});
					if ( locked ) return;

					++counter;
				});
				turnState["counter"] = counter;
				turnState["start of turn"] = true;

				if ( ::gui ) {
					uf::Serializer payload;
					payload["battle"] = metadata["battle"];
					::gui->queueHook("world:Battle.TurnStart.%UID%", payload);
				}
			} else {
				turnState["start of turn"] = false;
			}
		}

		std::function<bool()> getTurnPlayer = [&]()->bool{
		//	for ( auto& member : metadata["battle"]["transients"] ) {
			bool found = false;
			ext::json::forEach(metadata["battle"]["transients"], [&](ext::json::Value& member, bool breaks){
				if ( member["hp"].as<int>() <= 0 ) return;
			
				bool locked = false;
			//	for ( auto& status : member["statuses"] ) {
				ext::json::forEach(member["statuses"], [&](ext::json::Value& status){
					std::string statusId = status["id"].as<std::string>();
					uf::Serializer statusData = masterDataGet("Status", statusId);
					for ( auto it = status["modifier"].begin(); it != status["modifier"].end(); ++it ) {
						statusData[it.key()] = *it;
					}

					if ( statusData["lock"].as<bool>() ) locked = true;
				});
				if ( locked ) return;
			
				if ( turnPlayer > member["uid"].as<size_t>() ) return;
				if ( member["type"] != turnState["phase"] ) return;
				turnPlayer = member["uid"].as<size_t>();
				breaks = true;
				found = true;
			});
			return found;
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
		auto& member = metadata["battle"]["transients"][payload["uid"].as<std::string>()];
		uf::Serializer cardData = masterDataGet("Card", member["id"].as<std::string>());
		uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].as<std::string>());
		name = charaData["name"].as<std::string>();
		payload["message"] = "What will "+colorString("FF0000") + "" + name + ""+colorString("FFFFFF") + " do?";

		// 
		if ( member["type"] == "enemy" ) {
			payload["message"] = ""+colorString("FF0000") + "" + name + ""+colorString("FFFFFF") + "'s turn...";
			payload["timeout"] = 2.0f;
		}

		bool skipped = false;
		// tick down statuses
		if ( turnState["start of turn"].as<bool>() ) {
			/* remove dead */ if ( false ) {
				for ( auto it = metadata["battle"]["transients"].begin(); it != metadata["battle"]["transients"].end(); ++it ) {
					std::string key = it.key();
					if ( metadata["battle"]["transients"][key]["hp"].as<int>() > 0 ) continue;
				//	metadata["battle"]["transients"].erase(key);
				}
			//	std::cout << metadata["battle"]["transients"] << std::endl;
			}
		//	for ( auto& member : metadata["battle"]["transients"] ) {
			ext::json::forEach(metadata["battle"]["transients"], [&](ext::json::Value& member){
				// clear OPT used skills
				member["used skills"] = ext::json::array();
				if ( member["type"] != turnState["phase"] ) return;
				auto& statuses = member["statuses"];
//				std::cout << "Before: " << member["uid"].as<std::string>() << ": " << statuses << std::endl;
			//	for ( int i = 0; i < statuses.size(); ++i ) {
				ext::json::forEach(statuses, [&](size_t i, ext::json::Value& status){
					uf::Serializer statusData = masterDataGet("Status", status["id"].as<std::string>());
					for ( auto it = status["modifier"].begin(); it != status["modifier"].end(); ++it ) {
						statusData[it.key()] = *it;
					}
					
					uf::Serializer cardData = masterDataGet( "Card", member["id"].as<std::string>() );
					uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].as<std::string>());
					
					if ( statusData["hp"].is<double>() ) {
						int64_t hp = statusData["hp"].as<int>();
						member["hp"] = member["hp"].as<int>() + hp;
						payload["message"] = payload["message"].as<std::string>() + "\n"+colorString("FF0000") + "" + charaData["name"].as<std::string>() +  ""+colorString("FFFFFF") + " gained "+colorString("00FF00") + ""+ std::to_string(hp) +" HP"+colorString("FFFFFF") + " from "+colorString("FF0000") + "" + statusData["name"].as<std::string>() + ""+colorString("FFFFFF") + "";
					}
					if ( statusData["hp%"].is<double>() ) {
						int64_t hp = member["max hp"].as<float>() * (statusData["hp%"].as<int>() / 100.0f);
						if ( member["max hp"].as<int>() < member["hp"].as<int>() + hp ) hp = member["max hp"].as<int>() - member["hp"].as<int>();
						member["hp"] = member["hp"].as<int>() + hp;
						if ( hp > 0 ) payload["message"] = payload["message"].as<std::string>() + "\n"+colorString("FF0000") + "" + charaData["name"].as<std::string>() +  ""+colorString("FFFFFF") + " gained "+colorString("00FF00") + ""+ std::to_string(hp) +" HP"+colorString("FFFFFF") + " from "+colorString("FF0000") + "" + statusData["name"].as<std::string>() + ""+colorString("FFFFFF") + "";
					}
					if ( statusData["mp"].is<double>() ) {
						int64_t mp = statusData["mp"].as<int>();
						member["mp"] = member["mp"].as<int>() + mp;
						if ( member["max mp"].as<int>() < member["mp"].as<int>() + mp ) mp = member["max mp"].as<int>() - member["mp"].as<int>();
						payload["message"] = payload["message"].as<std::string>() + "\n"+colorString("FF0000") + "" + charaData["name"].as<std::string>() +  ""+colorString("FFFFFF") + " gained "+colorString("0000FF") + ""+ std::to_string(mp) +" MP"+colorString("FFFFFF") + " from "+colorString("FF0000") + "" + statusData["name"].as<std::string>() + ""+colorString("FFFFFF") + "";
					}
					if ( statusData["lock"].is<double>() ) {
						skipped = true;
					}
				});
				uf::Serializer newStatuses;
				for ( int i = statuses.size() - 1; i >= 0; --i ) {
					auto& status = statuses[i];
					uf::Serializer statusData = masterDataGet("Status", status["id"].as<std::string>());
					if ( !ext::json::isNull( status["turns"] ) ) {
						status["turns"] = status["turns"].as<int>() - 1;
						if ( status["turns"].as<int>() < 0 ) {
							if ( statusData["id"] != "0" ) payload["message"] = payload["message"].as<std::string>() + "\n"+colorString("FF0000") + "" + statusData["name"].as<std::string>() + ""+colorString("FFFFFF") + " wore off.";
						} else {
							newStatuses.emplace_back(status);
						}
					} else {
						newStatuses.emplace_back(status);
					}
				}
				statuses = newStatuses;
//				std::cout << "After: " << member["uid"].as<std::string>() << ": " << statuses << std::endl;
			});
		}

		{
			for ( auto it = metadata["battle"]["transients"].begin(); it != metadata["battle"]["transients"].end(); ++it ) {
				std::string key = it.key();
				if ( key == "" || ext::json::isNull( (*it)["type"] ) ) metadata["battle"].erase(key);
			}
		}

		if ( ::gui ) {
			uf::Serializer payload;
			payload["battle"] = metadata["battle"];
			::gui->queueHook("world:Battle.Update.%UID%", payload);
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
		payload["actions"]["list"] = ext::json::array();
		payload["actions"]["names"]["member-attack"] = "攻撃";
		payload["actions"]["names"]["member-skill"] = "スキル";
		payload["actions"]["names"]["inventory"] = "アイテム";
		payload["actions"]["names"]["talk"] = "話し掛ける";
		payload["actions"]["names"]["transients"] = "転光生";
		payload["actions"]["names"]["escape"] = "脱走";
		payload["actions"]["names"]["analyze"] = "Analyze";
		payload["actions"]["names"]["pass"] = "Pass";
		for ( auto& x : list ) payload["actions"]["list"].emplace_back(x);
	*/
		payload["actions"] = metadata["actions"];
		payload[""] = member;
		return payload;
	});
	this->addHook( "world:Battle.Attack.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;

		std::string skillId = json["skill"].as<std::string>();
		uf::Serializer skillData = masterDataGet("Skill", skillId);

//		ext::Housamo* transient = transients.at(uid);
		auto& member = metadata["battle"]["transients"][json["uid"].as<std::string>()];

		uf::Serializer payload;
		payload["message"] = "";
		// modify skill data based on statuses
	//	for ( auto& status : member["statuses"] ) {
		ext::json::forEach(member["statuses"], [&](ext::json::Value& status){
			std::string statusId = status["id"].as<std::string>();
			uf::Serializer statusData = masterDataGet("Status", statusId);
			for ( auto it = status["modifier"].begin(); it != status["modifier"].end(); ++it ) {
				statusData[it.key()] = *it;
			}
			if ( statusData["skill"] != skillId ) return;
			if ( statusData["range"].is<double>() ) skillData["range"] = statusData["range"];
		});
		// select random target
		if ( ext::json::isNull(  json["target"] ) ) {
			if ( ext::json::isNull(  skillData["target"] ) ) skillData["target"] = "enemy";
			if ( skillData["target"] == "self" ) {
				json["target"].emplace_back( member["uid"].as<size_t>() );
			} else if ( skillData["range"].is<double>() && skillData["range"].as<int>() > 1 ) {
				json["target"] = ext::json::array();
			//	for ( auto& m : metadata["battle"]["transients"] ) {
				ext::json::forEach(member["battle"]["transients"], [&](ext::json::Value& m){
					if ( skillData["target"] == "ally" && m["type"] != member["type"] ) return;
					if ( skillData["target"] != "ally" && m["type"] == member["type"] ) return;
					json["target"].emplace_back( m["uid"].as<size_t>() );
				});
			} else {
				std::vector<uint64_t> targets;
			//	for ( auto& m : metadata["battle"]["transients"] ) {
				ext::json::forEach(member["battle"]["transients"], [&](ext::json::Value& m){
					if ( skillData["target"] == "ally" && m["type"] != member["type"] ) return;
					if ( skillData["target"] != "ally" && m["type"] == member["type"] ) return;
					targets.push_back( m["uid"].as<size_t>() );
				});
				int ri = floor(rand() % targets.size());
				json["target"] = ext::json::array();
				json["target"].emplace_back(targets.at(ri));
			}
		}

		uf::Serializer elementData;
		uf::Serializer cardData = masterDataGet("Card", member["id"].as<std::string>());
		uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].as<std::string>());
		std::string elementId;
		
		float basePower = member["type"] == "enemy" ? 1.2f : 1.0f;

		if ( skillId != "0" ) {
			payload["message"] = payload["message"].as<std::string>() + "\n"+colorString("FF0000") + "" + charaData["name"].as<std::string>() + ""+colorString("FFFFFF") + ": "+colorString("7777FF") + "" + skillData["name"].as<std::string>() + colorString("FFFFFF");

			elementId = skillData["type"].as<std::string>();
			elementData = masterDataGet("Element", elementId);
		} else {
			payload["message"] = payload["message"].as<std::string>() + "\n"+colorString("FF0000") + "" + charaData["name"].as<std::string>() + ""+colorString("FFFFFF") + ": "+colorString("7777FF") + "攻撃" + colorString("FFFFFF");

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
			::gui->queueHook("world:Battle.Damage.%UID%", payload, delays.damage);
			delays.damage += 0.25f;
		};
		
		{
			int64_t mp = member["mp"].as<int>();
			int64_t hp = member["hp"].as<int>();
			int64_t mpCost = skillData["mp"].as<int>();
			int64_t hpCost = skillData["hp%"].as<int>();

		//	for ( auto& status : member["statuses"] ) {
			ext::json::forEach(member["statuses"], [&](ext::json::Value& status){
				std::string statusId = status["id"].as<std::string>();
				uf::Serializer statusData = masterDataGet("Status", statusId);
				for ( auto it = status["modifier"].begin(); it != status["modifier"].end(); ++it ) {
					statusData[it.key()] = *it;
				}

				if ( !ext::json::isNull( statusData["type"] ) && elementData["type"] != statusData["type"] ) return;
				if ( !ext::json::isNull( statusData["attributes"] ) ) {
					bool found = false;
					for ( auto& value : statusData["attributes"] ) {
						if ( value == elementData["id"] ) found = true;
					}
					if ( !found ) return;
				}

				if ( hpCost > 0 && statusData["cost"].is<double>() ) {
					hpCost -= statusData["cost"].as<int>();
				}
			});

			hpCost = member["max hp"].as<float>() * ( hpCost / 100.0f );

			if ( mp < mpCost ) {
				payload["message"] = "Not enough MP to use `"+colorString("FF0000") + ""+skillData["name"].as<std::string>()+""+colorString("FFFFFF") + "`.";
				payload["timeout"] = 2.0f;
				if ( member["type"] != "enemy" ) {
					payload["invalid"] = true;
				}
				return payload;
			} else if ( mpCost > 0 ) {
				metadata["battle"]["transients"][member["uid"].as<std::string>()]["mp"] = mp - mpCost;
			}
			if ( hp < hpCost ) {
				payload["message"] = "Not enough HP to use `"+colorString("FF0000") + ""+skillData["name"].as<std::string>()+""+colorString("FFFFFF") + "`.";
				payload["timeout"] = 2.0f;
				if ( member["type"] != "enemy" ) {
					payload["invalid"] = true;
					return payload;
				} else {
					skillId = "0";
					skillData = masterDataGet("Skill", skillId);
				}
			} else if ( hpCost > 0 ) {
				metadata["battle"]["transients"][member["uid"].as<std::string>()]["hp"] = hp - hpCost;
				{
					uf::Serializer payload;
					payload["uid"] = member["uid"];
					payload["message"] = hpCost;
					payload["color"] = "FF0000";
					showMessage( payload );
				}
			}
		}

		if ( skillData["turns+"].is<double>() ) {
			member["used skills"].emplace_back(skillId);
			payload["message"] = payload["message"].as<std::string>() + "\nGained "+colorString("FF0000") + ""+skillData["turns+"].as<std::string>()+""+colorString("FFFFFF") + " additional turns!";
			metadata["battle"]["turn"]["counter"] = metadata["battle"]["turn"]["counter"].as<float>() + skillData["turns+"].as<float>();
		}

	//	for ( auto& targetId : json["target"] ) {
		ext::json::forEach(json["target"], [&](ext::json::Value& targetId){
			uf::Serializer target = metadata["battle"]["transients"][targetId.as<std::string>()];

			float power = basePower;
			float morePower = 0;
			float probability = skillData["proc"].is<double>() ? skillData["proc"].as<int>() / 100.0f : 1.0f;
			// modify probability from statuses
		//	for ( auto& status : metadata["battle"]["transients"][json["uid"].as<std::string>()]["statuses"] ) {
			ext::json::forEach(metadata["battle"]["transients"][json["uid"].as<std::string>()]["statuses"], [&](ext::json::Value& status){
				std::string statusId = status["id"].as<std::string>();
				uf::Serializer statusData = masterDataGet("Status", statusId);
				for ( auto it = status["modifier"].begin(); it != status["modifier"].end(); ++it ) {
					statusData[it.key()] = *it;
				}

				if ( !ext::json::isNull( statusData["type"] ) && elementData["type"] != statusData["type"] ) return;
				if ( !ext::json::isNull( statusData["attributes"] ) ) {
					bool found = false;
					for ( auto& value : statusData["attributes"] ) {
						if ( value == elementData["id"] ) found = true;
					}
					if ( !found ) return;
				}

				if ( statusData["proc+"].is<double>() ) {
					float moreProb = statusData["proc+"].as<float>() / 100.0f;
					probability += moreProb;
					payload["message"] = payload["message"].as<std::string>() + "\n"+colorString("FF00FF") + "" + std::to_string((int)(moreProb*100)) + "%"+colorString("FF0000") + " more probability"+colorString("FFFFFF") + "";
				}
			});
			// find attack modifying statuses on attacker
		//	for ( auto& status : metadata["battle"]["transients"][json["uid"].as<std::string>()]["statuses"] ) {
			ext::json::forEach(metadata["battle"]["transients"][json["uid"].as<std::string>()]["statuses"], [&](ext::json::Value& status){
				std::string statusId = status["id"].as<std::string>();
				uf::Serializer statusData = masterDataGet("Status", statusId);
				for ( auto it = status["modifier"].begin(); it != status["modifier"].end(); ++it ) {
					statusData[it.key()] = *it;
				}

				if ( !ext::json::isNull( statusData["type"] ) && elementData["type"] != statusData["type"] ) return;
				if ( !ext::json::isNull( statusData["attributes"] ) ) {
					bool found = false;
					for ( auto& value : statusData["attributes"] ) {
						if ( value == elementData["id"] ) found = true;
					}
					if ( !found ) return;
				}
				if ( !ext::json::isNull( statusData["ailments"] ) ) return;


				if ( statusData["power"].is<double>() ) {
					float adjust = statusData["power"].as<float>() / 100.0f;
					morePower += adjust;
				//	payload["message"] = payload["message"].as<std::string>() + "\n"+colorString("FF00FF") + "" + std::to_string((int)(adjust*100)) + "%"+colorString("FF0000") + " more power"+colorString("FFFFFF") + "";
				}
			});
			// find defense modifying statuses on target
		//	for ( auto& status : metadata["battle"]["transients"][targetId.as<std::string>()]["statuses"] ) {
			ext::json::forEach(metadata["battle"]["transients"][targetId.as<std::string>()]["statuses"], [&](ext::json::Value& status){
				std::string statusId = status["id"].as<std::string>();
				uf::Serializer statusData = masterDataGet("Status", statusId);
				for ( auto it = status["modifier"].begin(); it != status["modifier"].end(); ++it ) {
					statusData[it.key()] = *it;
				}

				if ( !ext::json::isNull( statusData["type"] ) && elementData["type"] != statusData["type"] ) return;
				if ( !ext::json::isNull( statusData["attributes"] ) ) {
					bool found = false;
					for ( auto& value : statusData["attributes"] ) {
						if ( value == elementData["id"] ) found = true;
					}
					if ( !found ) return;
				}

				if ( statusData["defense"].is<double>() ) {
					float adjust = statusData["defense"].as<float>() / 100.0f;
					morePower -= adjust;
				//	payload["message"] = payload["message"].as<std::string>() + "\n"+colorString("FF00FF") + "" + std::to_string((int)(adjust*100)) + "%"+colorString("FF0000") + " more defense"+colorString("FFFFFF") + "";
				}
			});
		//	if ( morePower != 0 ) payload["message"] = payload["message"].as<std::string>() + "\n"+colorString("FF00FF") + "" + std::to_string((int)(morePower*100)) + "%"+colorString("FF0000") + " power modifier"+colorString("FFFFFF") + "";

			float r = (rand() % 100) / 100.0;
			// missed
			if ( r > probability ) {
				uf::Serializer payload;
				payload["target"]["uid"] = target["uid"];
				payload["target"]["damage"] = "Miss";
				::gui->queueHook("world:Battle.Damage.%UID%", payload);
				return;
			}

			bool repel = false;
			std::function<void(const std::string&)> affinityCheck = [&]( const std::string& targetId ) {
				auto& target = metadata["battle"]["transients"][targetId];
				uf::Serializer cardData = masterDataGet("Card", target["id"].as<std::string>());
				uf::Serializer affinity = cardData["affinity"][elementId];
				// affinity change check
			//	for ( auto& status : target["statuses"] ) {
				ext::json::forEach(target["statuses"], [&](ext::json::Value& status){
					std::string statusId = status["id"].as<std::string>();
					uf::Serializer statusData = masterDataGet("Status", statusId);
					for ( auto it = status["modifier"].begin(); it != status["modifier"].end(); ++it ) {
						statusData[it.key()] = *it;
					}

					if ( !ext::json::isNull( statusData["type"] ) && elementData["type"] != statusData["type"] ) return;
					if ( !ext::json::isNull( statusData["attributes"] ) ) {
						bool found = false;
						for ( auto& value : statusData["attributes"] ) {
							if ( value == elementData["id"] ) found = true;
						}
						if ( !found ) return;
					}

					if ( !ext::json::isNull( statusData["affinity"] ) ) {
						if ( !ext::json::isNull( statusData["affinity"][elementId] ) ) affinity = statusData["affinity"][elementId];
					//	payload["message"] = payload["message"].as<std::string>() + "\naffinity modifier."+colorString("FFFFFF") + "";
					}
				});
				if ( !ext::json::isNull( affinity ) ) {
					std::string affinityStr = affinity.as<std::string>();
					if ( repel && affinityStr == "repel" ) affinityStr = "null";
					if ( affinityStr == "weak" ) { payload["message"] = payload["message"].as<std::string>() + "\n"+colorString("FF0000") + "Weak!\n"; morePower *= 2.0f; }
					else if ( affinityStr == "strong" ) { payload["message"] = payload["message"].as<std::string>() + "\n"+colorString("FF0000") + "Strong!\n"; morePower *= 0.5f; }
					else if ( affinityStr == "drain" ) { turnEffects.endTurn = true; payload["message"] = payload["message"].as<std::string>() + "\n"+colorString("FF0000") + "Drain!\n"; morePower *= -1.0f; }
					else if ( affinityStr == "null" ) { turnEffects.loseTurn = 2; payload["message"] = payload["message"].as<std::string>() + "\n"+colorString("FF0000") + "Nulled!\n"; morePower *= 0; }
					else if ( affinityStr == "repel" ) { turnEffects.endTurn = true; payload["message"] = payload["message"].as<std::string>() + "\n"+colorString("FF0000") + "Repelled!\n"; repel = true;
						// check again
						affinityCheck(json["uid"].as<std::string>());
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
			}; affinityCheck(target["uid"].as<std::string>());

			power += morePower;
			
			// damage calc
			float damage = 0.0f; {
				float calcDamage = 0.0f;
				// normal attack
				if ( elementData["type"] == "Physical"  ) {
					if ( skillData["hp%"].is<double>() ) {
						// max hp * power
						calcDamage = (member["max hp"].as<float>() * skillData["power"].as<float>() * 0.00114f );
					} else {
						// ( lv + str ) * power / 15
						calcDamage = (member["lv"].as<float>() + member["str"].as<float>()) * skillData["power"].as<float>() / 15.0f;
					}
				} else if ( elementData["type"] == "Magic" ) {
					if ( member["lv"].as<float>() < 30 ) {
						calcDamage *= skillData["power"].as<float>() * member["lv"].as<float>() * ( 2 * member["mag"].as<float>() + 70.0f ) / 1000.0f;
					} else if ( member["lv"].as<float>() < 160 ) {
						calcDamage = 3.0f * skillData["power"].as<float>() * ( 2 * member["mag"].as<float>() + 70.0f - ( 0.4f *  member["lv"].as<float>() ) ) / 100.0f;
					} else {
						calcDamage = 3.0f * skillData["power"].as<float>() * ( 2 * member["mag"].as<float>() + 5 ) / 100.0f;
					}
				} else {
					std::cout << elementData << ": " << skillData << std::endl;
				}
				damage = (int) (calcDamage * power);
			}

			if ( ext::json::isArray( skillData["hits"] ) ) {
				float low = skillData["hits"][0].as<float>();
				float high = skillData["hits"][1].as<float>();
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
				uf::Serializer cardData = masterDataGet("Card", target["id"].as<std::string>());
				uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].as<std::string>());
				int curHp = target["hp"].as<float>();
				bool isCrit = ((rand() % 100) / 100.0) < 0.1;
				if ( damage == 0 ) isCrit = false;
				float oldDamage = damage;
				if ( isCrit ) {
					damage *= 1.5;
					critted = true;
				}

				// is a heal
				if ( elementId == "14" ) {
					float power = skillData["power"].as<float>() / 100.0f * (morePower == 0 ? 1 : morePower);
					std::cout << target["max hp"].as<float>() << ", " << power << std::endl;
					damage = -1 * std::abs(target["max hp"].as<float>() * power);
				}

				if ( instakill ) damage = curHp;
				target["hp"] = (int) (curHp -= damage);
				if ( target["hp"].as<float>() > target["max hp"].as<float>() ) target["hp"] = target["max hp"].as<float>();

				std::cout << power << ", " << damage << ", " << morePower << (isCrit ? " / crit" : "") << std::endl;

				{

					uf::Serializer payload;
					payload["uid"] = target["uid"];
					payload["crit"] = isCrit;
					payload["message"] = "";
					if ( damage > 0 ) {
						payload["color"] = "FF0000";
						payload["message"] = (int) damage;
						
						if ( skillData["drains"].as<bool>() ) {
							float ratio = damage / target["max hp"].as<float>();
							int mpDrain = target["max mp"].as<float>() * ratio;
							if ( target["mp"].as<float>() > mpDrain ) mpDrain = target["mp"].as<float>();
							target["mp"] = target["mp"].as<float>() - mpDrain;
							payload["message"] = payload["message"].as<std::string>() + "\n  " + colorString("0000FF") + std::to_string(mpDrain);
							
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
					payload["message"] = payload["message"].as<std::string>() + "\n"+colorString("FF0000") + ""+ charaData["name"].as<std::string>() +" "+colorString("FFFFFF") + "died!";
					target["hp"] = 0;

					uf::Serializer statusPayload;
					statusPayload["id"] = "21";
					target["statuses"] = ext::json::array();
					target["statuses"].emplace_back(statusPayload);
				}
			}; applyDamage(repel ? json["uid"].as<std::string>() : target["uid"].as<std::string>());
			// counter statuses
			if ( !repel ) {
			//	for ( auto& status : metadata["battle"]["transients"][target["uid"].as<std::string>()]["statuses"] ) {
				ext::json::forEach(metadata["battle"]["transients"][target["uid"].as<std::string>()]["statuses"], [&](ext::json::Value& status){
					std::string statusId = status["id"].as<std::string>();
					if ( statusId != "19" ) return;
					uf::Serializer statusData = masterDataGet("Status", statusId);
					for ( auto it = status["modifier"].begin(); it != status["modifier"].end(); ++it ) {
						statusData[it.key()] = *it;
					}

					if ( !ext::json::isNull( statusData["type"] ) && elementData["type"] != statusData["type"] ) return;
					if ( !ext::json::isNull( statusData["attributes"] ) ) {
						bool found = false;
						for ( auto& value : statusData["attributes"] ) {
							if ( value == elementData["id"] ) found = true;
						}
						if ( !found ) return;
					}
					if ( statusData["proc"].is<double>() ) {
						float r = (rand() % 100) / 100.0;
						if ( r <= statusData["proc"].as<float>() / 100.0f ) {
							applyDamage(json["uid"].as<std::string>());
							payload["message"] = payload["message"].as<std::string>() + "\nCountered!";
						}
					}
				});
			}
			// apply status
			{
			//	for ( auto& status : skillData["statuses"] ) {
				ext::json::forEach(skillData["statuses"], [&](ext::json::Value& status){
					std::string target = status["target"].as<std::string>();
					std::string statusId = status["id"].as<std::string>();
					uf::Serializer statusData = masterDataGet("Status", statusId);
					if ( skillData["type"].as<int>() == 16 ) return;
					float r = (rand() % 100) / 100.0;
					if ( repel ) {
						if ( target == "enemy" ) target = "self";
						else target = "";
					}
					if ( statusData["proc"].is<double>() ) {
						// failed
						if ( r > statusData["proc"].as<float>() / 100.0f ) {
							target = "";
						}
					}

					uf::Serializer targets;
					if ( target == "self" ) {
						targets.emplace_back(json["uid"].as<std::string>());
				//	} else if ( target == "enemy" || target == "ally" ) {
					} else {
						targets.emplace_back(targetId.as<std::string>());
					}
				//	for ( auto& target : targets ) {
					ext::json::forEach(targets, [&](ext::json::Value& target){
						// remove status(es)
						if ( status["type"] == "remove" ) {
							for ( auto& statusC : metadata["battle"]["transients"][target.as<std::string>()]["statuses"] ) {
								if ( statusC["id"] == statusId ) {
									statusC = ext::json::null();
									statusC["id"] = 0;
									statusC["turns"] = -1;
								}
							}
							return;
						}

						uf::Serializer statusPayload;
						statusPayload["id"] = statusId;
						statusPayload["modifier"] = status["modifier"];

						statusPayload["turns"] = status["turns"].is<double>() ? status["turns"] : statusData["turns"];
						metadata["battle"]["transients"][target.as<std::string>()]["statuses"].emplace_back(statusPayload);

						for ( auto it = status["modifier"].begin(); it != status["modifier"].end(); ++it ) {
							statusData[it.key()] = *it;
						}

						uf::Serializer cardData = masterDataGet( "Card", metadata["battle"]["transients"][target.as<std::string>()]["id"].as<std::string>() );
						uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].as<std::string>());

					//	payload["message"] = payload["message"].as<std::string>() + "\nApplied "+colorString("FF0000") + "" + statusData["name"].as<std::string>() + ""+colorString("FFFFFF") + " to " + charaData["name"].as<std::string>();
						{
							uf::Serializer payload;
							payload["uid"] = targetId;
							payload["message"] = skillData["name"].as<std::string>();
							payload["color"] = "9900FF";
							showMessage( payload );
						}
					});
				});
			}

			if ( damage > 0 ) {
				float r = (rand() % 100) / 100.0;
			//	if ( r < 0.5 ) {
				if ( r < 1.0 ) {
					playSound(*this, member["id"].as<std::string>(), r > 0.25 ? "attack" : "skill");
				} else {
					playSound(*this, target["id"].as<std::string>(), r > 0.75 ? "damagedsmall" : "damagedlarge");
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
				::gui->queueHook("world:Battle.OnCrit.%UID%", payload, 0.1f);
			}
		});

		/* heal self */
		if ( skillData["add hp"].is<double>() ) {
			int heal = skillData["add hp"].as<float>() / 100.0f * member["max hp"].as<float>();
			if ( heal + member["hp"].as<float>() >= member["max hp"].as<float>() ) heal = member["max hp"].as<float>() - member["hp"].as<float>();
			member["hp"] = member["hp"].as<float>() + heal;

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
			float decrement = turnState["decrement"].as<float>();
			turnState["counter"] = turnState["counter"].as<float>() - turnEffects.loseTurn;
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
		auto& member = metadata["battle"]["transients"][json["uid"].as<std::string>()];
	/*
		ext::Housamo* transient = transients.at(uid);
		uf::Serializer& member = transient->getComponent<uf::Serializer>();
	*/
		std::string message;

		uf::Serializer cardData = masterDataGet("Card", member["id"].as<std::string>());
		uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].as<std::string>());
		uf::Serializer affinity = cardData["affinity"];
		std::string name = charaData["name"].as<std::string>();
		if ( member["type"] == "player" ) message += colorString("00FF00");
		else if ( member["type"] == "enemy" ) message += colorString("FF0000");
		else  message += colorString("AAAAAA");


		message += name + colorString("FFFFFF") + ": " + member["type"].as<std::string>();
		
		message += "\n" + colorString("FF0000") + "HP: " + member["hp"].as<std::string>();
		message += "\n" + colorString("0000FF") + "MP: " + member["mp"].as<std::string>();


		message += "\n" + colorString("9900FF") + "Ailments" + colorString("FFFFFF") + ":";
	//	for ( auto& status : member["statuses"] ) {
		ext::json::forEach(member["statuses"], [&](ext::json::Value& status){
			std::string statusId = status["id"].as<std::string>();
			uf::Serializer statusData = masterDataGet("Status", statusId);
			for ( auto it = status["modifier"].begin(); it != status["modifier"].end(); ++it ) {
				statusData[it.key()] = *it;
			}
			if ( statusId == "0" ) return;
			// affinity change check
			for ( auto it = statusData["affinity"].begin(); it != statusData["affinity"].end(); ++it ) {
				affinity[it.key()] = *it;
			}
			message += "\n\t" + statusData["name"].as<std::string>() + (ext::json::isNull( status["turns"] ) ? "" : " (Turns: " + status["turns"].as<std::string>() + ")" );
		});

		message += "\n" + colorString("9900FF") + "Affinities" + colorString("FFFFFF") + ":";

	//	for ( auto it = affinity.begin(); it != affinity.end(); ++it ) {
	//		std::string elementId = it.key();
	//		std::string value = it->as<std::string>();
		ext::json::forEach(affinity, [&](const std::string& elementId, ext::json::Value& value){
			uf::Serializer elementData = masterDataGet("Element", elementId);
			message += "\n\t" + elementData["name"].as<std::string>() + ": " + value.as<std::string>();
		});

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
		::gui->callHook("menu:Close.%UID%");
	
		// cleanup
		for ( ext::Housamo* pointer : transients ) {
			pointer->destroy();
			this->removeChild(*pointer);
			delete pointer;
		}
		transients.clear();

		if ( json["won"].as<bool>() ) {
			// play death sound
		//	playSound(*this, metadata["battle"]["enemy"]["uid"].as<size_t>(), "killed");
			// kill
			{
				uf::Scene& scene = uf::scene::getCurrentScene();
				std::size_t uid = metadata["battle"]["enemy"]["uid"].as<size_t>();
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
		if ( json["action"].as<std::string>() == "killed" ) {
			// play death sound
			playSound(*this, metadata["battle"]["enemy"]["uid"].as<size_t>(), "killed");
			// kill
			{
				uf::Scene& scene = uf::scene::getCurrentScene();
				std::size_t uid = metadata["battle"]["enemy"]["uid"].as<size_t>();
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
			std::size_t uid = metadata["battle"]["player"]["uid"].as<size_t>();
			uf::Entity*  = scene.findByUid(uid);
			if (  ) {
				uf::Serializer& pMetadata = ->getComponent<uf::Serializer>();
				pMetadata["system"].erase("battle");
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