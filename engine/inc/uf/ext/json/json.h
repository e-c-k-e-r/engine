#pragma once

#include <json/json.h>

namespace ext {
	namespace json {
		inline bool isTable( const Json::Value& v ) { return v.isArray() || v.isObject(); }
		inline bool isObject( const Json::Value& v ) { return v.isObject(); }
		inline bool isArray( const Json::Value& v ) { return v.isArray(); }
		inline bool isNull( const Json::Value& v ) { return v.isNull(); }
	/*
		inline bool isTable( sol::object v ) { return v.is<sol::table>(); }
		inline bool isObject( sol::object v ) { return v.is<sol::table>(); }
		inline bool isArray( sol::object v ) { return v.is<sol::table>(); }
		inline bool isNull( sol::object v ) { return v == sol::lua_nil; }

		template<typename T>
		inline bool isTable( T v ) { return v.get_type() == sol::type::table; }

		template<typename T>
		inline bool isObject( T v ) { return v.get_type() == sol::type::table; }

		template<typename T>
		inline bool isArray( T v ) { return v.get_type() == sol::type::table; }

		template<typename T>
		inline bool isNull( T v ) { return v.get_type() == sol::type::nil; }
	*/
	}
}

#include "json.inl"