#include <uf/ext/lua/lua.h>
#if UF_USE_LUA
#include <uf/utils/math/physics.h>

namespace binds {
	pod::Vector3f& linearVelocity( pod::Physics& self ) { return self.linear.velocity; }
	pod::Quaternion<>& rotationalVelocity( pod::Physics& self ) { return self.rotational.velocity; }

	void setLinearVelocity( pod::Physics& self, const pod::Vector3f& v ) { self.linear.velocity = v; }
	void setRotationalVelocity( pod::Physics& self, const pod::Quaternion<>& v ) { self.rotational.velocity = v; }

	void enableGravity( pod::PhysicsState& state, bool s ) {
		if ( !state.body ) return;
		state.body->enableGravity(s);
	}

	std::tuple<uf::Object*, float> rayCast( pod::Physics& self, const pod::Vector3f& center, const pod::Vector3f& direction ) {
		float depth = -1;
		uf::Object* object = uf::physics::impl::rayCast(self, center, direction, depth);
		
		return std::make_tuple( object, depth );
	}
}

UF_LUA_REGISTER_USERTYPE(pod::Physics,
	UF_LUA_REGISTER_USERTYPE_DEFINE( linearVelocity, UF_LUA_C_FUN(::binds::linearVelocity) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( rotationalVelocity, UF_LUA_C_FUN(::binds::rotationalVelocity) ),
	
	UF_LUA_REGISTER_USERTYPE_DEFINE( setLinearVelocity, UF_LUA_C_FUN(::binds::setLinearVelocity) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( setRotationalVelocity, UF_LUA_C_FUN(::binds::setRotationalVelocity) )
)

UF_LUA_REGISTER_USERTYPE(pod::PhysicsState,
	UF_LUA_REGISTER_USERTYPE_DEFINE( setVelocity, UF_LUA_C_FUN(uf::physics::impl::setVelocity) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( setImpulse, UF_LUA_C_FUN(uf::physics::impl::setImpulse) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( applyImpulse, UF_LUA_C_FUN(uf::physics::impl::applyImpulse) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( applyMovement, UF_LUA_C_FUN(uf::physics::impl::applyMovement) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( applyVelocity, UF_LUA_C_FUN(uf::physics::impl::applyVelocity) ),
//	UF_LUA_REGISTER_USERTYPE_DEFINE( applyRotation, UF_LUA_C_FUN(uf::physics::impl::applyRotation) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( enableGravity, UF_LUA_C_FUN(::binds::enableGravity) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( activateCollision, UF_LUA_C_FUN(uf::physics::impl::activateCollision) ),
	
	UF_LUA_REGISTER_USERTYPE_DEFINE( rayCast, UF_LUA_C_FUN(::binds::rayCast) ),
	
	UF_LUA_REGISTER_USERTYPE_DEFINE( getMass, UF_LUA_C_FUN(uf::physics::impl::getMass) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( setMass, UF_LUA_C_FUN(uf::physics::impl::setMass) )
)

#endif