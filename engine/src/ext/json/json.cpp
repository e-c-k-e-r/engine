#include <uf/ext/json/json.h>
#include <uf/utils/io/iostream.h>
#include <iomanip>

ext::json::Value& ext::json::Value::operator=( const ext::json::Value& json ) {
	ext::json::base_value::operator=( (const ext::json::base_value&) json );
	return *this;
}

ext::json::Value& ext::json::reserve( ext::json::Value& value, size_t size ) {
	if ( !ext::json::isArray( value ) ) value = ext::json::array();
	value.reserve(size);
	return value;
}

#if UF_JSON_USE_NLOHMANN
ext::json::Value& ext::json::Value::reserve( size_t n ) {
	auto* ptr = this->get_ptr<ext::json::base_value::array_t*>();
	if ( ptr ) ptr->reserve(n);
	return *this;
}
#endif

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

#if defined(UF_JSON_USE_NLOHMANN) && UF_JSON_USE_NLOHMANN
uf::stl::vector<uf::stl::string> ext::json::keys( const ext::json::Value& v ) {
	uf::stl::vector<uf::stl::string> keys;
	if ( !ext::json::isObject( v ) ) return keys;
	for ( auto it = v.begin(); it != v.end(); ++it ) {
		keys.emplace_back(it.key());
	}
	return keys;
}
#elif defined(UF_JSON_USE_RAPIDJSON) && UF_JSON_USE_RAPIDJSON
	rapidjson::Document::AllocatorType ext::json::allocator;
#endif

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

uf::stl::string ext::json::encode( const ext::json::Value& json, bool pretty ) {
	return ext::json::encode( json, ext::json::EncodingSettings{ .pretty = true } );
}
uf::stl::string ext::json::encode( const ext::json::Value& _json, const ext::json::EncodingSettings& settings ) {
	ext::json::Value json = ext::json::reencode( _json, settings );
#if defined(UF_JSON_USE_NLOHMANN) && UF_JSON_USE_NLOHMANN
	return settings.pretty ? json.dump(1, '\t') : json.dump();
#elif defined(UF_JSON_USE_JSONCPP) && UF_JSON_USE_JSONCPP
	Json::FastWriter fast;
	Json::StyledWriter styled;
	uf::stl::string output = settings.pretty ? styled.write(json) : fast.write(json);
	if ( output.back() == '\n' ) output.pop_back();
	return output;
#elif defined(UF_JSON_USE_LUA) && UF_JSON_USE_LUA
	return ext::lua::state["json"]["encode"]( _json );
#endif
}

#if UF_USE_LUA
uf::stl::string ext::json::encode( const sol::table& table ) {
	return ext::lua::state["json"]["encode"]( table );
}
#endif
ext::json::Value& ext::json::decode( ext::json::Value& json, const uf::stl::string& str ) {
#if defined(UF_JSON_USE_NLOHMANN) && UF_JSON_USE_NLOHMANN
#if UF_NO_EXCEPTIONS
	json = nlohmann::json::parse(str, nullptr, true, true);
#else
	try {
		json = nlohmann::json::parse(str, nullptr, true, true);
	} catch ( nlohmann::json::parse_error& e ) {
		UF_MSG_ERROR("JSON error: " << e.what() << "\tAttempted to parse: " << str)
	}
#endif
#elif defined(UF_JSON_USE_JSONCPP) && UF_JSON_USE_JSONCPP 
	Json::Reader reader;
	if ( !reader.parse(str, json) ) {
		UF_MSG_ERROR("JSON error: " << reader.getFormattedErrorMessages() << "\tAttempted to parse: " << str)
	}
#elif defined(UF_JSON_USE_LUA) && UF_JSON_USE_LUA
	json = ext::lua::state["json"]["decode"]( str );
#endif
	return json;
}