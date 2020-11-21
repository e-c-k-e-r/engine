#pragma once

#include <uf/config.h>

#include <uf/utils/userdata/userdata.h>
#include <uf/ext/lua/lua.h>
#include <uf/ext/json/json.h>

#include <string>
#include <type_traits>

namespace uf {
//	class UF_API Serializer : public sol::table {
	class UF_API Serializer : public Json::Value {
	public:
		typedef std::string output_t;
		typedef std::string  input_t;
	
		Serializer( const std::string& str = "" );
		Serializer( const Json::Value& );
		Serializer( const sol::table& );
		
		Serializer::output_t serialize( bool pretty = false ) const;
		void deserialize( const std::string& );

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
			if ( !serializeable(input) ) return std::string("");
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
			if ( !input["base64"].is<std::string>() ) return T();
			pod::Userdata* userdata = uf::userdata::fromBase64(input["base64"].as<std::string>());
			T res = uf::userdata::get<T>( userdata );
			uf::userdata::destroy( userdata );
			return res;
		}
	
		bool readFromFile( const std::string& from );
		bool writeToFile( const std::string& to ) const;
		
		void merge( const uf::Serializer& other, bool priority = true );
		void import( const uf::Serializer& other );

		operator Serializer::output_t();
		operator Serializer::output_t() const;
		uf::Serializer& operator=( const std::string& str );
		uf::Serializer& operator=( const Json::Value& json );
		uf::Serializer& operator=( const sol::table& json );
		uf::Serializer& operator<<( const std::string& str );
		uf::Serializer& operator>>( std::string& str );
		const uf::Serializer& operator>>( std::string& str ) const;
	};

	typedef Serializer Metadata;
}