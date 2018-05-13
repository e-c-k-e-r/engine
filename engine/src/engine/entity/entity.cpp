#include <uf/engine/entity/entity.h>
#include <uf/utils/io/iostream.h>

uf::Entity uf::Entity::null;
uf::Entity::Entity(bool shouldInitialize){
	if ( shouldInitialize ) this->initialize();
}
uf::Entity::~Entity(){
	this->destroy();
}
void uf::Entity::setParent( uf::Entity& parent ) {
	this->m_parent = &parent;
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
void uf::Entity::initialize(){

}
void uf::Entity::destroy(){
	for ( uf::Entity* kv : this->m_children ) {
		kv->destroy();
		kv->setParent();
	}
	this->m_children.clear();
}
void uf::Entity::tick(){
	for ( uf::Entity* kv : this->m_children ) {
		kv->tick();
	}
}
void uf::Entity::render(){
	for ( uf::Entity* kv : this->m_children ) {
		kv->render();
	}
}