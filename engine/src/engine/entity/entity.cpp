#include <uf/engine/entity/entity.h>
#include <uf/utils/io/iostream.h>

uf::Entity uf::Entity::null;
std::vector<uf::Entity*> uf::Entity::entities;
std::size_t uf::Entity::uids = 0;

uf::Entity::Entity(bool shouldInitialize){
	if ( shouldInitialize ) this->initialize();
}
uf::Entity::~Entity(){
	this->destroy();
}
bool uf::Entity::hasParent() const {
	return this->m_parent;
}
void uf::Entity::setParent() {
	this->m_parent = NULL;
}
void uf::Entity::setParent( uf::Entity& parent ) {
	this->m_parent = &parent == &uf::Entity::null ? NULL : &parent;
}
void uf::Entity::addChild( uf::Entity& child ) {
	this->m_children.push_back(&child);
	child.setParent(*this);
}
void uf::Entity::removeChild( uf::Entity& child ) {
	for ( uf::Entity::container_t::iterator it = this->m_children.begin(); it != this->m_children.end(); ++it ) {
		uf::Entity* entity = *it;
		if ( &child == entity ) {
			*it = NULL;
			it = this->m_children.erase( it );
			child.setParent();
			return;
		}
	}
}
void uf::Entity::moveChild( uf::Entity& child ) {
	if ( !child.m_parent ) return;
	uf::Entity& parent = *child.m_parent;
	parent.removeChild( child );
	this->addChild( child );
}
uf::Entity::container_t& uf::Entity::getChildren() {
	return this->m_children;
}
const uf::Entity::container_t& uf::Entity::getChildren() const {
	return this->m_children;
}
const std::string& uf::Entity::getName() const {
	return this->m_name;
}
std::size_t uf::Entity::getUid() const {
	return this->m_uid;
}
void uf::Entity::initialize(){
	if ( this->m_uid == 0 ) {	
		uf::Entity::entities.push_back(this);
		this->m_uid = ++uf::Entity::uids;
	}
}
void uf::Entity::destroy(){
	for ( uf::Entity* kv : this->m_children ) {
		if ( !kv ) continue;
		if ( kv->getUid() == 0 ) continue;
		kv->destroy();
		kv->setParent();
	}
	this->m_children.clear();

	auto it = std::find( uf::Entity::entities.begin(), uf::Entity::entities.end(), this );
	if ( it != uf::Entity::entities.end() ) {
		*it = NULL;
	}
	this->m_uid = 0;
}
void uf::Entity::tick(){
	for ( uf::Entity* kv : this->m_children ) {
		if ( !kv ) continue;
		if ( kv->getUid() == 0 ) continue;
		kv->tick();
	}
}
void uf::Entity::render(){
	for ( uf::Entity* kv : this->m_children ) {
		if ( !kv ) continue;
		if ( kv->getUid() == 0 ) continue;
		kv->render();
	}
}

uf::Entity* uf::Entity::findByName( const std::string& name ) {
	for ( uf::Entity* entity : this->getChildren() ) {
		if ( entity->getName() == name ) return entity;
		uf::Entity* p = entity->findByName(name);
		if ( p ) return p;
	}
	return (uf::Entity*) NULL;
};
uf::Entity* uf::Entity::findByUid( std::size_t id ) {
	for ( uf::Entity* entity : this->getChildren() ) {
		if ( entity->getUid() == id ) return entity;
		uf::Entity* p = entity->findByUid(id);
		if ( p ) return p;
	}
	return (uf::Entity*) NULL;
};

const uf::Entity* uf::Entity::findByName( const std::string& name ) const {
	for ( uf::Entity* entity : this->getChildren() ) {
		if ( entity->getName() == name ) return entity;
		uf::Entity* p = entity->findByName(name);
		if ( p ) return p;
	}
	return (uf::Entity*) NULL;
};
const uf::Entity* uf::Entity::findByUid( std::size_t id ) const {
	for ( const uf::Entity* entity : this->getChildren() ) {
		if ( entity->getUid() == id ) return entity;
		const uf::Entity* p = entity->findByUid(id);
		if ( p ) return p;
	}
	return (const uf::Entity*) NULL;
};

uf::Entity* uf::Entity::globalFindByName( const std::string& name ) {
	for ( uf::Entity* e : uf::Entity::entities ) {
		if ( !e ) continue;
		if ( e->getUid() == 0 ) continue;
		if ( e->getName() == name ) return e;
	}
	return NULL;
}