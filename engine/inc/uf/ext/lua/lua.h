#pragma once

#include <uf/config.h>
#if UF_USE_LUA

#if UF_USE_LUAJIT
	#define SOL_LUAJIT 1
#endif

#define SOL_NO_EXCEPTIONS 1

#if UF_ENV_DREAMCAST
	#define SOL_NO_THREAD_LOCAL 1
	#define UF_LUA_PCALLS 0
#else
	#define SOL_ALL_SAFETIES_ON 1
	#define UF_LUA_PCALLS 1
#endif

#if UF_LUA_PCALLS
	#define LUA_FUN sol::protected_function
#else
	#define LUA_FUN sol::function
#endif


#include <sol/sol.hpp>
#include <uf/utils/memory/unordered_map.h>
#include <uf/utils/singletons/pre_main.h>
#include <uf/utils/string/ext.h>
#include <uf/ext/json/json.h>

namespace pod {
	struct UF_API LuaScript {
		uf::stl::string file;
		sol::environment env;
	};
}

namespace ext {
	namespace lua {
		extern UF_API bool enabled;
		extern UF_API sol::state state;
		extern UF_API uf::stl::string main;
		extern UF_API uf::stl::unordered_map<uf::stl::string, uf::stl::string> modules;
		extern UF_API uf::stl::vector<std::function<void()>>* onInitializationFunctions;

		void UF_API initialize();
		void UF_API terminate();
		void UF_API onInitialization( const std::function<void()>& );
		
		bool UF_API run( const uf::stl::string&, bool = UF_LUA_PCALLS );
		
		pod::LuaScript UF_API script( const uf::stl::string& );
		void UF_API script( const uf::stl::string&, pod::LuaScript& );
		
		bool UF_API run( const pod::LuaScript&, bool = UF_LUA_PCALLS );

		sol::table createTable();
		uf::stl::string sanitize( const uf::stl::string& dirty, int index = -1 );
		std::optional<ext::json::Value> encode( sol::table table );
		std::optional<sol::table> decode( const ext::json::Value& string );
	}
}

#include "lua.inl"
#else

#define UF_LUA_REGISTER_USERTYPE_MEMBER(...)
#define UF_LUA_REGISTER_USERTYPE(...)
#define UF_LUA_REGISTER_COMPONENT(...)
#define UF_LUA_REGISTER_USERTYPE_AND_COMPONENT(...)

#endif