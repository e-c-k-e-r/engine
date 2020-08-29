#include "world.h"
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

#include "./player/player.h"

#include "./gui/battle.h"
#include "./gui/dialogue.h"
#include "./gui/pause.h"

#include ".//battle.h"
#include ".//dialogue.h"

#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/rendermodes/deferred.h>
#include <uf/ext/vulkan/rendermodes/rendertarget.h>
#include <uf/ext/vulkan/rendermodes/stereoscopic_deferred.h>
#include <uf/ext/openvr/openvr.h>

EXT_OBJECT_REGISTER_CPP(World)
void ext::World::initialize() {
	uf::Scene::initialize();
	this->m_name = "World";
//	this->load("./scenes/world/scene.json");

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
	
	this->addHook( "world:Music.LoadPrevious.%UID%", [&](const std::string& event)->std::string{
		uf::Serializer json = event;

		if ( metadata["previous bgm"]["filename"] == "" ) return "false";

		std::string filename = metadata["previous bgm"]["filename"].asString();
		float timestamp = metadata["previous bgm"]["timestamp"].asFloat();

//		std::cout << metadata["previous bgm"] << std::endl;

		uf::Audio& audio = this->getComponent<uf::Audio>();
		if ( audio.playing() ) {
			metadata["previous bgm"]["filename"] = audio.getFilename();
			metadata["previous bgm"]["timestamp"] = audio.getTime();
			audio.stop();
		}
		audio.load(filename);
		audio.setVolume(metadata["volumes"]["bgm"].asFloat());
		audio.setTime(timestamp);
		audio.play();

		return "true";
	});
	this->addHook( "asset:Load." + std::to_string(this->getUid()), [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		std::string filename = json["filename"].asString();

		if ( uf::string::extension(filename) != "ogg" ) return "false";
		const uf::Audio* audioPointer = NULL;
		try { audioPointer = &assetLoader.get<uf::Audio>(filename); } catch ( ... ) {}
		if ( !audioPointer ) return "false";

		uf::Audio& audio = this->getComponent<uf::Audio>();
		if ( audio.playing() ) {
		/*
			if ( filename.find("_intro") == std::string::npos ) {
				metadata["previous bgm"]["filename"] = audio.getFilename();
				metadata["previous bgm"]["timestamp"] = audio.getTime();
			}
		*/
			audio.stop();
		}
		
//		std::cout << metadata["previous bgm"] << std::endl;

		audio.load(filename);
		audio.setVolume(metadata["volumes"]["bgm"].asFloat());
		audio.play();

		return "true";
	});

	static uf::Timer<long long> timer(false);
	if ( !timer.running() ) timer.start();
	this->addHook( "menu:Pause", [&](const std::string& event)->std::string{
		if ( timer.elapsed().asDouble() < 1 ) return "false";
		timer.reset();

		uf::Serializer json = event;
		ext::Gui* manager = (ext::Gui*) this->findByName("Gui Manager");
		if ( !manager ) return "false";
		uf::Serializer payload;
		ext::Gui* gui = (ext::Gui*) manager->findByUid( (payload["uid"] = manager->loadChild("/scenes/world/gui/pause/menu.json", false)).asUInt64() );
		uf::Serializer& metadata = gui->getComponent<uf::Serializer>();
		metadata["menu"] = json["menu"];
		gui->initialize();
		return payload;
	});
	this->addHook( "world:Entity.LoadAsset", [&](const std::string& event)->std::string{
		uf::Serializer json = event;

		std::string asset = json["asset"].asString();
		std::string uid = json["uid"].asString();

		assetLoader.load(asset, "asset:Load." + uid);

		return "true";
	});
	this->addHook( "world:Battle.Prepare", [&](const std::string& event)->std::string{
		uf::Serializer json = event;

		uf::Entity& player = *this->getController();
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
		ext::Gui* guiManager = (ext::Gui*) this->findByName("Gui Manager");
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
		ext::HousamoBattle* battleManager = (ext::HousamoBattle*) this->findByName("Battle Manager");
		if ( !battleManager ) return "false";

		this->removeChild(*battleManager);
		battleManager->destroy();
		delete battleManager;

		return "true";
	});
	this->addHook( "menu:Dialogue.Start", [&](const std::string& event)->std::string{
		if ( timer.elapsed().asDouble() < 1 ) return "false";
		timer.reset();
		ext::Gui* guiManager = (ext::Gui*) this->findByName("Gui Manager");
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
		ext::DialogueManager* dialogueManager = (ext::DialogueManager*) this->findByName("Dialogue Manager");
		if ( !dialogueManager ) return "false";

		this->removeChild(*dialogueManager);
		dialogueManager->destroy();
		delete dialogueManager;

		return "true";
	});

	/* store viewport size */ {
		metadata["window"]["size"]["x"] = ext::vulkan::width;
		metadata["window"]["size"]["y"] = ext::vulkan::height;
		
		this->addHook( "window:Resized", [&](const std::string& event)->std::string{
			uf::Serializer json = event;

			pod::Vector2ui size; {
				size.x = json["window"]["size"]["x"].asUInt64();
				size.y = json["window"]["size"]["y"].asUInt64();
			}

			metadata["window"] = json["window"];

			return "true";
		});
	}

	// lock control
	{
		uf::Serializer payload;
		payload["state"] = false;
		uf::hooks.call("window:Mouse.CursorVisibility", payload);
		uf::hooks.call("window:Mouse.Lock");
	}
}

void ext::World::tick() {
	uf::Scene::tick();

	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Asset& assetLoader = this->getComponent<uf::Asset>();

	/* check if audio needs to loop */ try {
		uf::Audio& bgm = this->getComponent<uf::Audio>();
		float current = bgm.getTime();
		float end = bgm.getDuration();
		float epsilon = 0.005f;
		if ( current + epsilon >= end || !bgm.playing() ) {
			// intro to main transition
			std::string filename = bgm.getFilename();
			filename = assetLoader.getOriginal(filename);
			if ( filename.find("_intro") != std::string::npos ) {
				assetLoader.load(uf::string::replace( filename, "_intro", "" ), "asset:Load." + std::to_string(this->getUid()));
			// loop
			} else {
				bgm.setTime(0);
				if ( !bgm.playing() ) bgm.play();
			}
		}
	} catch ( ... ) {

	}

	/* Regain control if nothing requests it */ {
		ext::Gui* menu = (ext::Gui*) this->findByName("Gui: Menu");
		if ( !menu ) {
			uf::Serializer payload;
			payload["state"] = false;
			uf::hooks.call("window:Mouse.CursorVisibility", payload);
			uf::hooks.call("window:Mouse.Lock");
		}
	}

	/* Print Entity Information */  {
		static uf::Timer<long long> timer(false);
		if ( !timer.running() ) timer.start();
		if ( uf::Window::isKeyPressed("U") && timer.elapsed().asDouble() >= 1 ) { timer.reset();
			uf::iostream << "Current size: " << uf::Entity::entities.size() << " | UIDs: " << uf::Entity::uids << "\n";
			uint orphans = 0;
			uint empty = 0;
			for ( uf::Entity* e : uf::Entity::entities ) {
				if ( e && !e->hasParent() ) {
					++orphans;
					uf::iostream << "Orphan: " << e->getName() << ": " << e << "\n";
				}
				if ( !e ) ++empty;
			}
			uf::iostream << "Orphans: " << orphans << "\n";
			uf::iostream << "Empty: " << empty << "\n";
		}
	}
	
	/* Updates Sound Listener */ {
		pod::Transform<>& transform = this->getController()->getComponent<pod::Transform<>>();
		
		ext::oal.listener( "POSITION", { transform.position.x, transform.position.y, transform.position.z } );
		ext::oal.listener( "VELOCITY", { 0, 0, 0 } );
		ext::oal.listener( "ORIENTATION", { 0, 0, 1, 1, 0, 0 } );
	}
}

void ext::World::render() {
	uf::Scene::render();

	/* Update lights */ {
		uf::Serializer& metadata = this->getComponent<uf::Serializer>();
		auto& scene = *this;
		std::vector<ext::vulkan::Graphic*> blitters;
		auto& renderMode = ext::vulkan::getRenderMode("", true);
		if ( renderMode.getType() == "Deferred (Stereoscopic)" ) {
			auto* renderModePointer = (ext::vulkan::StereoscopicDeferredRenderMode*) &renderMode;
			blitters.push_back(&renderModePointer->blitters.left);
			blitters.push_back(&renderModePointer->blitters.right);
		} else if ( renderMode.getType() == "Deferred" ) {
			auto* renderModePointer = (ext::vulkan::DeferredRenderMode*) &renderMode;
			blitters.push_back(&renderModePointer->blitter);
		}
		auto& controller = *scene.getController();
		auto& camera = controller.getComponent<uf::Camera>();
	//	auto& uniforms = blitter.uniforms;
		struct UniformDescriptor {
			struct Matrices {
				alignas(16) pod::Matrix4f view[2];
				alignas(16) pod::Matrix4f projection[2];
			} matrices;
			alignas(16) pod::Vector4f ambient;
			struct Light {
				alignas(16) pod::Vector4f position;
				alignas(16) pod::Vector4f color;
			} lights;
		};
		
		struct SpecializationConstant {
			int32_t maxLights = 32;
		} specializationConstants;

		for ( size_t _ = 0; _ < blitters.size(); ++_ ) {
			auto& blitter = *blitters[_];
			
			uint8_t* buffer;
			size_t len;
			auto* shader = &blitter.material.shaders.front();
			
			for ( auto& _ : blitter.material.shaders ) {
				if ( _.uniforms.empty() ) continue;
				auto& userdata = _.uniforms.front();
				buffer = (uint8_t*) (void*) userdata;
				len = userdata.data().len;
				shader = &_;
				specializationConstants = _.specializationConstants.get<SpecializationConstant>();
			}

			if ( !buffer ) continue;

			UniformDescriptor* uniforms = (UniformDescriptor*) buffer;
			for ( std::size_t i = 0; i < 2; ++i ) {
				uniforms->matrices.view[i] = camera.getView( i );
				uniforms->matrices.projection[i] = camera.getProjection( i );
			}
			{
				uniforms->ambient.x = metadata["light"]["ambient"][0].asFloat();
				uniforms->ambient.y = metadata["light"]["ambient"][1].asFloat();
				uniforms->ambient.z = metadata["light"]["ambient"][2].asFloat();
				uniforms->ambient.w = metadata["light"]["kexp"].asFloat();
			}
			{
				std::vector<uf::Entity*> entities;
				std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
					if ( !entity || entity->getName() != "Light" ) return;
					entities.push_back(entity);
				};
				for ( uf::Scene* scene : ext::vulkan::scenes ) { if ( !scene ) continue;
					scene->process(filter);
				}
				{
					const pod::Vector3& position = controller.getComponent<pod::Transform<>>().position;
					std::sort( entities.begin(), entities.end(), [&]( const uf::Entity* l, const uf::Entity* r ){
						if ( !l ) return false; if ( !r ) return true;
						if ( !l->hasComponent<pod::Transform<>>() ) return false; if ( !r->hasComponent<pod::Transform<>>() ) return true;
						return uf::vector::magnitude( uf::vector::subtract( l->getComponent<pod::Transform<>>().position, position ) ) < uf::vector::magnitude( uf::vector::subtract( r->getComponent<pod::Transform<>>().position, position ) );
					} );
				}

				{
					uf::Serializer& metadata = controller.getComponent<uf::Serializer>();
					if ( metadata["light"]["should"].asBool() ) {
						entities.push_back(&controller);
					}
				}
				UniformDescriptor::Light* lights = (UniformDescriptor::Light*) &buffer[sizeof(UniformDescriptor) - sizeof(UniformDescriptor::Light)];
				for ( size_t i = 0; i < specializationConstants.maxLights; ++i ) {
					UniformDescriptor::Light& light = lights[i];
					light.position = { 0, 0, 0, 0 };
					light.color = { 0, 0, 0, 0 };
				}
				for ( size_t i = 0; i < specializationConstants.maxLights && i < entities.size(); ++i ) {
					UniformDescriptor::Light& light = lights[i];
					uf::Entity* entity = entities[i];

					pod::Transform<>& transform = entity->getComponent<pod::Transform<>>();
					uf::Serializer& metadata = entity->getComponent<uf::Serializer>();
					
					light.position.x = transform.position.x;
					light.position.y = transform.position.y;
					light.position.z = transform.position.z;

					if ( entity == &controller ) {
						light.position.y += 2;
					}

					light.position.w = metadata["light"]["power"].asFloat();

					light.color.x = metadata["light"]["color"][0].asFloat();
					light.color.y = metadata["light"]["color"][1].asFloat();
					light.color.z = metadata["light"]["color"][2].asFloat();
					
					light.color.w = metadata["light"]["radius"].asFloat();
				}
			}
			// blitter.updateBuffer( (void*) buffer, blitter.uniforms.data().len, 0, false );
			shader->updateBuffer( (void*) buffer, len, 0, false );
		}
	}
}

uf::Entity* ext::World::getController() {
	if ( ext::vulkan::currentRenderMode ) {
		auto& renderMode = *ext::vulkan::currentRenderMode;
		std::string name = renderMode.name;
		auto split = uf::string::split( name, ": " );
		if ( split.front() == "Render Target" ) {
			uint64_t uid = std::stoi( split.back() );
			uf::Entity* ent = this->findByUid( uid );
			if ( ent ) return ent;
		}
	}
	return uf::Scene::getController();
}

const uf::Entity* ext::World::getController() const {
	return uf::Scene::getController();
}