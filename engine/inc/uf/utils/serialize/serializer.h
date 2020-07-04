#pragma once

#include <uf/config.h>
#include <json/json.h>
#include <string>

namespace uf {
	class UF_API Serializer : public Json::Value {
	public:
		typedef std::string output_t;
		typedef std::string  input_t;
	protected:
	public:
		Serializer( const std::string& str = "" );
		Serializer( const Json::Value& );
		
		Serializer::output_t serialize() const;
		void deserialize( const std::string& );

		bool readFromFile( const std::string& from );
		bool writeToFile( const std::string& to ) const;
		
		void merge( const uf::Serializer& other, bool priority = true );

		operator Serializer::output_t();
		operator Serializer::output_t() const;
		uf::Serializer& operator=( const std::string& str );
		uf::Serializer& operator=( const Json::Value& json );
		uf::Serializer& operator<<( const std::string& str );
		uf::Serializer& operator>>( std::string& str );
		const uf::Serializer& operator>>( std::string& str ) const;
	};
}