#include <uf/engine/behavior/behavior.h>
#include <uf/engine/object/object.h>

bool uf::Behaviors::hasBehavior( const pod::Behavior& target ) {
	for ( auto& behavior : this->m_behaviors ) {
		if ( behavior.type == target.type ) return true;
	}
	return false;
}
void uf::Behaviors::addBehavior( const pod::Behavior& behavior ) {
	if ( hasBehavior( behavior ) ) return;
	this->m_behaviors.emplace_back(behavior);
}

void uf::Behaviors::initialize() {
	uf::Object& self = *((uf::Object*) this);
	for ( auto& behavior : this->m_behaviors ) behavior.initialize(self);
}
void uf::Behaviors::tick() {
	uf::Object& self = *((uf::Object*) this);
	for ( auto it = this->m_behaviors.rbegin(); it != this->m_behaviors.rend(); ++it ) {
		it->tick(self);
	}
}
void uf::Behaviors::render() {
	uf::Object& self = *((uf::Object*) this);
	for ( auto it = this->m_behaviors.rbegin(); it != this->m_behaviors.rend(); ++it ) {
		it->render(self);
	}
}
void uf::Behaviors::destroy() {
	uf::Object& self = *((uf::Object*) this);
	for ( auto it = this->m_behaviors.rbegin(); it != this->m_behaviors.rend(); ++it ) {
		it->destroy(self);
	}
}