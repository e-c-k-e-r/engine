#include <uf/utils/serialize/serializer.h>
#include <uf/utils/string/string.h>
#include <uf/utils/string/ext.h>

#include <uf/utils/io/iostream.h>

#include <sstream>
#include <fstream>
#include <functional>

namespace {
	std::string encode( const Json::Value& json, bool pretty = true ) {
/*
		std::stringstream ss;
		if ( pretty ) {
			ss << json;
		} else {
			Json::StreamWriterBuilder builder;
			builder["commentStyle"] = "None";
			builder["indentation"] = "";
			std::unique_ptr<Json::StreamWriter> writer( builder.newStreamWriter() );
			writer->write(json, &ss);
		}
		return ss.str();
*/
		Json::FastWriter fast;
		Json::StyledWriter styled;
		std::string output = pretty ? styled.write(json) : fast.write(json);
		if ( output.back() == '\n' ) output.pop_back();
		return output;
	}
	std::string encode( const sol::table& table ) {
		return ext::lua::state["json"]["encode"]( table );
	}

	void decode( Json::Value& json, const std::string& str ) {
		Json::Reader reader;
		if ( !reader.parse(str, json) ) {
			uf::iostream << "JSON Error: " << reader.getFormattedErrorMessages() << "\n";
		}
	}
}

uf::Serializer::Serializer( const std::string& str ) { //: sol::table(ext::lua::state, sol::create) {
	this->deserialize(str);
}
uf::Serializer::Serializer( const sol::table& table ) { //: sol::table(ext::lua::state, sol::create) {
	this->deserialize( encode( table ) );
}
uf::Serializer::Serializer( const Json::Value& json ) { //: sol::table(ext::lua::state, sol::create) {
	this->deserialize( encode( json ) );
}
uf::Serializer::output_t uf::Serializer::serialize( bool pretty ) const {
	return encode( *this, pretty );
}
void uf::Serializer::deserialize( const std::string& str ) {
	if ( str == "" ) return;
	decode( *this, str );
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
	if ( !ext::json::isObject( *this ) || !ext::json::isObject( other ) ) return;

	std::function<void(Json::Value&, const Json::Value&)> update = [&]( Json::Value& a, const Json::Value& b ) {
		if ( !ext::json::isObject( b ) ) return;
		for ( const auto& key : b.getMemberNames() ) {
		/*
			if ( !ext::json::isObject( a ) || !priority )
				a[key] = b[key];
			update(a[key], b[key]);
		*/
			if( a[key].type() == Json::objectValue && b[key].type() == Json::objectValue ) {
				update(a[key], b[key]);
			}
			if ( !priority )
				a[key] = b[key];
		}
	};

	update(*this, other);
}
void uf::Serializer::import( const uf::Serializer& other ) {
	if ( !ext::json::isObject( *this ) || !ext::json::isObject( other ) ) return;

	std::function<void(Json::Value&, const Json::Value&)> update = [&]( Json::Value& a, const Json::Value& b ) {
		// doesn't exist, just copy it
		if ( ext::json::isNull( a ) && !ext::json::isNull( b ) ) {
			a = b;
		// exists, iterate through children
		} else if ( ext::json::isObject( a ) && ext::json::isObject( b ) ) {
			for ( const auto& key : b.getMemberNames() )
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
uf::Serializer& uf::Serializer::operator=( const sol::table& table ) {
	this->deserialize( encode( table ) );
	return *this;
}
uf::Serializer& uf::Serializer::operator=( const Json::Value& json ) {
	this->deserialize( encode( json ) );
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