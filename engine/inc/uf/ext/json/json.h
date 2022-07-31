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

		//
		ext::json::Value find( const uf::stl::string& needle, const ext::json::Value& haystack );

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

#include "json.inl"