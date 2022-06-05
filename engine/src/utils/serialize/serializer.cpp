#include <uf/utils/serialize/serializer.h>
#include <uf/utils/string/string.h>
#include <uf/utils/string/ext.h>

#include <uf/utils/io/iostream.h>
#include <uf/ext/zlib/zlib.h>

#include <sstream>
#include <fstream>
#include <functional>

#define UF_SERIALIZER_IMPLICIT_LOAD 1
#if UF_ENV_DREAMCAST
#define UF_SERIALIZER_AUTO_CONVERT 0
#else
#define UF_SERIALIZER_AUTO_CONVERT 1
#endif

//#define UF_MSG_DEBUG_(...) UF_MSG_DEBUG(__VA_ARGS__)
#define UF_MSG_DEBUG_(...)

uf::Serializer::Serializer() {
}
uf::Serializer::Serializer( const uf::stl::string& str ) {
	this->deserialize(str);
}
#if UF_USE_LUA
uf::Serializer::Serializer( const sol::table& table ) {
	this->deserialize( ext::json::encode( table ) );
}
#endif
uf::Serializer::Serializer( const ext::json::Value& json ) {
	*this = json;
}
uf::Serializer::Serializer( const ext::json::base_value& json ) {
	*this = json;
}
uf::Serializer::output_t uf::Serializer::serialize( const ext::json::EncodingSettings& settings ) const {
	return ext::json::encode( *this, settings );
}

uf::stl::string uf::Serializer::resolveFilename( const uf::stl::string& filename, bool compareTimes ) {
	uf::stl::string _filename = filename;
	if ( ext::json::PREFERRED_ENCODING != "" && ext::json::PREFERRED_ENCODING != "json" ) {
		_filename = uf::string::replace( _filename, "/\\.json$/", "." + ext::json::PREFERRED_ENCODING );
	}
	if ( ext::json::PREFERRED_COMPRESSION != "" ) {
		_filename = _filename + "." + ext::json::PREFERRED_COMPRESSION;
	}
	if ( !compareTimes ) return _filename;

	if ( uf::io::exists( _filename ) ) {
		bool should = !uf::io::exists( filename ); // implicit load if our requested filename doesnt exist anyways, but our preferred source does
		if ( !should ) should = uf::io::mtime( _filename ) >= uf::io::mtime( filename ); // implicit load the preferred source is newer than the requested filename
		if ( should ) return _filename;
	}
	return filename;
}

bool uf::Serializer::readFromFile( const uf::stl::string& filename, const uf::stl::string& hash ) {
//	UF_MSG_DEBUG("Loading: " << filename);
#if UF_SERIALIZER_IMPLICIT_LOAD
	// implicitly check for optimal format for plain .json requests
	if ( uf::string::matched( filename, "/\\.json$/" ) ) {
		uf::stl::string _filename = uf::Serializer::resolveFilename( filename );
		if ( _filename != filename ) return readFromFile( _filename, hash );
	}
#endif
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

	ext::json::DecodingSettings settings;
	if ( uf::string::matched( filename, "/\\.bson/" ) ) settings.encoding = "bson";
	else if ( uf::string::matched( filename, "/\\.cbor/" ) ) settings.encoding = "cbor";
	else if ( uf::string::matched( filename, "/\\.msgpack/" ) ) settings.encoding = "msgpack";
	else if ( uf::string::matched( filename, "/\\.ubjson/" ) ) settings.encoding = "ubjson";
	else if ( uf::string::matched( filename, "/\\.bjdata/" ) ) settings.encoding = "bjdata";
//	else UF_MSG_DEBUG( string );

	this->deserialize( buffer, settings );

#if UF_SERIALIZER_AUTO_CONVERT
	if ( uf::string::matched( filename, "/\\.json$/" ) ) {
		if ( ext::json::PREFERRED_COMPRESSION != "" || (ext::json::PREFERRED_ENCODING != "" || ext::json::PREFERRED_ENCODING != "json") ) {
			ext::json::EncodingSettings _settings;
			_settings.encoding = ext::json::PREFERRED_ENCODING;
			_settings.compression = ext::json::PREFERRED_COMPRESSION;
			uf::stl::string _filename = filename;

			if ( ext::json::PREFERRED_ENCODING != "" && ext::json::PREFERRED_ENCODING != "json" ) {
				_settings.encoding = ext::json::PREFERRED_ENCODING;
				_filename = uf::string::replace( _filename, "/\\.json$/", "." + ext::json::PREFERRED_ENCODING );
			}
			if ( ext::json::PREFERRED_COMPRESSION != "" ) {
				_settings.compression = ext::json::PREFERRED_COMPRESSION;
				_filename = _filename + "." + ext::json::PREFERRED_COMPRESSION;
			}
			bool should = !uf::io::exists( _filename ); // auto convert if preferred file doesn't already exist
			if ( !should ) should = uf::io::mtime( _filename ) < uf::io::mtime( filename ); // auto convert if preferred file is older than source file
			if ( should ) {
				UF_MSG_DEBUG_("Auto converting: " << _filename );
				writeToFile( _filename, _settings );
			}
		}
	}
#endif

	return true;
}
bool uf::Serializer::writeToFile( const uf::stl::string& filename, const ext::json::EncodingSettings& settings ) const {
	uf::stl::string output = filename;
	
	if ( settings.encoding != "" && settings.encoding != "json" )
		output = uf::string::replace( output, "/\\.json/", "." + settings.encoding );
	if ( settings.compression != "" && !uf::string::matched( output, "/."+settings.compression+"/" ) )
		output += "." + settings.compression;

	uf::stl::string buffer = this->serialize( settings );
	size_t written = uf::io::write( output, buffer.c_str(), buffer.size() );
#if UF_SERIALIZER_AUTO_CONVERT
	// implicitly check for optimal format for plain .json requests
	if ( uf::string::matched( output, "/\\.json$/" ) && settings.compression != ext::json::PREFERRED_COMPRESSION ) {
		uf::stl::string _filename = output;
		auto _settings = settings;

		if ( ext::json::PREFERRED_ENCODING != "" && ext::json::PREFERRED_ENCODING != "json" ) {
			_settings.encoding = ext::json::PREFERRED_ENCODING;
		}
		if ( ext::json::PREFERRED_COMPRESSION != "" ) {
			_settings.compression = ext::json::PREFERRED_COMPRESSION;
		}
		writeToFile( _filename, _settings );
	}
#endif

	if ( !written ) UF_MSG_ERROR("Failed to write JSON file: " << output);
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
	ext::json::Value::operator=(json);
	return *this;
}
uf::Serializer& uf::Serializer::operator=( const ext::json::base_value& json ) {
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