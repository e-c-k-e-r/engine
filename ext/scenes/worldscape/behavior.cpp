#include "behavior.h"

#include <uf/utils/time/time.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/window/window.h>

#include <uf/utils/audio/audio.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/camera/camera.h>

#include <uf/engine/asset/asset.h>
#include <uf/engine/asset/masterdata.h>

#include "../../ext.h"

#include <uf/utils/renderer/renderer.h>
#include <uf/ext/openvr/openvr.h>

UF_BEHAVIOR_REGISTER_CPP(ext::WorldScapeSceneBehavior)
#define this ((uf::Scene*) &self)
void ext::WorldScapeSceneBehavior::initialize( uf::Object& self ) {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Asset& assetLoader = this->getComponent<uf::Asset>();

	/* Grab master data */ {
		std::vector<std::string> tables = {
			"Card",
			"Chara",
			"Skill",
			"Element",
			"Status",
		};
		for ( auto& table : tables ) {
			uf::MasterData data;
			metadata["system"]["mastertable"][table] = data.load(table);
		}
	}

	static uf::Timer<long long> timer(false);
	if ( !timer.running() ) timer.start();
/*	
	this->addHook( "world:Battle.Prepare", [&](const std::string& event)->std::string{
		uf::Serializer json = event;

		uf::Entity& player = this->getController();
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
		uf::Object* guiManager = (uf::Object*) this->globalFindByName("Gui Manager");
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
		ext::HousamoBattle* battleManager = (ext::HousamoBattle*) this->globalFindByName("Battle Manager");
		if ( !battleManager ) return "false";

		this->removeChild(*battleManager);
		battleManager->destroy();
		delete battleManager;

		return "true";
	});
	this->addHook( "menu:Dialogue.Start", [&](const std::string& event)->std::string{
		if ( timer.elapsed().asDouble() < 1 ) return "false";
		timer.reset();
		uf::Object* guiManager = (uf::Object*) this->globalFindByName("Gui Manager");
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
		ext::DialogueManager* dialogueManager = (ext::DialogueManager*) this->globalFindByName("Dialogue Manager");
		if ( !dialogueManager ) return "false";

		this->removeChild(*dialogueManager);
		dialogueManager->destroy();
		delete dialogueManager;

		return "true";
	});
*/
}
void ext::WorldScapeSceneBehavior::tick( uf::Object& self ) {
}
void ext::WorldScapeSceneBehavior::render( uf::Object& self ){}
void ext::WorldScapeSceneBehavior::destroy( uf::Object& self ){}
#undef this