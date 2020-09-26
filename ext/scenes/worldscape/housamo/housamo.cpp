#include ".h"

#include <uf/engine/scene/scene.h>
#include <uf/utils/serialize/serializer.h>

namespace {
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

void ext::Housamo::initialize() {
	uf::Object::initialize();

	this->m_name = "Housamo";

	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	/*
		{ 
			"id": "140901",
			"lv": 60,
			"sl": 100,
			"jingi": 100,
		*	"limit": 3,
		* 	"hp": 10082,
		* 	"atk": 6119,
		* 	"skills": [401,349,203,532],
		*	"skin": "skin1"
		},
	*/
	std::string id = metadata[""]["id"].asString();
	uint64_t lv = !metadata[""]["level"].isNull() ? parseInt(metadata[""]["level"].asString()) : parseInt(metadata[""]["lv"].asString());
	uint64_t sl = !metadata[""]["skill level"].isNull() ? parseInt(metadata[""]["skill level"].asString()) : parseInt(metadata[""]["sl"].asString());
	uint64_t jingi = parseInt(metadata[""]["jingi"].asString());
	uint64_t limit = parseInt(metadata[""]["limit"].asString());
	uint64_t str = parseInt(metadata[""]["str"].asString());
	uint64_t mag = parseInt(metadata[""]["mag"].asString());
	uint64_t vit = parseInt(metadata[""]["vit"].asString());
	uint64_t agi = parseInt(metadata[""]["agi"].asString());
	 int64_t hp = metadata[""]["hp"].isNull() ? -1 : parseInt(metadata[""]["hp"].asString());
	 int64_t mp = metadata[""]["mp"].isNull() ? -1 : parseInt(metadata[""]["mp"].asString());
	
	std::vector<std::string> skills;
	for ( auto i = 0; i < metadata[""]["skills"].size(); ++i ) {
		skills.push_back(metadata[""]["skills"][i].asString());
	}
	uf::Serializer cardData = masterDataGet("Card",id);
/*
	{
		uint64_t base = parseInt(cardData["base_hp"].asString());
		uint64_t max = parseInt(cardData["max_hp"].asString());
		uint64_t maxlv = parseInt(cardData["max_lv"].asString());
		max_hp += ( (max - base) * (lv - 1) / (maxlv - 1) ) + base;
		max_hp /= 20.0f;
	}
	{
		uint64_t base = parseInt(cardData["base_mp"].asString());
		uint64_t max = parseInt(cardData["max_mp"].asString());
		uint64_t maxlv = parseInt(cardData["max_lv"].asString());
		max_mp += ( (max - base) * (lv - 1) / (maxlv - 1) ) + base;
	}
*/
	{
		float scale = 3.0f;
		if ( str == 0 ) {
			uint64_t base = parseInt(cardData["base_atk"].asString());
			uint64_t max = parseInt(cardData["max_atk"].asString());
			uint64_t maxlv = parseInt(cardData["max_lv"].asString());
			str = ( (max - base) * (lv - 1) / (maxlv - 1) ) + base;
			str /= scale * 100.0f;
		}
		if ( mag == 0 ) {
			uint64_t base = parseInt(cardData["base_atk"].asString());
			uint64_t max = parseInt(cardData["max_atk"].asString());
			uint64_t maxlv = parseInt(cardData["max_lv"].asString());
			mag = ( (max - base) * (lv - 1) / (maxlv - 1) ) + base;
			mag /= scale * 100.0f;
		}
		if ( vit == 0 ) {
			uint64_t base = parseInt(cardData["base_hp"].asString());
			uint64_t max = parseInt(cardData["max_hp"].asString());
			uint64_t maxlv = parseInt(cardData["max_lv"].asString());
			vit = ( (max - base) * (lv - 1) / (maxlv - 1) ) + base;
			vit /= scale * 100.0f;
		}
	}
	uint64_t max_hp = (lv + vit) * 6;
	uint64_t max_mp = (mag + lv) * 3;

	if ( metadata[""]["max hp"].isNumeric() ) max_hp = parseInt(metadata[""]["max hp"].asString());
	if ( metadata[""]["max mp"].isNumeric() ) max_mp = parseInt(metadata[""]["max mp"].asString());

	if ( hp == -1 ) hp = max_hp;
	if ( mp == -1 ) mp = max_mp;
	if ( limit == 0 ) {
		uint64_t baseL0 = 0;
		switch ( parseInt(cardData["rarity"].asString() ) ) {
			case 1: baseL0 = 20; break;
			case 2: baseL0 = 25; break;
			case 3: baseL0 = 30; break;
			case 4: baseL0 = 35; break;
			case 5: baseL0 = 40; break;
		}
		while ( baseL0 > 0 && baseL0 < lv ) {
			++limit;
			baseL0 += 10;
		}
	}
	// populate skills based on masterdata
	if ( skills.empty() ) {
		for ( auto i = 0; i < cardData["skill_id"].size(); ++i ) {
			int learn = cardData["skill_learn_lv"][i].asInt();
			if ( lv < learn ) break;
			skills.push_back(cardData["skill_id"][i].asString());
		}
	}
	

	// convert back
	metadata[""].removeMember("level");
	metadata[""].removeMember("skill level");
	metadata[""]["lv"] = lv;
	metadata[""]["sl"] = sl;
	metadata[""]["jingi"] = jingi;
	metadata[""]["limit"] = limit;
	metadata[""]["hp"] = hp;
	metadata[""]["max hp"] = max_hp;
	metadata[""]["mp"] = mp;
	metadata[""]["max mp"] = max_mp;
	metadata[""]["str"] = str;
	metadata[""]["mag"] = mag;
	metadata[""]["vit"] = vit;
	metadata[""]["agi"] = agi;
	metadata[""]["skills"] = Json::arrayValue;

//	std::cout << metadata[""] << std::endl;

	for ( auto& id : skills ) metadata[""]["skills"].append(id);
}
void ext::Housamo::tick() {
	uf::Object::tick();
}
void ext::Housamo::render() {
	uf::Object::render();
}
void ext::Housamo::destroy() {
	uf::Object::destroy();
}