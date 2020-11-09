#include <uf/ext/lua/lua.h>

#include <uf/utils/math/matrix.h>

UF_LUA_REGISTER_USERTYPE(pod::Matrix4f,
	UF_LUA_REGISTER_USERTYPE_DEFINE( m, []( pod::Matrix4f& self, const size_t& index ) {
		return self[index];
	})
)
