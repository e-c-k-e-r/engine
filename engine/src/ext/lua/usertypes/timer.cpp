#include <uf/ext/lua/lua.h>

#include <uf/utils/math/physics.h>

UF_LUA_REGISTER_USERTYPE(uf::Timer<>,
	UF_LUA_REGISTER_USERTYPE_DEFINE( elapsed, []( uf::Timer<>& self ) {
		return self.elapsed().asDouble();
	}),
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Timer<>::start ),
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Timer<>::stop ),
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Timer<>::reset ),
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Timer<>::update ),
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Timer<>::running )
)