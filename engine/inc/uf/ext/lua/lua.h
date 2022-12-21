#pragma once

#include <uf/config.h>
#if UF_USE_LUA
	#if UF_USE_LUAJIT
		#define SOL_LUAJIT 1
//		#define SOL_USING_CXX_LUA_JIT 1
	#else
//		#define SOL_USING_CXX_LUA 1
	#endif
#define SOL_NO_EXCEPTIONS 1
#if UF_ENV_DREAMCAST
	#define SOL_NO_THREAD_LOCAL 1
#else
	#define SOL_ALL_SAFETIES_ON 1
#endif

#include <sol/sol.hpp>
#include <uf/utils/memory/unordered_map.h>
#include <uf/utils/singletons/pre_main.h>
#include <uf/utils/string/ext.h>

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
		
		bool UF_API run( const uf::stl::string&, bool = true );
		
		pod::LuaScript UF_API script( const uf::stl::string& );
		void UF_API script( const uf::stl::string&, pod::LuaScript& );
		
		bool UF_API run( const pod::LuaScript&, bool = true );

		sol::table createTable();
		uf::stl::string sanitize( const uf::stl::string& dirty, int index = -1 );
		std::optional<uf::stl::string> encode( sol::table table );
		std::optional<sol::table> decode( const uf::stl::string& string );
	}
}

#include "lua.inl"
#endif