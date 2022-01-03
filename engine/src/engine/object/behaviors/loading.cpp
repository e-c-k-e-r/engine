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
	this->addHook( "system:Load.Finished.%UID%", [&](){
		auto& scene = uf::scene::getCurrentScene();
		auto& parent = this->getParent();
		auto& metadata = this->getComponent<uf::ObjectBehavior::Metadata>();

		if ( !metadata.system.loaded ) {
			UF_MSG_ERROR("invalid hook invocation: system:Load.Finished:" << this->getUid());
			return;
		}

		// unbind loading behavior
		this->removeBehavior(pod::Behavior{.type = TYPE(uf::LoadingBehavior::Metadata)});

		// 
		if ( parent.getUid() == scene.getUid() ) return;

		// re-parent to main scene
		scene.moveChild(*this);
		// erase previous parent
		parent.getParent().removeChild(parent);
		// ???
		parent.process([&]( uf::Entity* entity ) {
			if ( !entity || !entity->isValid() || !entity->hasComponent<pod::Transform<>>() ) return;
			auto& transform = entity->getComponent<pod::Transform<>>();
			transform.scale = { 0, 0, 0 };
			entity->render();
		});	
		// queue destruction of parent
		pod::payloads::Entity payload;
		payload.uid = parent.getUid();
		payload.pointer = (uf::Object*) &parent;
		scene.queueHook("system:Destroy", payload);
	});
}
void uf::LoadingBehavior::tick( uf::Object& self ) {
	auto& metadata = this->getComponent<uf::ObjectBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();

	if ( metadata.system.loaded ) return;

	size_t loading = 0;
	size_t loaded = 1;

	this->process([&]( uf::Entity* entity ) {
		if ( !entity || !entity->isValid() || !entity->hasComponent<uf::ObjectBehavior::Metadata>() ) return;
		auto& metadata = entity->getComponent<uf::ObjectBehavior::Metadata>();

		if ( metadata.system.load.ignore ) return;

		++loading;
		// loading not finished
		if ( metadata.system.load.progress < metadata.system.load.total ) return;
		++loaded;
	});

	if ( loading == loaded ) {
		metadata.system.loaded = true;
		this->callHook("system:Load.Finished.%UID%");
	}
}
void uf::LoadingBehavior::render( uf::Object& self ) {}
void uf::LoadingBehavior::destroy( uf::Object& self ) {}
void uf::LoadingBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ) {}
void uf::LoadingBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ) {}
#undef this