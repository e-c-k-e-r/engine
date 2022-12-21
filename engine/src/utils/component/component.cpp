#include <uf/utils/component/component.h>

uf::MemoryPool uf::component::memoryPool;
uf::Component::~Component() {
	this->destroyComponents();
}

#if UF_COMPONENT_POINTERED_USERDATA
pod::PointeredUserdata uf::Component::addComponent( pod::PointeredUserdata& userdata ) {
	pod::Component::id_t id = userdata.type;
	pod::Component& component = this->m_container[id];
	component.id = id;
	component.userdata = uf::pointeredUserdata::copy( userdata );
	return component.userdata;
}
pod::PointeredUserdata uf::Component::moveComponent( pod::PointeredUserdata& userdata ) {
	pod::Component::id_t id = userdata.type;
	pod::Component& component = this->m_container[id];
	component.id = id;
	component.userdata = userdata;
	userdata.data = NULL;
	return component.userdata;
}
#else
pod::Userdata* uf::Component::addComponent( pod::Userdata* userdata ) {
	pod::Component::id_t id = userdata->type;
	pod::Component& component = this->m_container[id];
	component.id = id;
	component.userdata = uf::userdata::copy( userdata );
	return component.userdata;
}
pod::Userdata* uf::Component::moveComponent( pod::Userdata*& userdata ) {
	pod::Component::id_t id = userdata->type;
	pod::Component& component = this->m_container[id];
	component.id = id;
	component.userdata = userdata;
	userdata = NULL;
	return component.userdata;
}
#endif

void uf::Component::destroyComponents() {
	for ( auto& kv : this->m_container ) {
		pod::Component& component = kv.second;
	#if UF_COMPONENT_POINTERED_USERDATA
		uf::pointeredUserdata::destroy(uf::component::memoryPool, component.userdata);
	#else
		uf::userdata::destroy(uf::component::memoryPool, component.userdata);
	#endif
	}
	this->m_container.clear();
}

#include <uf/utils/serialize/serializer.h>
// override serializers
template<>
uf::Serializer uf::Serializer::toBase64( const pod::Component& input ) {
	uf::Serializer serialized;
	return serialized;
}