#include <uf/utils/serialize/serializable.h>

 void uf::Serializable::deserialize( const Serializer& input ) {
 	this->deserialize((const Serializer::input_t&) input);
 }

void uf::Serializable::operator<<( const Serializer::input_t& input ) {
	this->deserialize(input);
}
void uf::Serializable::operator>>( Serializer::output_t& output ) {
	output = this->serialize();
}