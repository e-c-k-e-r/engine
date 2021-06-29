#pragma once

#include <uf/config.h>

#include <json/json.h>

namespace ext {
	namespace json {
		class UF_API Value;
		typedef Json::Value base_value;
		typedef Value Document;
		
		class UF_API Value : public base_value {
		public:
			Value( Json::ValueType type = Json::objectValue ) : base_value(type) {}
			Value& operator=( const Value& );

			template<typename... Args> Value& operator[]( Args... args );
			template<typename... Args> const Value& operator[]( Args... args ) const;
			template<typename... Args> Value& operator=( Args... args );

			template<typename T>
			Value& emplace_back( const T& );
		};

		inline bool isTable( const Value& v ) { return v.isArray() || v.isObject(); }
		inline bool isObject( const Value& v ) { return v.isObject(); }
		inline bool isArray( const Value& v ) { return v.isArray(); }
		inline bool isNull( const Value& v ) { return v.isNull(); }

		inline Value array() { return Value(Json::arrayValue); }
		inline Value object() { return Value(Json::objectValue); }
		inline Value null() { return Value(Json::nullValue); }
		
		inline uf::stl::vector<uf::stl::string> keys( const Value& v ) { return v.getMemberNames(); }
	}
}

template<typename... Args> ext::json::Value& ext::json::Value::operator[]( Args... args ) {
	return (ext::json::Value&) ext::json::base_value::operator[](args...);
}
template<typename... Args> const ext::json::Value& ext::json::Value::operator[]( Args... args ) const {
	return (const ext::json::Value&) ext::json::base_value::operator[](args...);
}
template<typename... Args> ext::json::Value& ext::json::Value::operator=( Args... args ) {
	ext::json::base_value::operator=(args...);
	return *this;
}

template<typename T>
ext::json::Value& ext::json::Value::emplace_back( const T& v ) {
	ext::json::base_value::append( v );
	return *this;
}