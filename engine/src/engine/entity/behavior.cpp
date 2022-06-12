#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>
#include <uf/engine/instantiator/instantiator.h>

UF_BEHAVIOR_ENTITY_CPP_BEGIN(uf::Entity)
UF_BEHAVIOR_TRAITS_CPP(uf::EntityBehavior, ticks = false, renders = false, multithread = false)
#define this ((uf::Entity*) &self)
void uf::EntityBehavior::initialize( uf::Object& self ) {
	if ( !this->isValid() ) this->setUid();
}
void uf::EntityBehavior::tick( uf::Object& self ) {}
void uf::EntityBehavior::render( uf::Object& self ) {} 
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
void uf::EntityBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ) {}
void uf::EntityBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ) {}
#undef this
UF_BEHAVIOR_ENTITY_CPP_END(Entity)