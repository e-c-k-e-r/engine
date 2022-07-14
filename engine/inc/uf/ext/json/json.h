#pragma once

#include <uf/config.h>

#if defined(UF_JSON_USE_NLOHMANN) && UF_JSON_USE_NLOHMANN
	#include "nlohmann.h"
#else
	#error "JSON implementation not defined"
#endif

#if UF_USE_LUA
	#include <uf/ext/lua/lua.h>
#endif
#if UF_USE_TOML
	#include <uf/ext/toml/toml.h>
#endif

namespace ext {
	namespace json {
		extern uf::stl::string PREFERRED_COMPRESSION;
		extern uf::stl::string PREFERRED_ENCODING;

		struct UF_API EncodingSettings {
			uf::stl::string encoding = ""; // auto-assume re-encoding scheme
			uf::stl::string compression = ""; // auto-assume compression scheme
			bool pretty = false;
			bool quantize = false;
			uint8_t precision = 0;
		};
		struct UF_API DecodingSettings {
			uf::stl::string encoding = "";
		};

		// cares not about breaking
		void UF_API forEach( ext::json::Value& json, const std::function<void(ext::json::Value&)>& function );
		void UF_API forEach( ext::json::Value& json, const std::function<void(size_t, ext::json::Value&)>& function );
		void UF_API forEach( ext::json::Value& json, const std::function<void(const uf::stl::string&, ext::json::Value&)>& function );

		// handles breaks
		void UF_API forEach( ext::json::Value& json, const std::function<void(ext::json::Value&, bool&)>& function );
		void UF_API forEach( ext::json::Value& json, const std::function<void(size_t, ext::json::Value&, bool&)>& function );
		void UF_API forEach( ext::json::Value& json, const std::function<void(const uf::stl::string&, ext::json::Value&, bool&)>& function );

		// const
		void UF_API forEach( const ext::json::Value& json, const std::function<void(const ext::json::Value&)>& function );
		void UF_API forEach( const ext::json::Value& json, const std::function<void(const ext::json::Value&, bool&)>& function );
		void UF_API forEach( const ext::json::Value& json, const std::function<void(size_t, const ext::json::Value&)>& function );
		void UF_API forEach( const ext::json::Value& json, const std::function<void(const uf::stl::string&, const ext::json::Value&)>& function );
		void UF_API forEach( const ext::json::Value& json, const std::function<void(size_t, const ext::json::Value&, bool&)>& function );
		void UF_API forEach( const ext::json::Value& json, const std::function<void(const uf::stl::string&, const ext::json::Value&, bool&)>& function );

		template<typename T> uf::stl::vector<T> vector( const ext::json::Value& value );

		ext::json::Value UF_API reencode( const ext::json::Value& json, const ext::json::EncodingSettings& settings );
		ext::json::Value& UF_API reencode( ext::json::Value& json, const ext::json::EncodingSettings& settings );

		template<typename T> T& encode( const ext::json::Value& json, T& output, const ext::json::EncodingSettings& settings = {} );
		template<typename T> ext::json::Value& decode( ext::json::Value& json, const T& input, const DecodingSettings& settings = {} );
		
		inline uf::stl::string encode( const ext::json::Value& json, const ext::json::EncodingSettings& settings = {} ) {
			uf::stl::string output;
			encode( json, output, settings );
			return output;
		}
	/*
		uf::stl::string& UF_API encode( const ext::json::Value& json, uf::stl::string&, const ext::json::EncodingSettings& settings = {} );
		uf::stl::vector<uint8_t>& UF_API encode( const ext::json::Value& json, uf::stl::vector<uint8_t>&, const ext::json::EncodingSettings& settings = {} );

		ext::json::Value& UF_API decode( ext::json::Value& json, const uf::stl::string& str, const DecodingSettings& = {} );
		ext::json::Value& UF_API decode( ext::json::Value& json, const uf::stl::vector<uint8_t>& str, const DecodingSettings& = {} );
	*/

	#if UF_USE_LUA
		uf::stl::string UF_API encode( const sol::table& table );
	#endif
	}
}

template<typename T>
uf::stl::vector<T> ext::json::vector( const ext::json::Value& value ) {
	uf::stl::vector<T> res;
	res.reserve( value.size() );
	ext::json::forEach( value, [&]( const ext::json::Value& value ){
		res.emplace_back( value.as<T>() );
	});
	return res;
}

template<typename T>
T& ext::json::encode( const ext::json::Value& json, T& output, const ext::json::EncodingSettings& settings ) {
//	ext::json::Value json = ext::json::reencode( _json, settings );

#define UF_JSON_PARSE_ENCODING(ENC)\
	if ( settings.encoding == #ENC ) {\
		auto buffer = nlohmann::json::to_ ## ENC( (const ext::json::base_value&) (json) );\
		output = T( buffer.begin(), buffer.end() );\
}

	if ( settings.encoding == ""  || settings.encoding == "json" ) {
		output = settings.pretty ? json.dump(1, '\t') : json.dump();
	}
	else UF_JSON_PARSE_ENCODING(bson)
	else UF_JSON_PARSE_ENCODING(cbor)
	else UF_JSON_PARSE_ENCODING(msgpack)
	else UF_JSON_PARSE_ENCODING(ubjson)
	else UF_JSON_PARSE_ENCODING(bjdata)
#if UF_USE_TOML
	else if ( settings.encoding == "toml" ) {
		uf::stl::string buffer = ext::toml::fromJson( json.dump() );
		output = T( buffer.begin(), buffer.end() );
	}
#endif
	else {
		// should probably default to json, not my problem
		UF_MSG_ERROR("invalid encoding requested: {}", settings.encoding);
	}
	return output;
#undef UF_JSON_PARSE_ENCODING
}
template<typename T>
ext::json::Value& ext::json::decode( ext::json::Value& json, const T& input, const DecodingSettings& settings ) {
#define UF_JSON_PARSE_ENCODING(ENC)\
	if ( settings.encoding == #ENC ) {\
		json = nlohmann::json::from_ ## ENC(input, strict, exceptions);\
	}

	constexpr bool comments = true;
	constexpr bool strict = true;
	constexpr bool exceptions = true;
#if UF_EXCEPTIONS
	try {
#endif
		if ( settings.encoding == "" || settings.encoding == "json" ) {
			json = nlohmann::json::parse(input, nullptr, exceptions, comments);
		}
		else UF_JSON_PARSE_ENCODING(bson)
		else UF_JSON_PARSE_ENCODING(cbor)
		else UF_JSON_PARSE_ENCODING(msgpack)
		else UF_JSON_PARSE_ENCODING(ubjson)
		else UF_JSON_PARSE_ENCODING(bjdata)
#if UF_USE_TOML
		else if ( settings.encoding == "toml" ) {
		//	UF_MSG_DEBUG("TOML: {}", std::string_view{ (const char*) input.data(), input.size() });
			T parsed = ext::toml::toJson( input );
		//	UF_MSG_DEBUG("JSON: {}", std::string_view{ (const char*) parsed.data(), parsed.size() });
			json = nlohmann::json::parse(parsed, nullptr, exceptions, comments);
		//	UF_MSG_DEBUG("ENCODED: {}", json.dump());
		}
#endif
		else {
			// should probably default to json, not my problem
			UF_MSG_ERROR("invalid encoding requested: {}", settings.encoding);
		}
#if UF_EXCEPTIONS
	} catch ( nlohmann::json::parse_error& e ) {
		UF_MSG_ERROR("JSON error: {}", e.what());
	}
#endif
	return json;

#undef UF_JSON_PARSE_ENCODING
}