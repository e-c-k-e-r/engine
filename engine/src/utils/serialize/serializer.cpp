#include <uf/utils/serialize/serializer.h>
#include <uf/utils/string/string.h>
#include <uf/utils/string/ext.h>

#include <uf/utils/io/iostream.h>
#include <uf/ext/zlib/zlib.h>

#include <sstream>
#include <fstream>
#include <functional>

uf::Serializer::Serializer( const uf::stl::string& str ) { //: sol::table(ext::lua::state, sol::create) {
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
	return ext::json::encode( *this, ext::json::EncodingSettings{ .pretty = pretty } );
}
uf::Serializer::output_t uf::Serializer::serialize( const ext::json::EncodingSettings& settings ) const {
	return ext::json::encode( *this, settings );
}
void uf::Serializer::deserialize( const uf::stl::string& str ) {
	if ( str == "" ) return;
	ext::json::decode( *this, str );
}

bool uf::Serializer::readFromFile( const uf::stl::string& filename, const uf::stl::string& hash ) {
	uf::String string;
//	uf::stl::string filename = _filename + ( uf::io::exists(_filename + ".gz") ? ".gz" : "" );
	bool exists = uf::io::exists( filename );
	if ( !exists ) {
		UF_MSG_ERROR("Failed to read JSON file `" << filename << "`: does not exist");
		return false;
	}
	auto buffer = uf::io::readAsBuffer( filename, hash );
	if ( buffer.empty() ) {
		UF_MSG_ERROR("Failed to read JSON file `" << filename << "`: empty file or hash mismatch");
		return false;
	}	

	auto& str = string.getString();
	str.reserve(buffer.size());
	str.assign(buffer.begin(), buffer.end());

	this->deserialize(string);
	return true;
}
bool uf::Serializer::writeToFile( const uf::stl::string& filename, const ext::json::EncodingSettings& settings ) const {
	uf::stl::string buffer = this->serialize( settings );
	size_t written = uf::io::write( filename + ( settings.compress ? ".gz" : "" ) , buffer.c_str(), buffer.size() );
	if ( !written ) UF_MSG_ERROR("Failed to write JSON file `" << filename << "`");
	return written;
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
		if ( diff[i]["op"].as<uf::stl::string>() != "remove" )
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
ext::json::Value& uf::Serializer::path( const uf::stl::string& path ) {
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

uf::Serializer& uf::Serializer::operator=( const uf::stl::string& str ) {
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
uf::Serializer& uf::Serializer::operator<<( const uf::stl::string& str ) {
	this->deserialize(str);
	return *this;
}
uf::Serializer& uf::Serializer::operator>>( uf::stl::string& str ) {
	str = this->serialize();
	return *this;
}
const uf::Serializer& uf::Serializer::operator>>( uf::stl::string& str ) const {
	str = this->serialize();
	return *this;
}