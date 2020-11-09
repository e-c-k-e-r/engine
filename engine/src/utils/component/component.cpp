#include <uf/utils/component/component.h>

UF_API uf::MemoryPool uf::component::memoryPool;
uf::Component::~Component() {
	this->destroyComponents();
}

void uf::Component::destroyComponents() {
	for ( auto& kv : this->m_container ) {
		pod::Component& component = kv.second;
		uf::userdata::destroy(uf::component::memoryPool, component.userdata);
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