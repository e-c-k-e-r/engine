#include <uf/ext/lua/lua.h>

#include <uf/utils/camera/camera.h>

UF_LUA_REGISTER_USERTYPE(uf::Camera,
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Camera::update )
)
