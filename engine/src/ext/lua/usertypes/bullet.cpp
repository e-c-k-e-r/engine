#include <uf/ext/lua/lua.h>
#if UF_USE_LUA && UF_USE_BULLET
#include <uf/utils/math/physics.h>
#include <uf/ext/bullet/bullet.h>

UF_LUA_REGISTER_USERTYPE(pod::Bullet,
	UF_LUA_REGISTER_USERTYPE_DEFINE( setVelocity, UF_LUA_C_FUN(ext::bullet::setVelocity) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( applyImpulse, UF_LUA_C_FUN(ext::bullet::applyImpulse) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( applyMovement, UF_LUA_C_FUN(ext::bullet::applyMovement) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( applyVelocity, UF_LUA_C_FUN(ext::bullet::applyVelocity) ),
//	UF_LUA_REGISTER_USERTYPE_DEFINE( applyRotation, UF_LUA_C_FUN(ext::bullet::applyRotation) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( activateCollision, UF_LUA_C_FUN(ext::bullet::activateCollision) )
)

#endif