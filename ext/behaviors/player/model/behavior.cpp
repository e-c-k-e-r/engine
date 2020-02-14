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
	auto& scene = uf::scene::getCurrentScene();
	auto& controller = scene.getController();
	auto& controllerTransform = controller.getComponent<pod::Transform<>>();
	auto& transform = this->getComponent<pod::Transform<>>();
	auto& metadata = this->getComponent<uf::Serializer>();
	transform.reference = &controllerTransform;
}
void ext::PlayerModelBehavior::tick( uf::Object& self ) {
	if ( this->getChildren().size() != 1 ) return;
	auto& metadata = this->getComponent<uf::Serializer>();
	auto& scene = uf::scene::getCurrentScene();
	auto& controller = scene.getController();
	auto& controllerTransform = controller.getComponent<pod::Transform<>>();
	auto& transform = this->getComponent<pod::Transform<>>();
	if ( metadata["track via reference"].as<bool>() )
		transform.reference = metadata["track player"].as<bool>() ? &controllerTransform : NULL;
	else if ( metadata["track player"].as<bool>() ) {
		transform.position = controllerTransform.position;
		transform.orientation = controllerTransform.orientation;
	}
}

void ext::PlayerModelBehavior::render( uf::Object& self ){
	if ( this->getChildren().size() != 1 ) return;
	auto& metadata = this->getComponent<uf::Serializer>();
	if ( !metadata["hide player model"].as<bool>() ) return;
	
	auto& scene = uf::scene::getCurrentScene();
	auto& controller = scene.getController();
	auto& player = this->getParent().as<uf::Object>();
	auto& model = this->getChildren().front()->as<uf::Object>();
	auto& transform = this->getComponent<pod::Transform<>>();

	if ( player.getUid() == controller.getUid() ) {
		transform.scale = { 0, 0, 0 };
	} else {
		transform.scale = uf::vector::decode( metadata["system"]["transform"]["scale"], pod::Vector3f{1,1,1} );
	}
}
void ext::PlayerModelBehavior::destroy( uf::Object& self ){

}
#undef this