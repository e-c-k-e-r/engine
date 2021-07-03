#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>
#include <uf/engine/instantiator/instantiator.h>

UF_BEHAVIOR_ENTITY_CPP_BEGIN(uf::Entity)
UF_BEHAVIOR_TRAITS_CPP(uf::EntityBehavior, ticks = true, renders = false, multithread = false)
#define this ((uf::Entity*) &self)
void uf::EntityBehavior::initialize( uf::Object& self ) {
	if ( !this->isValid() ) this->setUid();
}

void uf::EntityBehavior::tick( uf::Object& self ) {
//	if ( !uf::scene::useGraph ) for ( uf::Entity* kv : this->getChildren() ) if ( kv->isValid() ) kv->tick();
}
void uf::EntityBehavior::render( uf::Object& self ) {
//	if ( !uf::scene::useGraph ) for ( uf::Entity* kv : this->getChildren() ) if ( kv->isValid() ) kv->render();
}
void uf::EntityBehavior::destroy( uf::Object& self ) {
	for ( uf::Entity* kv : this->getChildren() ) {
		if ( !kv->isValid() ) continue;
		kv->destroy();
		if ( uf::Entity::deleteChildrenOnDestroy ) delete kv;
	}
	this->getChildren().clear();
	if ( this->hasParent() ) this->getParent().removeChild(*this);
	this->unsetUid();
	if ( uf::Entity::deleteComponentsOnDestroy ) this->destroyComponents();
}
#undef this
UF_BEHAVIOR_ENTITY_CPP_END(Entity)