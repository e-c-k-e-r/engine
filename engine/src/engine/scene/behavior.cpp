#include <uf/engine/scene/scene.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/renderer/renderer.h>

UF_BEHAVIOR_ENTITY_CPP_BEGIN(uf::Scene)
UF_BEHAVIOR_TRAITS_CPP(uf::SceneBehavior, ticks = false, renders = false, multithread = false)
#define this ((uf::Scene*) &self)
void uf::SceneBehavior::initialize( uf::Object& self ) {
	uf::renderer::states::rebuild = true;

	this->addHook( "system:Renderer.QueueRebuild", [&](ext::json::Value& json){
		uf::renderer::states::rebuild = true;
	});
	this->addHook( "system:Destroy", [&](ext::json::Value& json){
		size_t uid = json["uid"].as<size_t>();
		if ( uid <= 0 ) return;
		auto* target = this->findByUid(uid);
		if ( !target ) return;
		target->destroy();
		delete target;

		this->queueHook("system:Renderer.QueueRebuild");
	});
}
void uf::SceneBehavior::tick( uf::Object& self ) {}
void uf::SceneBehavior::render( uf::Object& self ) {}
void uf::SceneBehavior::destroy( uf::Object& self ) {
	uf::renderer::states::rebuild = true;
}
#undef self
UF_BEHAVIOR_ENTITY_CPP_END(Scene)