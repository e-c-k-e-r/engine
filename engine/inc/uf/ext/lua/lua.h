#pragma once

#include <uf/config.h>

#define SOL_ALL_SAFETIES_ON 1

#include <sol/sol.hpp>

namespace pod {
	struct UF_API LuaScript {
		std::string filename;
		std::string header;
		sol::environment env;
	};
}

namespace ext {
	namespace lua {
		extern UF_API sol::state state;
		extern UF_API std::string main;
		void UF_API initialize();
		void UF_API terminate();
		
		bool UF_API run( const std::string&, bool = true );
		
		pod::LuaScript UF_API script( const std::string& );
		bool UF_API run( const pod::LuaScript& );
	}
}

#include "lua.inl"