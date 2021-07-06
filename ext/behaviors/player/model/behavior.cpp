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
#include <uf/utils/mesh/mesh.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/renderer/renderer.h>

UF_BEHAVIOR_REGISTER_CPP(ext::PlayerModelBehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::PlayerModelBehavior, ticks = true, renders = false, multithread = false)
#define this ((uf::Scene*) &self)
void ext::PlayerModelBehavior::initialize( uf::Object& self ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& controller = scene.getController();
	auto& controllerTransform = controller.getComponent<pod::Transform<>>();
	auto& transform = this->getComponent<pod::Transform<>>();
	auto& metadata = this->getComponent<ext::PlayerModelBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();
	transform.reference = &controllerTransform;

	metadata.reference = &controllerTransform;

	this->addHook( "object:UpdateMetadata.%UID%", [&](){
		metadata.deserialize(self, metadataJson);
	});
	metadata.deserialize(self, metadataJson);
}
void ext::PlayerModelBehavior::tick( uf::Object& self ) {
	auto& metadata = this->getComponent<ext::PlayerModelBehavior::Metadata>();
	if ( !metadata.hide || metadata.set || this->getChildren().size() != 1 ) return;
	auto& metadataJson = this->getComponent<uf::Serializer>();
#if UF_ENTITY_METADATA_USE_JSON
	metadata.deserialize();
#endif
	this->process([&](uf::Entity* entity){
		if ( !entity->hasComponent<uf::Graphic>() ) return;
		auto& graphic = entity->getComponent<uf::Graphic>();
		auto& pipeline = graphic.getPipeline();
		pipeline.metadata.process = false;
		metadata.set = true;
	});
	metadata.set = true;
}

void ext::PlayerModelBehavior::render( uf::Object& self ){}
void ext::PlayerModelBehavior::destroy( uf::Object& self ){}
void ext::PlayerModelBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ){
	auto& transform = this->getComponent<pod::Transform<>>();

	/*this->*/track = serializer["track"].as<bool>();
	/*this->*/hide = serializer["hide"].as<bool>();
	/*this->*/scale = transform.scale;

	transform.reference = /*this->*/track ? /*this->*/reference : NULL;
}
void ext::PlayerModelBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ){
	serializer["track"] = /*this->*/track;
	serializer["hide"] = /*this->*/hide;
}
#undef this