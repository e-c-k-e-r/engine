#include <uf/ext/lua/lua.h>
#if UF_USE_LUA
#include <uf/utils/math/physics.h>

namespace binds {
	pod::Vector3f& linearVelocity( pod::Physics& self ) { return self.linear.velocity; }
	pod::Quaternion<>& rotationalVelocity( pod::Physics& self ) { return self.rotational.velocity; }

	void setLinearVelocity( pod::Physics& self, const pod::Vector3f& v ) { self.linear.velocity = v; }
	void setRotationalVelocity( pod::Physics& self, const pod::Quaternion<>& v ) { self.rotational.velocity = v; }
}

UF_LUA_REGISTER_USERTYPE(pod::Physics,
	UF_LUA_REGISTER_USERTYPE_DEFINE( linearVelocity, UF_LUA_C_FUN(::binds::linearVelocity) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( rotationalVelocity, UF_LUA_C_FUN(::binds::rotationalVelocity) ),
	
	UF_LUA_REGISTER_USERTYPE_DEFINE( setLinearVelocity, UF_LUA_C_FUN(::binds::setLinearVelocity) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( setRotationalVelocity, UF_LUA_C_FUN(::binds::setRotationalVelocity) )
)
#endif