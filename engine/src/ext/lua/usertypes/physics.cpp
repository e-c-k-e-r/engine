#include <uf/ext/lua/lua.h>
#if UF_USE_LUA && UF_USE_REACTPHYSICS
#include <uf/utils/math/physics.h>

namespace binds {
	bool hasBody( pod::Physics& self ) { return self.body; }
	pod::Vector3f& linearVelocity( pod::Physics& self ) { return self.velocity; }
	pod::Quaternion<>& rotationalVelocity( pod::Physics& self ) { return self.angularVelocity; }

	void setLinearVelocity( pod::Physics& self, const pod::Vector3f& v ) { self.velocity = v; }
	void setRotationalVelocity( pod::Physics& self, const pod::Quaternion<>& v ) { self.angularVelocity = v; }

	void enableGravity( pod::PhysicsState& state, bool s ) {
		if ( !state.body ) return;
		state.body->enableGravity(s);
	}
	
	void applyRotation( pod::PhysicsState& state, const pod::Quaternion<>& q ) {
		return uf::physics::impl::applyRotation( state, q );
	}

	std::tuple<uf::Object*, float> rayCast( pod::Physics& self, const pod::Vector3f& center, const pod::Vector3f& direction ) {
		float depth = -1;
		uf::Object* object = uf::physics::impl::rayCast(self, center, direction, depth);
		
		return std::make_tuple( object, depth );
	}
}

#include <uf/ext/lua/component.h>
UF_LUA_REGISTER_USERTYPE_AND_COMPONENT(pod::Physics,
	UF_LUA_REGISTER_USERTYPE_DEFINE( hasBody, UF_LUA_C_FUN(::binds::hasBody) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( linearVelocity, UF_LUA_C_FUN(::binds::linearVelocity) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( rotationalVelocity, UF_LUA_C_FUN(::binds::rotationalVelocity) ),
	
	UF_LUA_REGISTER_USERTYPE_DEFINE( setLinearVelocity, UF_LUA_C_FUN(::binds::setLinearVelocity) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( setRotationalVelocity, UF_LUA_C_FUN(::binds::setRotationalVelocity) ),

	UF_LUA_REGISTER_USERTYPE_DEFINE( setVelocity, UF_LUA_C_FUN(uf::physics::impl::setVelocity) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( setImpulse, UF_LUA_C_FUN(uf::physics::impl::setImpulse) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( applyImpulse, UF_LUA_C_FUN(uf::physics::impl::applyImpulse) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( applyMovement, UF_LUA_C_FUN(uf::physics::impl::applyMovement) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( applyVelocity, UF_LUA_C_FUN(uf::physics::impl::applyVelocity) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( applyRotation, UF_LUA_C_FUN(::binds::applyRotation) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( enableGravity, UF_LUA_C_FUN(::binds::enableGravity) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( activateCollision, UF_LUA_C_FUN(uf::physics::impl::activateCollision) ),
	
	UF_LUA_REGISTER_USERTYPE_DEFINE( rayCast, UF_LUA_C_FUN(::binds::rayCast) ),
	
	UF_LUA_REGISTER_USERTYPE_DEFINE( getMass, UF_LUA_C_FUN(uf::physics::impl::getMass) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( setMass, UF_LUA_C_FUN(uf::physics::impl::setMass) )
)

#endif