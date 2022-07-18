#include <uf/ext/json/json.h>
#include <uf/utils/io/iostream.h>
#include <iomanip>

uf::stl::string ext::json::PREFERRED_COMPRESSION = "";
uf::stl::string ext::json::PREFERRED_ENCODING = "json";

ext::json::Value& ext::json::Value::operator=( const uf::Serializer& json ) {
	return *this = (const ext::json::Value&) json;
}

ext::json::Value& ext::json::Value::operator=( const ext::json::Value& json ) {
	ext::json::base_value::operator=( (const ext::json::base_value&) json );
	return *this;
}

ext::json::Value& ext::json::Value::emplace_back( const ext::json::Value& json ) {
	return (ext::json::Value&) ext::json::base_value::emplace_back( (const ext::json::base_value&) json );
}
ext::json::Value& ext::json::Value::emplace_back( const uf::Serializer& json ) {
	return (ext::json::Value&) ext::json::base_value::emplace_back( (const ext::json::base_value&) json );
}

ext::json::Value& ext::json::reserve( ext::json::Value& value, size_t size ) {
	if ( !ext::json::isArray( value ) ) value = ext::json::array();
	value.reserve(size);
	return value;
}

ext::json::Value& ext::json::Value::reserve( size_t n ) {
	auto* ptr = this->get_ptr<ext::json::base_value::array_t*>();
	if ( ptr ) ptr->reserve(n);
	return *this;
}

ext::json::Value ext::json::find( const uf::stl::string& needle, const ext::json::Value& haystack ) {
	ext::json::Value exact = ext::json::null();
	ext::json::Value regexed = ext::json::null();

	ext::json::forEach( haystack, [&]( const uf::stl::string& key, const ext::json::Value& value, bool& breaks ) {
		if ( needle == key ) {
			exact = value;
			breaks = true;
		} else if ( uf::string::isRegex( key ) && uf::string::matched( needle, key ) ) {
			regexed = value;
		}
	});
	return !ext::json::isNull(exact) ? exact : regexed;
}

void ext::json::forEach( ext::json::Value& json, const std::function<void(ext::json::Value&)>& function ) {
//	UF_ASSERT_SAFE( ext::json::isArray(json) || ext::json::isObject(json) );
//	for ( auto it = json.begin(); it != json.end(); ++it ) { auto& v = *it;
	for ( auto& v : json ) {
		function( (ext::json::Value&) v );
	}
}
void ext::json::forEach( ext::json::Value& json, const std::function<void(size_t, ext::json::Value&)>& function ) {
//	UF_ASSERT_SAFE( ext::json::isArray(json) );
	for ( size_t i = 0; i < json.size(); ++i ) {
		function( i, (ext::json::Value&) json[i] );
	}
}
void ext::json::forEach( ext::json::Value& json, const std::function<void(const uf::stl::string&, ext::json::Value&)>& function ) {
//	UF_ASSERT_SAFE( ext::json::isObject(json) );
	for ( auto it = json.begin(); it != json.end(); ++it ) {
		function( it.key(), (ext::json::Value&) *it );
	}
}
void ext::json::forEach( ext::json::Value& json, const std::function<void(ext::json::Value&, bool&)>& function ) {
//	UF_ASSERT_SAFE( ext::json::isArray(json) || ext::json::isObject(json) );
//	for ( auto it = json.begin(); it != json.end(); ++it ) { auto& v = *it;
	for ( auto& v : json ) {
		bool breaks = false;
		function( (ext::json::Value&) v, breaks );
		if ( breaks ) break;
	}
}
void ext::json::forEach( ext::json::Value& json, const std::function<void(size_t, ext::json::Value&, bool&)>& function ) {
//	UF_ASSERT_SAFE( ext::json::isArray(json) );
	for ( size_t i = 0; i < json.size(); ++i ) {
		bool breaks = false;
		function( i, (ext::json::Value&) json[i], breaks );
		if ( breaks ) break;
	}
}
void ext::json::forEach( ext::json::Value& json, const std::function<void(const uf::stl::string&, ext::json::Value&, bool&)>& function ) {
//	UF_ASSERT_SAFE( ext::json::isObject(json) );
	for ( auto it = json.begin(); it != json.end(); ++it ) {
		bool breaks = false;
		function( it.key(), (ext::json::Value&) *it, breaks );
		if ( breaks ) break;
	}
}

void ext::json::forEach( const ext::json::Value& json, const std::function<void(size_t, const ext::json::Value&)>& function ) {
//	UF_ASSERT_SAFE( ext::json::isArray(json) );
	for ( size_t i = 0; i < json.size(); ++i ) {
		function( i, (const ext::json::Value&) json[i] );
	}
}
void ext::json::forEach( const ext::json::Value& json, const std::function<void(const uf::stl::string&, const ext::json::Value&)>& function ) {
//	UF_ASSERT_SAFE( ext::json::isObject(json) );
	for ( auto it = json.begin(); it != json.end(); ++it ) {
		function( it.key(), (const ext::json::Value&) *it );
	}
}
void ext::json::forEach( const ext::json::Value& json, const std::function<void(size_t, const ext::json::Value&, bool&)>& function ) {
//	UF_ASSERT_SAFE( ext::json::isArray(json) );
	for ( size_t i = 0; i < json.size(); ++i ) {
		bool breaks = false;
		function( i, (const ext::json::Value&) json[i], breaks );
		if ( breaks ) break;
	}
}
void ext::json::forEach( const ext::json::Value& json, const std::function<void(const uf::stl::string&, const ext::json::Value&, bool&)>& function ) {
//	UF_ASSERT_SAFE( ext::json::isObject(json) );
	for ( auto it = json.begin(); it != json.end(); ++it ) {
		bool breaks = false;
		function( it.key(), (const ext::json::Value&) *it, breaks );
		if ( breaks ) break;
	}
}
void ext::json::forEach( const ext::json::Value& json, const std::function<void(const ext::json::Value&)>& function ) {
//	UF_ASSERT_SAFE( ext::json::isArray(json) || ext::json::isObject(json) );
//	for ( auto it = json.begin(); it != json.end(); ++it ) { auto& v = *it;
	for ( auto& v : json ) {
		function( (const ext::json::Value&) v );
	}
}
void ext::json::forEach( const ext::json::Value& json, const std::function<void(const ext::json::Value&, bool&)>& function ) {
//	UF_ASSERT_SAFE( ext::json::isArray(json) || ext::json::isObject(json) );
//	for ( auto it = json.begin(); it != json.end(); ++it ) { auto& v = *it;
	for ( auto& v : json ) {
		bool breaks = false;
		function( (const ext::json::Value&) v, breaks );
		if ( breaks ) break;
	}
}

uf::stl::vector<uf::stl::string> ext::json::keys( const ext::json::Value& v ) {
	uf::stl::vector<uf::stl::string> keys;
	if ( !ext::json::isObject( v ) ) return keys;
	for ( auto it = v.begin(); it != v.end(); ++it ) {
		keys.emplace_back(it.key());
	}
	return keys;
}

ext::json::Value ext::json::reencode( const ext::json::Value& _x, const EncodingSettings& settings ) {
	if ( settings.precision == 0 ) return _x;
	ext::json::Value x = _x;
	if ( x.is<float>(true) ) {
		uf::stl::stringstream str;
		str << std::setprecision(settings.precision) << x.as<float>();
	//	ext::json::decode( x, str.str() );
		x = nlohmann::json::parse(str);
	} else if ( ext::json::isObject( x ) || ext::json::isArray( x ) ) {
		ext::json::forEach( x, [&]( ext::json::Value& v ){ ext::json::reencode( v, settings ); } );
	}
	return x;
}
ext::json::Value& ext::json::reencode( ext::json::Value& x, const EncodingSettings& settings ) {
	if ( settings.precision == 0 ) return x;
	if ( x.is<float>(true) ) {
		uf::stl::stringstream str;
		str << std::setprecision(settings.precision) << x.as<float>();
	//	ext::json::decode( x, str.str() );
		x = nlohmann::json::parse(str);
	} else if ( ext::json::isObject( x ) || ext::json::isArray( x ) ) {
		ext::json::forEach( x, [&]( ext::json::Value& v ){ ext::json::reencode( v, settings ); } );
	}
	return x;
}

#if UF_USE_LUA
uf::stl::string ext::json::encode( const sol::table& table ) {
	return ext::lua::state["json"]["encode"]( table );
}
#endif