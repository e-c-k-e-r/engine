#if 0
#include ".h"

#include <uf/engine/scene/scene.h>
#include <uf/utils/serialize/serializer.h>

namespace {
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
	uf::stl::string id = metadata[""]["id"].as<uf::stl::string>();
	uint64_t lv = !ext::json::isNull( metadata[""]["level"] ) ? parseInt(metadata[""]["level"].as<uf::stl::string>()) : parseInt(metadata[""]["lv"].as<uf::stl::string>());
	uint64_t sl = !ext::json::isNull( metadata[""]["skill level"] ) ? parseInt(metadata[""]["skill level"].as<uf::stl::string>()) : parseInt(metadata[""]["sl"].as<uf::stl::string>());
	uint64_t jingi = parseInt(metadata[""]["jingi"].as<uf::stl::string>());
	uint64_t limit = parseInt(metadata[""]["limit"].as<uf::stl::string>());
	uint64_t str = parseInt(metadata[""]["str"].as<uf::stl::string>());
	uint64_t mag = parseInt(metadata[""]["mag"].as<uf::stl::string>());
	uint64_t vit = parseInt(metadata[""]["vit"].as<uf::stl::string>());
	uint64_t agi = parseInt(metadata[""]["agi"].as<uf::stl::string>());
	 int64_t hp = ext::json::isNull( metadata[""]["hp"] ) ? -1 : parseInt(metadata[""]["hp"].as<uf::stl::string>());
	 int64_t mp = ext::json::isNull( metadata[""]["mp"] ) ? -1 : parseInt(metadata[""]["mp"].as<uf::stl::string>());
	
	uf::stl::vector<uf::stl::string> skills;
	for ( auto i = 0; i < metadata[""]["skills"].size(); ++i ) {
		skills.push_back(metadata[""]["skills"][i].as<uf::stl::string>());
	}
	uf::Serializer cardData = masterDataGet("Card",id);
/*
	{
		uint64_t base = parseInt(cardData["base_hp"].as<uf::stl::string>());
		uint64_t max = parseInt(cardData["max_hp"].as<uf::stl::string>());
		uint64_t maxlv = parseInt(cardData["max_lv"].as<uf::stl::string>());
		max_hp += ( (max - base) * (lv - 1) / (maxlv - 1) ) + base;
		max_hp /= 20.0f;
	}
	{
		uint64_t base = parseInt(cardData["base_mp"].as<uf::stl::string>());
		uint64_t max = parseInt(cardData["max_mp"].as<uf::stl::string>());
		uint64_t maxlv = parseInt(cardData["max_lv"].as<uf::stl::string>());
		max_mp += ( (max - base) * (lv - 1) / (maxlv - 1) ) + base;
	}
*/
	{
		float scale = 3.0f;
		if ( str == 0 ) {
			uint64_t base = parseInt(cardData["base_atk"].as<uf::stl::string>());
			uint64_t max = parseInt(cardData["max_atk"].as<uf::stl::string>());
			uint64_t maxlv = parseInt(cardData["max_lv"].as<uf::stl::string>());
			str = ( (max - base) * (lv - 1) / (maxlv - 1) ) + base;
			str /= scale * 100.0f;
		}
		if ( mag == 0 ) {
			uint64_t base = parseInt(cardData["base_atk"].as<uf::stl::string>());
			uint64_t max = parseInt(cardData["max_atk"].as<uf::stl::string>());
			uint64_t maxlv = parseInt(cardData["max_lv"].as<uf::stl::string>());
			mag = ( (max - base) * (lv - 1) / (maxlv - 1) ) + base;
			mag /= scale * 100.0f;
		}
		if ( vit == 0 ) {
			uint64_t base = parseInt(cardData["base_hp"].as<uf::stl::string>());
			uint64_t max = parseInt(cardData["max_hp"].as<uf::stl::string>());
			uint64_t maxlv = parseInt(cardData["max_lv"].as<uf::stl::string>());
			vit = ( (max - base) * (lv - 1) / (maxlv - 1) ) + base;
			vit /= scale * 100.0f;
		}
	}
	uint64_t max_hp = (lv + vit) * 6;
	uint64_t max_mp = (mag + lv) * 3;

	if ( metadata[""]["max hp"].is<double>() ) max_hp = parseInt(metadata[""]["max hp"].as<uf::stl::string>());
	if ( metadata[""]["max mp"].is<double>() ) max_mp = parseInt(metadata[""]["max mp"].as<uf::stl::string>());

	if ( hp == -1 ) hp = max_hp;
	if ( mp == -1 ) mp = max_mp;
	if ( limit == 0 ) {
		uint64_t baseL0 = 0;
		switch ( parseInt(cardData["rarity"].as<uf::stl::string>() ) ) {
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
			int learn = cardData["skill_learn_lv"][i].as<int>();
			if ( lv < learn ) break;
			skills.push_back(cardData["skill_id"][i].as<uf::stl::string>());
		}
	}
	

	// convert back
	metadata[""].erase("level");
	metadata[""].erase("skill level");
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
	metadata[""]["skills"] = ext::json::array();

//	std::cout << metadata[""] << std::endl;

	for ( auto& id : skills ) metadata[""]["skills"].emplace_back(id);
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
#endif