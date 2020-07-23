#include <uf/utils/component/component.h>

uf::Component::~Component() {
	for ( auto& kv : this->m_container ) {
		pod::Component& component = kv.second;
		uf::userdata::destroy(component.userdata);
	}
}

#include <uf/utils/serialize/serializer.h>
// override serializers
template<>
uf::Serializer uf::Serializer::toBase64( const pod::Component& input ) {

}