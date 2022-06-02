#pragma once

#include <uf/config.h>

#if defined(UF_JSON_USE_NLOHMANN) && UF_JSON_USE_NLOHMANN
	#include "nlohmann.h"
#elif defined(UF_JSON_USE_JSONCPP) && UF_JSON_USE_JSONCPP
	#include "jsoncpp.h"
#elif defined(UF_JSON_USE_RAPIDJSON) && UF_JSON_USE_RAPIDJSON
	#include "rapidjson.h"
#elif defined(UF_JSON_USE_LUA) && UF_JSON_USE_LUA
	#include "lua.h"
#else
	#error "JSON implementation not defined"
#endif

#if UF_USE_LUA
	#include <uf/ext/lua/lua.h>
#endif

namespace ext {
	namespace json {
		extern uf::stl::string PREFERRED_COMPRESSION;
		extern uf::stl::string PREFERRED_ENCODING;

		struct UF_API EncodingSettings {
			uf::stl::string compression = ""; // auto-assume compression scheme
			uf::stl::string encoding = ""; // auto-assume re-encoding scheme
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

		ext::json::Value UF_API reencode( const ext::json::Value& json, const ext::json::EncodingSettings& settings );
		ext::json::Value& UF_API reencode( ext::json::Value& json, const ext::json::EncodingSettings& settings );

		uf::stl::string UF_API encode( const ext::json::Value& json, bool pretty = true );
		uf::stl::string UF_API encode( const ext::json::Value& json, const ext::json::EncodingSettings& settings );

		ext::json::Value& UF_API decode( ext::json::Value& json, const uf::stl::string& str, const DecodingSettings& = {} );
	#if UF_USE_LUA
		uf::stl::string UF_API encode( const sol::table& table );
	#endif
	}
}