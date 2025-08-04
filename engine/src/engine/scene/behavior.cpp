#include <uf/engine/scene/scene.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/renderer/renderer.h>
#include <uf/utils/math/physics.h>
#include <uf/engine/graph/graph.h>

UF_BEHAVIOR_ENTITY_CPP_BEGIN(uf::Scene)
UF_BEHAVIOR_TRAITS_CPP(uf::SceneBehavior, ticks = false, renders = false, multithread = false)
#define this ((uf::Scene*) &self)
void uf::SceneBehavior::initialize( uf::Object& self ) {
	uf::renderer::states::rebuild = true;

	this->addHook( "system:Renderer.QueueRebuild", [&](){
	//	uf::renderer::states::resized = true;
		uf::renderer::states::rebuild = true;
	});
	this->addHook( "system:Destroy", [&](pod::payloads::Entity& payload){
		if ( !payload.pointer ) {
			if ( payload.uid <= 0 ) return;
			payload.pointer = (uf::Object*) this->findByUid(payload.uid);
			if ( !payload.pointer ) return;
		}
		payload.pointer->queueDeletion();
	});

	uf::physics::initialize( self );
	uf::graph::initialize( self );

	auto& metadata = this->getComponent<uf::SceneBehavior::Metadata>();
}
void uf::SceneBehavior::tick( uf::Object& self ) {}
void uf::SceneBehavior::render( uf::Object& self ) {}
void uf::SceneBehavior::destroy( uf::Object& self ) {
	uf::renderer::states::rebuild = true;
}
void uf::SceneBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ) {}
void uf::SceneBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ) {}
#undef self
UF_BEHAVIOR_ENTITY_CPP_END(Scene)