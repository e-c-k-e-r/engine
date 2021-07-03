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
#include <uf/utils/graphic/mesh.h>
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

	metadata.serialize = [&]() {
		metadataJson["track"] = metadata.track;
		metadataJson["hide"] = metadata.hide;
	};
	metadata.deserialize = [&](){
		metadata.track = metadataJson["track"].as<bool>();
		metadata.hide = metadataJson["hide"].as<bool>();
		metadata.scale = transform.scale;

		transform.reference = metadata.track ? &controllerTransform : NULL;
	};
	this->addHook( "object:UpdateMetadata.%UID%", metadata.deserialize);
	this->addHook( "object:Reload.%UID%", metadata.deserialize);
	metadata.deserialize();
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
#undef this