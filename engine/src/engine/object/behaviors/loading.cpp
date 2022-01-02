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
#include <uf/utils/mesh/mesh.h>
#include <uf/ext/gltf/gltf.h>

UF_BEHAVIOR_REGISTER_CPP(uf::LoadingBehavior)
UF_BEHAVIOR_TRAITS_CPP(uf::LoadingBehavior, ticks = true, renders = false, multithread = false)
#define this (&self)
void uf::LoadingBehavior::initialize( uf::Object& self ) {
	auto& metadata = this->getComponent<uf::Serializer>();
	this->addHook( "system:Load.Finished.%UID%", [&](){
		metadata["system"]["loaded"] = true;
	//	this->removeBehavior(pod::Behavior{.type = uf::LoadingBehavior::type});
		this->removeBehavior(pod::Behavior{.type = TYPE(uf::LoadingBehavior::Metadata)});

		auto& parent = this->getParent();
		auto& scene = uf::scene::getCurrentScene();
		if ( parent.getUid() != scene.getUid() ) {
			scene.moveChild(*this);
			ext::json::Value payload;
			payload["uid"] = parent.getUid();
			parent.getParent().removeChild(parent);
			parent.process([&]( uf::Entity* entity ) {
				if ( !entity || !entity->isValid() || !entity->hasComponent<pod::Transform<>>() ) return;
				auto& transform = entity->getComponent<pod::Transform<>>();
				transform.scale = { 0, 0, 0 };
				entity->render();
			});	
			scene.queueHook("system:Destroy", payload);
		}
		return;
	});
}
void uf::LoadingBehavior::tick( uf::Object& self ) {
	auto& metadata = this->getComponent<uf::Serializer>();
	if ( metadata["system"]["loaded"].as<bool>() ) return;
	size_t loading = 0;
	size_t loaded = 1;
	this->process([&]( uf::Entity* entity ) {
		if ( !entity || !entity->isValid() || !entity->hasComponent<uf::Serializer>() ) return;
		auto& metadata = entity->getComponent<uf::Serializer>();
		if ( metadata["system"]["load"]["ignore"].is<bool>() ) return;
		if ( ext::json::isNull( metadata["system"]["load"] ) ) return;
		++loading;
		if ( metadata["system"]["load"]["progress"].as<int>() < metadata["system"]["load"]["total"].as<int>() ) return;
		++loaded;
	});
	if ( loading == loaded ) {
		metadata["system"]["loaded"] = true;
		this->callHook("system:Load.Finished.%UID%");
	}
}
void uf::LoadingBehavior::render( uf::Object& self ) {}
void uf::LoadingBehavior::destroy( uf::Object& self ) {}
void uf::LoadingBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ) {}
void uf::LoadingBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ) {}
#undef this