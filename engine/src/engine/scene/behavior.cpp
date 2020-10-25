#include <uf/engine/scene/scene.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/renderer/renderer.h>

UF_BEHAVIOR_ENTITY_CPP_BEGIN(Scene)
#define this ((uf::Scene*) &self)
void uf::SceneBehavior::initialize( uf::Object& self ) {
	uf::renderer::scenes.push_back(this);
	uf::renderer::states::rebuild = true;

	this->addHook( "system:Renderer.QueueRebuild", [&](const std::string& event)->std::string{	
		uf::renderer::states::rebuild = true;
		return "true";
	});
	this->addHook( "system:Destroy", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		size_t uid = json["uid"].asUInt64();
		if ( uid <= 0 ) return "false";
		auto* target = this->findByUid(uid);
		if ( !target ) return "false";
		target->destroy();
		delete target;

		this->queueHook("system:Renderer.QueueRebuild");

		return "true";
	});
}
void uf::SceneBehavior::tick( uf::Object& self ) {
}
void uf::SceneBehavior::render( uf::Object& self ) {
}
void uf::SceneBehavior::destroy( uf::Object& self ) {
	auto it = std::find(uf::renderer::scenes.begin(), uf::renderer::scenes.end(), this);
	if ( it != uf::renderer::scenes.end() ) uf::renderer::scenes.erase(it);
	uf::renderer::states::rebuild = true;
}
#undef self
UF_BEHAVIOR_ENTITY_CPP_END(Scene)