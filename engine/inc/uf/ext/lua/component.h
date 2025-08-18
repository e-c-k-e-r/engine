#pragma once

#include <uf/config.h>
#if UF_USE_LUA

// agony to try and get this included within "lua.h"

#include <uf/engine/object/object.h>
namespace ext {
	namespace lua {
		// component shenanigans
		using GetComponent = sol::object(*)(uf::Object&);

		extern UF_API uf::stl::unordered_map<uf::stl::string, GetComponent> componentGetters;
		
		template<typename T>
		sol::object getComponent( uf::Object& self ) {
		    return sol::make_object(ext::lua::state, std::ref(self.getComponent<T>()));
		}
	}
}

#endif