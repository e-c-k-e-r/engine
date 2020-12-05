#include <uf/ext/json/json.h>
#include <uf/utils/io/iostream.h>

#define SAFE_ASSERT(...) if ( !(__VA_ARGS__) ) std::cout << "ASSERTION FAILED: " << __FILE__ << "@" << __FUNCTION__ << ":" << __LINE__ << ": " << #__VA_ARGS__ << std::endl;

ext::json::Value& ext::json::Value::operator=( const ext::json::Value& json ) {
	ext::json::base_value::operator=( (const ext::json::base_value&) json );
	return *this;
}

void ext::json::forEach( ext::json::Value& json, const std::function<void(ext::json::Value&)>& function ) {
//	SAFE_ASSERT( ext::json::isArray(json) || ext::json::isObject(json) );
//	for ( auto it = json.begin(); it != json.end(); ++it ) { auto& v = *it;
	for ( auto& v : json ) {
		function( (ext::json::Value&) v );
	}
}
void ext::json::forEach( ext::json::Value& json, const std::function<void(size_t, ext::json::Value&)>& function ) {
//	SAFE_ASSERT( ext::json::isArray(json) );
	for ( size_t i = 0; i < json.size(); ++i ) {
		function( i, (ext::json::Value&) json[i] );
	}
}
void ext::json::forEach( ext::json::Value& json, const std::function<void(const std::string&, ext::json::Value&)>& function ) {
//	SAFE_ASSERT( ext::json::isObject(json) );
	for ( auto it = json.begin(); it != json.end(); ++it ) {
		function( it.key(), (ext::json::Value&) *it );
	}
}
void ext::json::forEach( ext::json::Value& json, const std::function<void(ext::json::Value&, bool&)>& function ) {
//	SAFE_ASSERT( ext::json::isArray(json) || ext::json::isObject(json) );
//	for ( auto it = json.begin(); it != json.end(); ++it ) { auto& v = *it;
	for ( auto& v : json ) {
		bool breaks = false;
		function( (ext::json::Value&) v, breaks );
		if ( breaks ) break;
	}
}
void ext::json::forEach( ext::json::Value& json, const std::function<void(size_t, ext::json::Value&, bool&)>& function ) {
//	SAFE_ASSERT( ext::json::isArray(json) );
	for ( size_t i = 0; i < json.size(); ++i ) {
		bool breaks = false;
		function( i, (ext::json::Value&) json[i], breaks );
		if ( breaks ) break;
	}
}
void ext::json::forEach( ext::json::Value& json, const std::function<void(const std::string&, ext::json::Value&, bool&)>& function ) {
//	SAFE_ASSERT( ext::json::isObject(json) );
	for ( auto it = json.begin(); it != json.end(); ++it ) {
		bool breaks = false;
		function( it.key(), (ext::json::Value&) *it, breaks );
		if ( breaks ) break;
	}
}

void ext::json::forEach( const ext::json::Value& json, const std::function<void(size_t, const ext::json::Value&)>& function ) {
//	SAFE_ASSERT( ext::json::isArray(json) );
	for ( size_t i = 0; i < json.size(); ++i ) {
		function( i, (const ext::json::Value&) json[i] );
	}
}
void ext::json::forEach( const ext::json::Value& json, const std::function<void(const std::string&, const ext::json::Value&)>& function ) {
//	SAFE_ASSERT( ext::json::isObject(json) );
	for ( auto it = json.begin(); it != json.end(); ++it ) {
		function( it.key(), (const ext::json::Value&) *it );
	}
}
void ext::json::forEach( const ext::json::Value& json, const std::function<void(size_t, const ext::json::Value&, bool&)>& function ) {
//	SAFE_ASSERT( ext::json::isArray(json) );
	for ( size_t i = 0; i < json.size(); ++i ) {
		bool breaks = false;
		function( i, (const ext::json::Value&) json[i], breaks );
		if ( breaks ) break;
	}
}
void ext::json::forEach( const ext::json::Value& json, const std::function<void(const std::string&, const ext::json::Value&, bool&)>& function ) {
//	SAFE_ASSERT( ext::json::isObject(json) );
	for ( auto it = json.begin(); it != json.end(); ++it ) {
		bool breaks = false;
		function( it.key(), (const ext::json::Value&) *it, breaks );
		if ( breaks ) break;
	}
}
void ext::json::forEach( const ext::json::Value& json, const std::function<void(const ext::json::Value&)>& function ) {
//	SAFE_ASSERT( ext::json::isArray(json) || ext::json::isObject(json) );
//	for ( auto it = json.begin(); it != json.end(); ++it ) { auto& v = *it;
	for ( auto& v : json ) {
		function( (const ext::json::Value&) v );
	}
}
void ext::json::forEach( const ext::json::Value& json, const std::function<void(const ext::json::Value&, bool&)>& function ) {
//	SAFE_ASSERT( ext::json::isArray(json) || ext::json::isObject(json) );
//	for ( auto it = json.begin(); it != json.end(); ++it ) { auto& v = *it;
	for ( auto& v : json ) {
		bool breaks = false;
		function( (const ext::json::Value&) v, breaks );
		if ( breaks ) break;
	}
}

#if defined(UF_JSON_USE_NLOHMANN) && UF_JSON_USE_NLOHMANN
std::vector<std::string> ext::json::keys( const ext::json::Value& v ) {
	std::vector<std::string> keys;
	if ( !ext::json::isObject( v ) ) return keys;
	for ( auto it = v.begin(); it != v.end(); ++it ) {
		keys.emplace_back(it.key());
	}
	return keys;
}
#elif defined(UF_JSON_USE_RAPIDJSON) && UF_JSON_USE_RAPIDJSON
	rapidjson::Document::AllocatorType ext::json::allocator;
#endif

std::string ext::json::encode( const ext::json::Value& json, bool pretty ) {
#if defined(UF_JSON_USE_NLOHMANN) && UF_JSON_USE_NLOHMANN
	return pretty ? json.dump(1, '\t') : json.dump();
#elif defined(UF_JSON_USE_JSONCPP) && UF_JSON_USE_JSONCPP
	Json::FastWriter fast;
	Json::StyledWriter styled;
	std::string output = pretty ? styled.write(json) : fast.write(json);
	if ( output.back() == '\n' ) output.pop_back();
	return output;
#elif defined(UF_JSON_USE_LUA) && UF_JSON_USE_LUA
	return ext::lua::state["json"]["encode"]( table );
#endif
}
std::string ext::json::encode( const sol::table& table ) {
	return ext::lua::state["json"]["encode"]( table );
}

void ext::json::decode( ext::json::Value& json, const std::string& str ) {
#if defined(UF_JSON_USE_NLOHMANN) && UF_JSON_USE_NLOHMANN
	try {
		json = nlohmann::json::parse(str, nullptr, true, true);
	} catch ( nlohmann::json::parse_error& e ) {
		uf::iostream << "[JSON] " << e.what() << "\n";
	}
#elif defined(UF_JSON_USE_JSONCPP) && UF_JSON_USE_JSONCPP 
	Json::Reader reader;
	if ( !reader.parse(str, json) ) {
		uf::iostream << "JSON Error: " << reader.getFormattedErrorMessages() << "\n";
	}
#elif defined(UF_JSON_USE_LUA) && UF_JSON_USE_LUA
	json = ext::lua::state["json"]["decode"]( str );
#endif
}