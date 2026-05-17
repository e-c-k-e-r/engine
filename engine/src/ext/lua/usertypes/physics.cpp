#include <uf/ext/lua/lua.h>
#if UF_USE_LUA
#include <uf/utils/math/physics.h>

namespace binds {
	bool hasBody( pod::PhysicsBody& self ) { return !!self.object; }
	pod::Vector3f& velocity( pod::PhysicsBody& self ) { return self.velocity; }
	pod::Vector3f& angularVelocity( pod::PhysicsBody& self ) { return self.angularVelocity; }

	void setVelocity( pod::PhysicsBody& self, const pod::Vector3f& v ) { uf::physics::setVelocity( self, v ); }
	void setAngularVelocity( pod::PhysicsBody& self, const pod::Quaternion<>& q, float dt = 0 ) { uf::physics::setAngularVelocity( self, q, dt ); }

	void applyVelocity( pod::PhysicsBody& self, const pod::Vector3f& v ) { uf::physics::applyVelocity( self, v ); }
	void applyAngularVelocity( pod::PhysicsBody& self, const pod::Quaternion<>& q, float dt = 0 ) { uf::physics::applyAngularVelocity( self, q, dt ); }
	
	float getMass( const pod::PhysicsBody& self ) { return self.mass; }
	void setMass( pod::PhysicsBody& self, float mass ) { self.mass = mass; }

	void enableGravity( pod::PhysicsBody& state, bool s ) {
		if ( !state.object ) return;
		if ( s ) {
			uf::physics::setGravity( state, pod::Vector3f{ 0, -9.81f, 0 } );
		} else {
			uf::physics::setGravity( state );
		}
	}
	
	void applyRotation( pod::PhysicsBody& state, const pod::Quaternion<>& q ) {
		return uf::physics::applyRotation( state, q );
	}

	std::tuple<uf::Object*, float> rayCast( pod::PhysicsBody& self, const pod::Vector3f& center, const pod::Vector3f& direction ) {
		pod::RayQuery query = uf::physics::rayCast( pod::Ray{center, direction}, self, uf::vector::norm( direction ) );
		uf::Object* object = query.hit ? query.body->object : NULL;
		float depth = query.hit ? query.contact.penetration : -1;		
		return std::make_tuple( object, depth );
	}
}

#include <uf/ext/lua/component.h>
UF_LUA_REGISTER_USERTYPE_AND_COMPONENT(pod::PhysicsBody,
	UF_LUA_REGISTER_USERTYPE_DEFINE( hasBody, UF_LUA_C_FUN(::binds::hasBody) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( velocity, UF_LUA_C_FUN(::binds::velocity) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( angularVelocity, UF_LUA_C_FUN(::binds::angularVelocity) ),
	
	UF_LUA_REGISTER_USERTYPE_DEFINE( setVelocity, UF_LUA_C_FUN(::binds::setVelocity) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( applyVelocity, UF_LUA_C_FUN(::binds::applyVelocity) ),
	
	UF_LUA_REGISTER_USERTYPE_DEFINE( setAngularVelocity, UF_LUA_C_FUN(::binds::setAngularVelocity) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( applyAngularVelocity, UF_LUA_C_FUN(::binds::applyAngularVelocity) ),

	UF_LUA_REGISTER_USERTYPE_DEFINE( applyImpulse, UF_LUA_C_FUN(uf::physics::applyImpulse) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( applyRotation, UF_LUA_C_FUN(::binds::applyRotation) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( enableGravity, UF_LUA_C_FUN(::binds::enableGravity) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( rayCast, UF_LUA_C_FUN(::binds::rayCast) ),
	
	UF_LUA_REGISTER_USERTYPE_DEFINE( getMass, UF_LUA_C_FUN(::binds::getMass) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( setMass, UF_LUA_C_FUN(::binds::setMass) )
)

#endif