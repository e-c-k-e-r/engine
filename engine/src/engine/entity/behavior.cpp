#include <uf/engine/entity/entity.h>
#include <uf/engine/instantiator/instantiator.h>

UF_BEHAVIOR_ENTITY_CPP_BEGIN(uf::Entity)
uf::Entity::Entity() UF_BEHAVIOR_ENTITY_CPP_ATTACH(uf::Entity)
#define this ((uf::Entity*) &self)
void uf::EntityBehavior::initialize( uf::Object& self ) {
	if ( !this->isValid() ) this->m_uid = ++uf::Entity::uids;
	// sanitize children
/*
	for ( auto& kv : this->m_children ) {
		if ( !uf::instantiator::valid(kv) ) {
			std::cout << "FOUND INVALID CHILD IN " << this->getName() << ": " << this->getUid() << ": " << kv << std::endl;
			kv = NULL;
		}
	}
*/
}
void uf::EntityBehavior::tick( uf::Object& self ) {
	for ( uf::Entity* kv : this->m_children ) {
		if ( !kv ) continue;
		if ( !kv->isValid() ) continue;
		kv->tick();
	}
}
void uf::EntityBehavior::render( uf::Object& self ) {
	for ( uf::Entity* kv : this->m_children ) {
		if ( !kv ) continue;
		if ( !kv->isValid() ) continue;
		kv->render();
	}
}
void uf::EntityBehavior::destroy( uf::Object& self ) {
	for ( uf::Entity* kv : this->m_children ) {
		if ( !kv ) continue;
		if ( !kv->isValid() ) continue;
		kv->destroy();
		if ( uf::Entity::deleteChildrenOnDestroy ) delete kv;
	}
	this->m_children.clear();
	this->m_uid = 0;
	if ( this->hasParent() )
		this->getParent().removeChild(*this);

	if ( uf::Entity::deleteComponentsOnDestroy ) this->destroyComponents();
}
#undef this
UF_BEHAVIOR_ENTITY_CPP_END(Entity)