#include <uf/utils/component/component.h>

UF_API uf::MemoryPool uf::component::memoryPool;
uf::Component::~Component() {
	for ( auto& kv : this->m_container ) {
		pod::Component& component = kv.second;
		uf::userdata::destroy(uf::component::memoryPool, component.userdata);
	/*
		if ( uf::component::memoryPool.size() > 0 ) {
			uf::userdata::destroy(uf::component::memoryPool, component.userdata);
		} else {
			uf::userdata::destroy(component.userdata);
		}
	*/
	}
}

#include <uf/utils/serialize/serializer.h>
// override serializers
template<>
uf::Serializer uf::Serializer::toBase64( const pod::Component& input ) {

}