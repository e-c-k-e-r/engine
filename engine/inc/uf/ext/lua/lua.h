#pragma once

#include <uf/config.h>
#if UF_USE_LUA
	#if UF_USE_LUAJIT
		#define SOL_LUAJIT 1
		#define SOL_USING_CXX_LUA_JIT 1
	#endif
#define SOL_NO_EXCEPTIONS 1
#define SOL_ALL_SAFETIES_ON 1

#include <sol/sol.hpp>
#include <unordered_map>
#include <uf/utils/singletons/pre_main.h>
#include <uf/utils/string/ext.h>

namespace pod {
	struct UF_API LuaScript {
		std::string file;
		sol::environment env;
	};
}

namespace ext {
	namespace lua {
		extern UF_API sol::state state;
		extern UF_API std::string main;
		extern UF_API std::unordered_map<std::string, std::string> modules;
		extern UF_API std::vector<std::function<void()>>* onInitializationFunctions;

		void UF_API initialize();
		void UF_API terminate();
		void UF_API onInitialization( const std::function<void()>& );
		
		bool UF_API run( const std::string&, bool = true );
		
		pod::LuaScript UF_API script( const std::string& );
		bool UF_API run( const pod::LuaScript&, bool = true );

		sol::table createTable();
		std::string sanitize( const std::string& dirty, int index = -1 );
		std::optional<std::string> encode( sol::table table );
		std::optional<sol::table> decode( const std::string& string );
	}
}

#include "lua.inl"
#endif