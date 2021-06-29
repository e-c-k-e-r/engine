#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>
#include <uf/engine/instantiator/instantiator.h>
#include <uf/utils/io/iostream.h>

uf::Entity uf::Entity::null;
std::size_t uf::Entity::uids = 0;
uf::MemoryPool uf::Entity::memoryPool;
bool uf::Entity::deleteChildrenOnDestroy = false;
bool uf::Entity::deleteComponentsOnDestroy = false;
uf::Entity::~Entity(){
	this->destroy();
}
bool uf::Entity::isValid() const {
	if ( uf::Entity::memoryPool.size() > 0 && !uf::Entity::memoryPool.exists((void*) this) ) return false;
	return this != NULL && (0 < this->m_uid && this->m_uid <= uf::Entity::uids);
}
void uf::Entity::setUid() {
	uf::scene::invalidateGraphs();
	this->m_uid = ++uf::Entity::uids;
}
void uf::Entity::unsetUid() {
	uf::scene::invalidateGraphs();
	this->m_uid = 0;
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
uf::Entity& uf::Entity::addChild( uf::Entity& child ) {
	this->m_children.push_back(&child);
	child.setParent(*this);
	return child;
}
void uf::Entity::addChild( uf::Entity* child ) {
	if ( !child ) return;
	this->m_children.push_back(child);
	child->setParent(*this);
}
void uf::Entity::removeChild( uf::Entity& child ) {
	auto it = std::find( this->m_children.begin(), this->m_children.end(), &child );
	if ( it == this->m_children.end() ) return;
	this->m_children.erase(it);
	child.setParent();
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
const uf::stl::string& uf::Entity::getName() const {
	return this->m_name;
}
void uf::Entity::setName( const uf::stl::string& name ) {
	this->m_name = name;
}
std::size_t uf::Entity::getUid() const {
	return this->m_uid;
}

void* uf::Entity::operator new(size_t size, const uf::stl::string& type ) {
	return type != "" && size == sizeof(uf::Entity) ? &uf::instantiator::instantiate( type ) : uf::instantiator::alloc( size );
}
void uf::Entity::operator delete( void* pointer ) {
	uf::instantiator::free( (uf::Entity*) pointer );
}

uf::Entity* uf::Entity::findByName( const uf::stl::string& name ) {
	for ( uf::Entity* entity : this->m_children ) {
		if ( entity->getName() == name ) return entity;
		uf::Entity* p = entity->findByName(name);
		if ( p ) return p;
	}
	return (uf::Entity*) NULL;
};
uf::Entity* uf::Entity::findByUid( std::size_t id ) {
	for ( uf::Entity* entity : this->m_children ) {
		if ( entity->getUid() == id ) return entity;
		uf::Entity* p = entity->findByUid(id);
		if ( p ) return p;
	}
	return (uf::Entity*) NULL;
};

const uf::Entity* uf::Entity::findByName( const uf::stl::string& name ) const {
	for ( uf::Entity* entity : this->m_children ) {
		if ( entity->getName() == name ) return entity;
		uf::Entity* p = entity->findByName(name);
		if ( p ) return p;
	}
	return (uf::Entity*) NULL;
};
const uf::Entity* uf::Entity::findByUid( std::size_t id ) const {
	for ( const uf::Entity* entity : this->m_children ) {
		if ( entity->getUid() == id ) return entity;
		const uf::Entity* p = entity->findByUid(id);
		if ( p ) return p;
	}
	return (const uf::Entity*) NULL;
};

void uf::Entity::process( std::function<void(uf::Entity*)> fn ) {
	fn(this);
	for ( uf::Entity* entity : this->m_children ) {
		if ( !entity ) continue;
		entity->process(fn);
	}
}
void uf::Entity::process( std::function<void(uf::Entity*, int)> fn, int depth ) {
	fn(this, depth);
	for ( uf::Entity* entity : this->m_children ) {
		if ( !entity ) continue;
		entity->process(fn, depth + 1);
	}
}
uf::Entity* uf::Entity::globalFindByUid( size_t uid ) {
	for ( auto& allocation : uf::Entity::memoryPool.allocations() ) {
		uf::Entity* entity = (uf::Entity*) allocation.pointer;
		if ( entity->getUid() == uid ) return entity;
	}
	return NULL;
}
uf::Entity* uf::Entity::globalFindByName( const uf::stl::string& name ) {
	for ( auto& allocation : uf::Entity::memoryPool.allocations() ) {
		uf::Entity* entity = (uf::Entity*) allocation.pointer;
		if ( !entity->isValid() ) continue;
		if ( entity->getName() == name ) return entity;
	}
	return NULL;
}