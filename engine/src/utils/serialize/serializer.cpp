#include <uf/utils/serialize/serializer.h>
#include <uf/utils/string/string.h>
#include <uf/utils/string/ext.h>

#include <uf/utils/io/iostream.h>
#include <uf/ext/zlib/zlib.h>
#include <uf/ext/lua/lua.h>

#include <sstream>
#include <fstream>
#include <functional>

#define UF_SERIALIZER_IMPLICIT_LOAD 1
#if UF_ENV_DREAMCAST
#define UF_SERIALIZER_AUTO_CONVERT 0
#else
#define UF_SERIALIZER_AUTO_CONVERT 1
#endif

// uf::Serializer::Serializer() {}
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
	return resolveFilename( filename, ext::json::EncodingSettings{
		.encoding = ext::json::PREFERRED_ENCODING,
		.compression = ext::json::PREFERRED_COMPRESSION,
	}, compareTimes );
}
uf::stl::string uf::Serializer::resolveFilename( const uf::stl::string& filename, const ext::json::EncodingSettings& settings, bool compareTimes ) {
	uf::stl::string _filename = filename;

	if ( settings.encoding != "" && settings.encoding != "json" ) {
		if ( uf::io::extension( _filename ) == "json" ) {
			_filename = uf::io::replace_extension( _filename, settings.encoding );
		}
	}
	if ( settings.compression != "" ) {
		_filename = _filename + "." + settings.compression;
	}
	if ( !compareTimes ) return _filename;

	uf::stl::string cache_filename = _filename;
#if !UF_CONVERSION_TO_CACHE
	if ( !uf::vfs::isPhysical( filename ) )
#endif
		cache_filename = "cache://" + _filename;

	if ( uf::io::exists( cache_filename ) ) {
		bool should = !uf::io::exists( filename );
		if ( !should ) should = uf::io::mtime( cache_filename ) >= uf::io::mtime( filename );
		if ( should ) return cache_filename;
	}

	if ( uf::io::exists( _filename ) ) {
		bool should = !uf::io::exists( filename );
		if ( !should ) should = uf::io::mtime( _filename ) >= uf::io::mtime( filename );
		if ( should ) return _filename;
	}
	return filename;
}

bool uf::Serializer::readFromFile( const uf::stl::string& filename, const uf::stl::string& hash ) {
#if UF_SERIALIZER_IMPLICIT_LOAD
	if ( uf::io::extension( filename ) == "json" ) {
	#if 0 && UF_USE_TOML
		uf::stl::string toml_filename = uf::string::replace( filename, "/\\.json$/", ".toml" ); // load from toml if newer
		if ( uf::io::mtime( toml_filename ) > uf::io::mtime( filename ) ) {
			UF_MSG_DEBUG("Deserialize redirect: {} -> {}", filename, toml_filename);
			return readFromFile( toml_filename, hash );
		}
	#endif
		uf::stl::string _filename = resolveFilename( filename );
		if ( _filename != filename ) return readFromFile( _filename, hash );
	}
#endif
	bool exists = uf::io::exists( filename );
	if ( !exists ) {
		UF_MSG_ERROR("Failed to read JSON file {}: does not exist", filename);
		return false;
	}
	auto buffer = uf::io::readAsBuffer( filename, hash );
	if ( buffer.empty() ) {
		UF_MSG_ERROR("Failed to read JSON file {}: empty file or hash mismatch", filename);
		return false;
	}

	ext::json::DecodingSettings settings;
	uf::stl::string ext = uf::io::extension( filename );

	if ( ext == "gz" || ext == "lz4" ) {
		ext = uf::io::extension( uf::io::remove_extension( filename ) );
	}

	if ( ext == "bson" ) settings.encoding = "bson";
	else if ( ext == "cbor" ) settings.encoding = "cbor";
	else if ( ext == "msgpack" ) settings.encoding = "msgpack";
	else if ( ext == "ubjson" ) settings.encoding = "ubjson";
	else if ( ext == "bjdata" ) settings.encoding = "bjdata";
	else if ( ext == "toml" ) settings.encoding = "toml";
	else if ( ext == "json" ) settings.encoding = "json";
	else UF_MSG_DEBUG( "invalid encoding filetype requested: {}", filename );

	this->deserialize( buffer, settings );

#if UF_SERIALIZER_AUTO_CONVERT
#if 0 && UF_USE_TOML
	if ( uf::string::matched( filename, "/\\.json$/" ) ) {
		if ( ext::json::PREFERRED_ENCODING != "toml" ) {
			uf::stl::string _filename = uf::string::replace( filename, "/\\.json$/", ".toml" );

			ext::json::EncodingSettings _settings{
				.encoding = "toml",
				.compression = "",
			};
			bool should = !uf::io::exists( _filename ); // auto convert if preferred file doesn't already exist
			if ( !should ) should = uf::io::mtime( _filename ) < uf::io::mtime( filename ); // auto convert if preferred file is older than source file
			if ( should ) {
				writeToFile( _filename, _settings );
			}
		}
	}
#endif
	if ( ext == "json" || ext == "toml" ) {
		// auto convert read file to preferred filetype
		if ( ext::json::PREFERRED_COMPRESSION != "" || (ext::json::PREFERRED_ENCODING != "" && ext::json::PREFERRED_ENCODING != "json") ) {
			ext::json::EncodingSettings _settings;
			_settings.encoding = ext::json::PREFERRED_ENCODING;
			_settings.compression = ext::json::PREFERRED_COMPRESSION;
			uf::stl::string _filename = filename;

			if ( ext::json::PREFERRED_ENCODING != "" && ext::json::PREFERRED_ENCODING != "json" ) {
				_settings.encoding = ext::json::PREFERRED_ENCODING;
				if ( uf::io::extension(_filename) == "json" ) {
					_filename = uf::io::replace_extension( _filename, ext::json::PREFERRED_ENCODING );
				}
			}
			if ( ext::json::PREFERRED_COMPRESSION != "" ) {
				_settings.compression = ext::json::PREFERRED_COMPRESSION;
				_filename = _filename + "." + ext::json::PREFERRED_COMPRESSION;
			}

		#if !UF_CONVERSION_TO_CACHE
			if ( !uf::vfs::isPhysical( filename ) ) _filename = "cache://" + _filename;
		#else
			_filename = "cache://" + _filename;
		#endif

			bool should = !uf::io::exists( _filename );
			if ( !should ) should = uf::io::mtime( _filename ) < uf::io::mtime( filename );
			if ( should ) {
				writeToFile( _filename, _settings );
			}
		}
	}
#endif

	return true;
}
bool uf::Serializer::writeToFile( const uf::stl::string& filename, const ext::json::EncodingSettings& settings ) const {
	uf::stl::string output = filename;

	if ( settings.encoding != "" && settings.encoding != "json" ) {
		output = uf::string::replace( output, ".json", "." + settings.encoding, false );
	}

	if ( settings.compression != "" && uf::io::extension(output) != settings.compression ) {
		output += "." + settings.compression;
	}

	uf::stl::string buffer = this->serialize( settings );
	size_t written = uf::io::write( output, buffer.data(), buffer.size() );

#if UF_SERIALIZER_AUTO_CONVERT
	if ( uf::io::extension(output) == "json" && settings.compression != ext::json::PREFERRED_COMPRESSION ) {
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

	if ( !written ) UF_MSG_ERROR("Failed to write JSON file: {}", output);
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

// only updates if this[key] == null
void uf::Serializer::import( const uf::Serializer& other ) {
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