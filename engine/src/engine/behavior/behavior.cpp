#include <uf/engine/behavior/behavior.h>

void uf::Behaviors::addBehavior( const pod::Behavior& behavior ) {
	std::cout << "[!!!] behavior attached" << std::endl;
	this->m_behaviors.emplace_back(behavior);
}

void uf::Behaviors::initialize() {
	for ( auto it = this->m_behaviors.rbegin(); it != this->m_behaviors.rend(); ++it ) {
		std::cout << "[!!!] behavior called: initialize" << std::endl;
		it->initialize();
	}
}
void uf::Behaviors::tick() {
	for ( auto it = this->m_behaviors.rbegin(); it != this->m_behaviors.rend(); ++it )
		it->tick();
}
void uf::Behaviors::render() {
	for ( auto it = this->m_behaviors.rbegin(); it != this->m_behaviors.rend(); ++it )
		it->render();
}
void uf::Behaviors::destroy() {
	for ( auto it = this->m_behaviors.rbegin(); it != this->m_behaviors.rend(); ++it ) {
		std::cout << "[!!!] behavior called: destroy" << std::endl;
		it->destroy();
	}
}