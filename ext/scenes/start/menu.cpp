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

#include <uf/utils/renderer/renderer.h>
#include <uf/ext/openvr/openvr.h>

#include "../../ext.h"
#include "../../gui/gui.h"

namespace {
	ext::Gui* circleIn;
	ext::Gui* circleOut;
}

uf::Entity& ext::StartMenu::getController() {
	return *this;
}
const uf::Entity& ext::StartMenu::getController() const {
	return *this;
}

EXT_OBJECT_REGISTER_CPP(StartMenu)
void ext::StartMenu::initialize() {
	ext::Scene::initialize();
	this->m_name = "Main Menu";

	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Asset& assetLoader = this->getComponent<uf::Asset>();

	/* Magic Circle Outter */ {
		circleOut = (ext::Gui*) this->findByUid( this->loadChild("./gui/mainmenu/circle-out.json", true) );
	}
	/* Magic Circle Inner */ {
		circleIn = (ext::Gui*) this->findByUid( this->loadChild("./gui/mainmenu/circle-in.json", true) );
	}
	// update camera
	{
		auto& controller = this->getController();
		controller.getComponent<uf::Camera>().update(true);
		pod::Transform<>& transform = controller.getComponent<pod::Transform<>>();

		uf::Serializer json;
		json.readFromFile("./data/entities/player.json");
		controller.getComponent<uf::Serializer>()["overlay"] = json["metadata"]["overlay"];
	}
}

void ext::StartMenu::tick() {
	ext::Scene::tick();

	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Asset& assetLoader = this->getComponent<uf::Asset>();

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

	{
		auto& controller = this->getController();
		auto& camera = controller.getComponent<uf::Camera>();
		camera.updateView();
	}
}

void ext::StartMenu::render() {
	ext::Scene::render();
}