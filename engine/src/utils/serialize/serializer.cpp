#include <uf/utils/serialize/serializer.h>
#include <uf/utils/string/string.h>
#include <uf/utils/string/ext.h>

#include <uf/utils/io/iostream.h>

#include <sstream>
#include <fstream>
#include <functional>

uf::Serializer::Serializer( const std::string& str ) { //: sol::table(ext::lua::state, sol::create) {
	this->deserialize(str);
}
#if UF_USE_LUA
uf::Serializer::Serializer( const sol::table& table ) { //: sol::table(ext::lua::state, sol::create) {
	this->deserialize( ext::json::encode( table ) );
}
#endif
uf::Serializer::Serializer( const ext::json::Value& json ) { //: sol::table(ext::lua::state, sol::create) {
//	this->deserialize( ext::json::encode( json ) );
	*this = json;
}
uf::Serializer::Serializer( const ext::json::base_value& json ) { //: sol::table(ext::lua::state, sol::create) {
//	this->deserialize( ext::json::encode( json ) );
	*this = json;
}
uf::Serializer::output_t uf::Serializer::serialize( bool pretty ) const {
	return ext::json::encode( *this, pretty );
}
void uf::Serializer::deserialize( const std::string& str ) {
	if ( str == "" ) return;
	ext::json::decode( *this, str );
}

bool uf::Serializer::readFromFile( const std::string& from, const std::string& hash ) {
	uf::String string;
	bool exists = uf::io::exists(from);
	if ( !exists ) {
		uf::iostream << "Failed to read JSON file `" << from << "`: does not exist" << "\n";
		return false;
	}
	auto buffer = uf::io::readAsBuffer( from, hash );
	if ( buffer.empty() ) {
		uf::iostream << "Failed to read JSON file `" << from << "`: empty file or hash mismatch" << "\n";
		return false;
	}

	auto& str = string.getString();
	str.reserve(buffer.size());
	str.assign(buffer.begin(), buffer.end());


	this->deserialize(string);
	return true;
/*	
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

	file.input.open(file.filename, std::ios::binary );
	file.input.seekg(0, std::ios::end);
	str.reserve(file.input.tellg());
	file.input.seekg(0, std::ios::beg);
	str.assign((std::istreambuf_iterator<char>(file.input)),std::istreambuf_iterator<char>());
	file.input.close();

	this->deserialize(file.buffer);
	return true;
*/
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
	if ( !ext::json::isObject( *this ) || !ext::json::isObject( other ) ) return;

	std::function<void(ext::json::Value&, const ext::json::Value&)> update = [&]( ext::json::Value& a, const ext::json::Value& b ) {
		if ( !ext::json::isObject( b ) ) return;
		auto keys = ext::json::keys( b );
			for ( auto key : keys ) {
			if( ext::json::isObject(a[key]) && ext::json::isObject(b[key]) ) {
				update(a[key], b[key]);
			}
			if ( !priority )
				a[key] = b[key];
		}
	};

	update(*this, other);
}
void uf::Serializer::import( const uf::Serializer& other ) {
/*
	uf::Serializer diff = nlohmann::json::diff(*this, other);
	uf::Serializer filtered;
	for ( size_t i = 0; i < diff.size(); ++i ) {
		if ( diff[i]["op"].as<std::string>() != "remove" )
			filtered.emplace_back(diff[i]);
	}
	patch( filtered );
	std::cout << this->dump(1, '\t') << std::endl;
*/

	if ( !ext::json::isObject( *this ) || !ext::json::isObject( other ) ) return;
	std::function<void(ext::json::Value&, const ext::json::Value&)> update = [&]( ext::json::Value& a, const ext::json::Value& b ) {
		// doesn't exist, just copy it
		if ( ext::json::isNull( a ) && !ext::json::isNull( b ) ) {
			a = b;
		// exists, iterate through children
		} else if ( ext::json::isObject( a ) && ext::json::isObject( b ) ) {
			auto keys = ext::json::keys( b );
			for ( auto key : keys )
				update(a[key], b[key]);
		}
	};
	update(*this, other);
}
ext::json::Value& uf::Serializer::path( const std::string& path ) {
	auto keys = uf::string::split(path, ".");
	ext::json::Value* traversal = this;
	for ( auto& key : keys ) {
		traversal = &((*traversal)[key]);
	}
	return *traversal;
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
#if UF_USE_LUA
uf::Serializer& uf::Serializer::operator=( const sol::table& table ) {
	this->deserialize( ext::json::encode( table ) );
	return *this;
}
#endif
uf::Serializer& uf::Serializer::operator=( const ext::json::Value& json ) {
	// this->deserialize( ext::json::encode( json ) );
	ext::json::Value::operator=(json);
	return *this;
}
uf::Serializer& uf::Serializer::operator=( const ext::json::base_value& json ) {
	// this->deserialize( ext::json::encode( json ) );
	ext::json::Value::operator=(json);
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