#include "behavior.h"

#include <uf/utils/time/time.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/window/window.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/audio/audio.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/camera/camera.h>
#include <uf/engine/asset/asset.h>
#include <uf/engine/asset/masterdata.h>
#include <uf/utils/renderer/renderer.h>
#include <uf/ext/gltf/gltf.h>
#include <uf/utils/math/collision.h>
#include "../../ext.h"
#include "../../gui/gui.h"

#include <uf/utils/math/physics.h>
#include <uf/ext/openvr/openvr.h>

namespace {
	uf::Object* circleIn;
	uf::Object* circleOut;
}

EXT_BEHAVIOR_REGISTER_CPP(StartMenuBehavior)
EXT_BEHAVIOR_REGISTER_AS_OBJECT(StartMenuBehavior, StartMenu)
#define this ((uf::Scene*) &self)
void ext::StartMenuBehavior::initialize( uf::Object& self ) {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Asset& assetLoader = this->getComponent<uf::Asset>();

	/* Magic Circle Outter */ {
	//	circleOut = (uf::Object*) this->findByUid( this->loadChildUid("./gui/mainmenu/circle-out.json", true) );
		circleOut = this->loadChildPointer("./gui/mainmenu/circle-out.json", true);
	}
	/* Magic Circle Inner */ {
	//	circleIn = (uf::Object*) this->findByUid( this->loadChildUid("./gui/mainmenu/circle-in.json", true) );
		circleIn = this->loadChildPointer("./gui/mainmenu/circle-in.json", true);
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
void ext::StartMenuBehavior::tick( uf::Object& self ) {
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
void ext::StartMenuBehavior::render( uf::Object& self ) {
}
void ext::StartMenuBehavior::destroy( uf::Object& self ) {
}
#undef this