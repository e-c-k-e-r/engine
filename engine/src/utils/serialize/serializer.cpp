#include <uf/utils/serialize/serializer.h>
#include <uf/utils/string/string.h>
#include <uf/utils/string/ext.h>

#include <uf/utils/io/iostream.h>

#include <sstream>
#include <fstream>
#include <functional>

uf::Serializer::Serializer( const std::string& str ) {
	this->deserialize(str);
}
uf::Serializer::Serializer( const Json::Value& json ) {
	try {
		*this = json;
	} catch (...) {
		// ignore parse errors
	}
}
uf::Serializer::output_t uf::Serializer::serialize() const {
	std::stringstream ss;
	ss << *this;
	return ss.str();
}
void uf::Serializer::deserialize( const std::string& str ) {
	if ( str != "" ) 
		try {
			std::stringstream(str) >> *this;
		} catch ( const std::exception& e ) {
			uf::iostream << "Error: " << e.what() << "\n";
		} catch (...) {
			// ignore parse errors
		}
}

bool uf::Serializer::readFromFile( const std::string& from ) {
	struct {
		std::ifstream input;
		uf::String buffer;
		std::string filename;
		bool exists;
	} file; {
		file.exists = uf::io::exists(from);
		file.filename = from;
	}

	if ( !file.exists ) return false;
	auto& str = file.buffer.getString();
	
	file.input.open(file.filename, std::ios::binary );
	file.input.seekg(0, std::ios::end);
	str.reserve(file.input.tellg());
	file.input.seekg(0, std::ios::beg);
	str.assign((std::istreambuf_iterator<char>(file.input)),std::istreambuf_iterator<char>());
	file.input.close();

	this->deserialize(file.buffer);
	return true;
}
bool uf::Serializer::writeToFile( const std::string& to ) const {
	std::string buffer = this->serialize();
	std::ofstream output;
	output.open(to, std::ios::binary);
	output.write( buffer.c_str(), buffer.size() );
	output.close();
	return true;
}

void uf::Serializer::merge( const uf::Serializer& other, bool priority ) {
	if ( !this->isObject() || !other.isObject() ) return;

	std::function<void(Json::Value&, const Json::Value&)> update = [&]( Json::Value& a, const Json::Value& b ) {
		if ( !b.isObject() ) return;
		for ( const auto& key : b.getMemberNames() ) {
			if ( !a.isObject() || !priority ) a[key] = b[key];
			update(a[key], b[key]);
		}
	};

	update(*this, other);
}

uf::Serializer::operator Serializer::output_t() {
	return this->serialize();
}
uf::Serializer::operator Serializer::output_t() const {
	return this->serialize();
}

uf::Serializer& uf::Serializer::operator=( const std::string& str ) {
	this->deserialize(str);
	return *this;
}
uf::Serializer& uf::Serializer::operator=( const Json::Value& json ) {
	Value::operator=(json);
	return *this;
}
uf::Serializer& uf::Serializer::operator<<( const std::string& str ) {
	this->deserialize(str);
	return *this;
}
uf::Serializer& uf::Serializer::operator>>( std::string& str ) {
	str = this->serialize();
	return *this;
}
const uf::Serializer& uf::Serializer::operator>>( std::string& str ) const {
	str = this->serialize();
	return *this;
}