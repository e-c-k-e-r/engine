#include "behavior.h"
#include "../behavior.h"
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/hook/hook.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/window/window.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/audio/audio.h>
#include <uf/ext/openvr/openvr.h>
#include <uf/utils/math/physics.h>
#include <mutex>

UF_BEHAVIOR_REGISTER_CPP(ext::PlayerModelBehavior)
#define this ((uf::Scene*) &self)

void ext::PlayerModelBehavior::initialize( uf::Object& self ) {
}
void ext::PlayerModelBehavior::tick( uf::Object& self ) {
	if ( this->getChildren().size() != 1 ) return;
	auto& metadata = this->getComponent<uf::Serializer>();
	if ( !metadata["track player"].as<bool>() ) return;
	auto& player = this->getParent().as<uf::Object>();
	auto& model = this->getChildren().front()->as<uf::Object>();

	auto& transform = model.getComponent<pod::Transform<>>();

	transform = player.getComponent<pod::Transform<>>();
	transform.scale = this->getComponent<pod::Transform<>>().scale;
//	transform.scale.x = metadata["system"]["transform"]["scale"][0].as<float>();
//	transform.scale.y = metadata["system"]["transform"]["scale"][1].as<float>();
//	transform.scale.z = metadata["system"]["transform"]["scale"][2].as<float>();
}

void ext::PlayerModelBehavior::render( uf::Object& self ){
	if ( this->getChildren().size() != 1 ) return;
	auto& metadata = this->getComponent<uf::Serializer>();
	if ( !metadata["hide player model"].as<bool>() ) return;
	
	auto& scene = uf::scene::getCurrentScene();
	auto& controller = scene.getController();
	auto& player = this->getParent().as<uf::Object>();
	auto& model = this->getChildren().front()->as<uf::Object>();
	auto& transform = model.getComponent<pod::Transform<>>();
	if ( player.getUid() == controller.getUid() ) {
		transform.scale = { 0, 0, 0 };
	} else {
		transform.scale = this->getComponent<pod::Transform<>>().scale;
	//	transform.scale = { 0.07, 0.07, 0.07 };
	//	transform.scale.x = metadata["system"]["transform"]["scale"][0].as<float>();
	//	transform.scale.y = metadata["system"]["transform"]["scale"][1].as<float>();
	//	transform.scale.z = metadata["system"]["transform"]["scale"][2].as<float>();
	}
}
void ext::PlayerModelBehavior::destroy( uf::Object& self ){

}
#undef this