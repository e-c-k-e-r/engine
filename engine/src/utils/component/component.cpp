#include <uf/utils/component/component.h>

uf::Component::~Component() {
	for ( auto& kv : this->m_container ) {
		pod::Component& component = kv.second;
		uf::userdata::destroy(component.userdata);
	}
}