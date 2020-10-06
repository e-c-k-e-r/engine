#include <uf/engine/entity/entity.h>

UF_BEHAVIOR_ENTITY_CPP_BEGIN(Entity)
#define this ((uf::Entity*) &self)
void uf::EntityBehavior::initialize( uf::Object& self ) {
	if ( this->m_uid == 0 )
		this->m_uid = ++uf::Entity::uids;
}
void uf::EntityBehavior::tick( uf::Object& self ) {
	for ( uf::Entity* kv : this->m_children ) {
		if ( !kv ) continue;
		if ( kv->getUid() == 0 ) continue;
		kv->tick();
	}
}
void uf::EntityBehavior::render( uf::Object& self ) {
	for ( uf::Entity* kv : this->m_children ) {
		if ( !kv ) continue;
		if ( kv->getUid() == 0 ) continue;
		kv->render();
	}
}
void uf::EntityBehavior::destroy( uf::Object& self ) {
	for ( uf::Entity* kv : this->m_children ) {
		if ( !kv ) continue;
		if ( kv->getUid() == 0 ) continue;
		kv->destroy();
		delete kv;
	}
	this->m_children.clear();
	this->m_uid = 0;
	if ( this->hasParent() ) this->getParent().removeChild(*this);
}
#undef this
UF_BEHAVIOR_ENTITY_CPP_END(Entity)