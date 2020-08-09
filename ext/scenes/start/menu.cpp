#include "menu.h"
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

#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/openvr/openvr.h>

#include "../../ext.h"
#include "../../gui/gui.h"

namespace {
	uf::Object controller;

	ext::Gui* circleIn;
	ext::Gui* circleOut;
}

uf::Entity* ext::StartMenu::getController() {
	return (uf::Entity*) &controller;
}
const uf::Entity* ext::StartMenu::getController() const {
	return (uf::Entity*) &controller;
}

EXT_OBJECT_REGISTER_CPP(StartMenu)
void ext::StartMenu::initialize() {
	uf::Scene::initialize();
	this->m_name = "Main Menu";
//	this->load("./scenes/start/scene.json");
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Asset& assetLoader = this->getComponent<uf::Asset>();

	this->addHook( "system:Quit.%UID%", [&](const std::string& event)->std::string{
		std::cout << event << std::endl;
		ext::ready = false;
		return "true";
	});
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

	this->addHook( "world:Entity.LoadAsset", [&](const std::string& event)->std::string{
		uf::Serializer json = event;

		std::string asset = json["asset"].asString();
		std::string uid = json["uid"].asString();

		assetLoader.load(asset, "asset:Load." + uid);

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
		payload["state"] = true;
		uf::hooks.call("window:Mouse.CursorVisibility", payload);
		uf::hooks.call("window:Mouse.Lock");
	}

	/* Magic Circle Outter */ {
		circleOut = (ext::Gui*) this->findByUid( this->loadChild("./gui/mainmenu/circle-out.json", true) );
	}
	/* Magic Circle Inner */ {
		circleIn = (ext::Gui*) this->findByUid( this->loadChild("./gui/mainmenu/circle-in.json", true) );
	}
	// update camera
	{
		controller.getComponent<uf::Camera>().update(true);
		pod::Transform<>& transform = controller.getComponent<pod::Transform<>>();

		uf::Serializer json;
		json.readFromFile("./data/entities/player.json");
		controller.getComponent<uf::Serializer>()["overlay"] = json["metadata"]["overlay"];
	}
}

void ext::StartMenu::tick() {
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

	if ( circleIn ) {
		pod::Transform<>& transform = circleIn->getComponent<pod::Transform<>>();
		static float time = 0.0f;
		float speed = 0.0125f;
		uf::Serializer& metadata = circleIn->getComponent<uf::Serializer>();
		if ( metadata["hovered"].asBool() ) speed = 0.25f;
		time += uf::physics::time::delta * -speed;
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
	//	metadata["color"][3] = alpha;
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
	//	metadata["color"][3] = alpha;
	}

	// check on controller position
/*
	{
		pod::Vector3f position = ext::openvr::controllerPosition( vr::Controller_Hand::Hand_Right );
		pod::Quaternion<> orientation = ext::openvr::controllerQuaternion( vr::Controller_Hand::Hand_Right );
		std::cout << "Right hand:\n";
		std::cout << "\tPosition:    " << position.x << ", " << position.y << ", " << position.z << "\n";
		std::cout << "\tOrientation: " << orientation.x << ", " << orientation.y << ", " << orientation.z << ", " << orientation.w << std::endl;
	}
*/
	{
		auto& camera = controller.getComponent<uf::Camera>();
		camera.updateView();
	}
}

void ext::StartMenu::render() {
	uf::Scene::render();
}