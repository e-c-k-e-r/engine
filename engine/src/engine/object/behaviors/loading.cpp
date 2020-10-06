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

UF_BEHAVIOR_REGISTER_CPP(LoadingBehavior)
#define this (&self)
void uf::LoadingBehavior::initialize( uf::Object& self ) {	
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();

	size_t target = 0;
	for ( int i = 0; i < metadata["system"]["assets"].size(); ++i ) {
		std::string filename = metadata["system"]["assets"][i].asString();
		if ( uf::io::extension(filename) != "json" ) ++target;
	}
	if ( target > 0 ) {
		metadata["system"]["load"]["progress"] = 0;
		metadata["system"]["load"]["total"] = target;
	}
	
	this->addHook( "system:Load.Finished.%UID%", [&](const std::string& event)->std::string{
		uf::Serializer json = event;
		

		metadata["system"]["loaded"] = true;
		this->removeBehavior<uf::LoadingBehavior>();
		// uf::instantiator::unbind("LoadingBehavior", *this);
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
	/*
		auto& parent = this->getParent();
		// unbind all children
		for ( auto* child : this->getChildren() ) {
			this->removeChild(*child);
			parent.addChild(*child);
		}
		auto& scene = uf::scene::getCurrentScene();
		if ( parent.getUid() != scene.getUid() ) {
			uf::Serializer payload;
			payload["uid"] = parent.getUid();
			parent.removeChild(*this);
			scene.queueHook("system:Destroy", payload);
		}
	*/	

		return "true";
	});
	this->addHook( "asset:Parsed.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;

		float portion = 1.0f / metadata["system"]["load"]["total"].asFloat();
		auto& progress = metadata["system"]["load"]["progress"];
		progress = progress.asFloat() + portion;

		return "true";
	});
}
void uf::LoadingBehavior::destroy( uf::Object& self ) {

}
void uf::LoadingBehavior::tick( uf::Object& self ) {
	auto& metadata = this->getComponent<uf::Serializer>();
	// if ( this->getChildren().empty() ) return;
	/* Check if we're a loading screen */ if ( !metadata["system"]["loaded"].asBool() ) {
		size_t loading = 0;
		size_t loaded = 0;
		std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
			if ( !entity || entity->getUid() == 0 || !entity->hasComponent<uf::Serializer>() ) return;
			auto& metadata = entity->getComponent<uf::Serializer>();
			if ( metadata["system"]["load"].isNull() ) return;
			++loading;
			if ( metadata["system"]["load"]["progress"].asFloat() < 1.0f ) return;
			++loaded;
		};
		this->process(filter);

		if ( loading == loaded ) {
			this->callHook("system:Load.Finished.%UID%");
		}
	}
}
void uf::LoadingBehavior::render( uf::Object& self ) {

}
#undef this