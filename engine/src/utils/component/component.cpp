#include <uf/utils/component/component.h>

UF_API uf::MemoryPool uf::component::memoryPool;
uf::Component::~Component() {
	this->destroyComponents();
}

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