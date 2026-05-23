#include <uf/ext/lua/lua.h>
#if UF_USE_LUA
#include <uf/utils/mesh/mesh.h>

namespace binds {
	void updateDescriptor( uf::Mesh& self ) {
		self.updateDescriptor();
	}
}

#include <uf/ext/lua/component.h>
UF_LUA_REGISTER_USERTYPE_AND_COMPONENT(uf::Mesh,
	UF_LUA_REGISTER_USERTYPE_DEFINE( updateDescriptor, UF_LUA_C_FUN( ::binds::updateDescriptor ) )
)
#endif