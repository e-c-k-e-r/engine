#pragma once

#include <uf/config.h>

#include <uf/ext/lua/lua.h>

namespace ext {
	namespace json {
		class UF_API Value;
		typedef sol::table base_value;
		typedef Value Document;
		
		class UF_API Value : public base_value {
		public:
			Value& operator=( const Value& );

			template<typename... Args> Value& operator[]( Args... args );
			template<typename... Args> const Value& operator[]( Args... args ) const;
			template<typename... Args> Value& operator=( Args... args );
		};

		inline bool isTable( sol::object v ) { return v.is<sol::table>(); }
		inline bool isObject( sol::object v ) { return v.is<sol::table>(); }
		inline bool isArray( sol::object v ) { return v.is<sol::table>(); }
		inline bool isNull( sol::object v ) { return v == sol::lua_nil; }
		
		template<typename T> inline bool isTable( T v );
		template<typename T> inline bool isObject( T v );
		template<typename T> inline bool isArray( T v );
		template<typename T> inline bool isNull( T v );
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
inline bool ext::json::isTable( T v ) { return v.get_type() == sol::type::table; }

template<typename T>
inline bool ext::json::isObject( T v ) { return v.get_type() == sol::type::table; }

template<typename T>
inline bool ext::json::isArray( T v ) { return v.get_type() == sol::type::table; }

template<typename T>
inline bool ext::json::isNull( T v ) { return v.get_type() == sol::type::nil; }