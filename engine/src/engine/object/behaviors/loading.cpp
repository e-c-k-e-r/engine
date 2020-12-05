#include <uf/engine/object/behaviors/loading.h>

#include <uf/engine/object/object.h>
#include <uf/engine/asset/asset.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/time/time.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/graphic/mesh.h>
#include <uf/ext/gltf/gltf.h>

UF_BEHAVIOR_REGISTER_CPP(uf::LoadingBehavior)
#define this (&self)
void uf::LoadingBehavior::initialize( uf::Object& self ) {
	auto& metadata = this->getComponent<uf::Serializer>();
	this->addHook( "system:Load.Finished.%UID%", [&](ext::json::Value& json){
		metadata["system"]["loaded"] = true;
		this->removeBehavior<uf::LoadingBehavior>();

		auto& parent = this->getParent();
		auto& scene = uf::scene::getCurrentScene();
		if ( parent.getUid() != scene.getUid() ) {
			scene.moveChild(*this);
			uf::Serializer payload;
			payload["uid"] = parent.getUid();
			parent.getParent().removeChild(parent);
			
			std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
				if ( !entity || entity->getUid() == 0 || !entity->hasComponent<pod::Transform<>>() ) return;
				auto& transform = entity->getComponent<pod::Transform<>>();
				transform.scale = { 0, 0, 0 };
				entity->render();
			};
			parent.process(filter);
			
			scene.queueHook("system:Destroy", payload);
		}
		return;
	});
}
void uf::LoadingBehavior::destroy( uf::Object& self ) {

}
void uf::LoadingBehavior::tick( uf::Object& self ) {
	auto& metadata = this->getComponent<uf::Serializer>();
	if ( metadata["system"]["loaded"].as<bool>() ) return;
	size_t loading = 0;
	size_t loaded = 1;
	std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
		if ( !entity || entity->getUid() == 0 || !entity->hasComponent<uf::Serializer>() ) return;
		auto& metadata = entity->getComponent<uf::Serializer>();
		if ( metadata["system"]["load"]["ignore"].is<bool>() ) return;
		if ( ext::json::isNull( metadata["system"]["load"] ) ) return;
		++loading;
		if ( metadata["system"]["load"]["progress"].as<int>() < metadata["system"]["load"]["total"].as<int>() ) return;
		++loaded;
	};
	this->process(filter);
	if ( loading == loaded ) {
		metadata["system"]["loaded"] = true;
		this->callHook("system:Load.Finished.%UID%");
	}
/*
	auto& metadata = this->getComponent<uf::Serializer>();
	if ( !metadata["system"]["loaded"].as<bool>() ) {
		size_t loading = 0;
		size_t loaded = 0;
		std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
			if ( !entity || entity->getUid() == 0 || !entity->hasComponent<uf::Serializer>() ) return;
			auto& metadata = entity->getComponent<uf::Serializer>();
			if ( ext::json::isNull( metadata["system"]["load"] ) ) return;
			++loading;
			if ( metadata["system"]["load"]["progress"].as<float>() < 1.0f ) return;
			++loaded;
		};
		this->process(filter);

		if ( loading == loaded ) {
			this->callHook("system:Load.Finished.%UID%");
		}
		auto& metadata = this->getComponent<uf::Serializer>();
		if ( metadata["system"]["load"]["progress"].as<float>() >= 1.0f ) {
			this->callHook("system:Load.Finished.%UID%");
		}
	}
*/
}
void uf::LoadingBehavior::render( uf::Object& self ) {

}
#undef this