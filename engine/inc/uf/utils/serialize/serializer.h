#pragma once

#include <uf/config.h>

#include <uf/utils/userdata/userdata.h>
#include <uf/ext/lua/lua.h>
#include <uf/ext/json/json.h>

#include <uf/utils/memory/string.h>
#include <type_traits>

namespace uf {
	class UF_API Serializer : public ext::json::Value {
	public:
		typedef uf::stl::string output_t;
		typedef uf::stl::string  input_t;
	
		Serializer();
		Serializer( const uf::stl::string& str );
		Serializer( const ext::json::base_value& );
		Serializer( const ext::json::Value& );
	#if UF_USE_LUA
		Serializer( const sol::table& );
	#endif
		
		Serializer::output_t serialize( const ext::json::EncodingSettings& = {} ) const;
		
		template<typename T>
		void deserialize( const T& input, const ext::json::DecodingSettings& settings = {} ) {
			ext::json::decode( *this, input, settings );
		}

		// serializeable
		template<typename T>
		static bool serializeable() {
			return std::is_standard_layout<T>::value && std::is_trivial<T>::value; // std::is_pod<T>::value;
		}
		template<typename T>
		static bool serializeable( const T& input ) {
			return serializeable<T>();
		}
	
		// serialize to base64
		template<typename T>
		static uf::Serializer toBase64( const T& input ) {
			// ensure this is a safe type to serialize
			if ( !serializeable(input) ) return uf::stl::string("");
			pod::Userdata* userdata = uf::userdata::create(input);
			uf::Serializer res;
			res["base64"] = uf::userdata::toBase64( userdata );
			uf::userdata::destroy( userdata );
			return res;
		}
		// convert from base64 to POD
		template<typename T>
		static T fromBase64( const uf::Serializer& input ) {
			// ensure this is a safe type to serialize
			if ( !serializeable(input) ) return T();
			// ensure it's "proper"
			if ( !input["base64"].is<uf::stl::string>() ) return T();
			pod::Userdata* userdata = uf::userdata::fromBase64(input["base64"].as<uf::stl::string>());
			T res = uf::userdata::get<T>( userdata );
			uf::userdata::destroy( userdata );
			return res;
		}
	
		static uf::stl::string resolveFilename( const uf::stl::string& filename, bool compareTimes = true );
		static uf::stl::string resolveFilename( const uf::stl::string& filename, const ext::json::EncodingSettings&, bool compareTimes = true );
		bool readFromFile( const uf::stl::string& from, const uf::stl::string& hash = "" );
		bool writeToFile( const uf::stl::string& to, const ext::json::EncodingSettings& = {} ) const;
		
		void merge( const uf::Serializer& other, bool priority = true );
		void import( const uf::Serializer& other );
		ext::json::Value& path( const uf::stl::string& );

		operator Serializer::output_t() const;
		uf::Serializer& operator=( const uf::stl::string& str );
		uf::Serializer& operator=( const ext::json::base_value& json );
		uf::Serializer& operator=( const ext::json::Value& json );
	#if UF_USE_LUA
		uf::Serializer& operator=( const sol::table& json );
	#endif
		uf::Serializer& operator<<( const uf::stl::string& str );
		uf::Serializer& operator>>( uf::stl::string& str );
		const uf::Serializer& operator>>( uf::stl::string& str ) const;
	};

	typedef Serializer Metadata;
}