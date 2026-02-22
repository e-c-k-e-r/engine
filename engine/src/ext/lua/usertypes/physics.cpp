#include <uf/ext/lua/lua.h>
#if UF_USE_LUA
#include <uf/utils/math/physics.h>

namespace binds {
	bool hasBody( pod::PhysicsBody& self ) { return !!self.object; }
	pod::Vector3f& velocity( pod::PhysicsBody& self ) { return self.velocity; }
	void setVelocity( pod::PhysicsBody& self, const pod::Vector3f& v ) { self.velocity = v; }
	void applyVelocity( pod::PhysicsBody& self, const pod::Vector3f& v ) { self.velocity += v; }
	
	float getMass( const pod::PhysicsBody& self ) { return self.mass; }
	void setMass( pod::PhysicsBody& self, float mass ) { self.mass = mass; }

	void enableGravity( pod::PhysicsBody& state, bool s ) {
		if ( !state.object ) return;
	#if UF_USE_REACTPHYSICS
		state.collider.body->enableGravity(s);
	#else
		if ( s ) {
			uf::physics::impl::setGravity( state, pod::Vector3f{ 0, -9.81f, 0 } );
		} else {
			uf::physics::impl::setGravity( state );
		}
	#endif
	}
	
	void applyRotation( pod::PhysicsBody& state, const pod::Quaternion<>& q ) {
		return uf::physics::impl::applyRotation( state, q );
	}

	std::tuple<uf::Object*, float> rayCast( pod::PhysicsBody& self, const pod::Vector3f& center, const pod::Vector3f& direction ) {
		pod::RayQuery query = uf::physics::impl::rayCast( pod::Ray{center, direction}, self, uf::vector::norm( direction ) );
		uf::Object* object = query.hit ? query.body->object : NULL;
		float depth = query.hit ? query.contact.penetration : -1;		
		return std::make_tuple( object, depth );
	}
}

#include <uf/ext/lua/component.h>
UF_LUA_REGISTER_USERTYPE_AND_COMPONENT(pod::PhysicsBody,
	UF_LUA_REGISTER_USERTYPE_DEFINE( hasBody, UF_LUA_C_FUN(::binds::hasBody) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( velocity, UF_LUA_C_FUN(::binds::velocity) ),
	
	UF_LUA_REGISTER_USERTYPE_DEFINE( setVelocity, UF_LUA_C_FUN(::binds::setVelocity) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( applyVelocity, UF_LUA_C_FUN(::binds::applyVelocity) ),

	UF_LUA_REGISTER_USERTYPE_DEFINE( applyImpulse, UF_LUA_C_FUN(uf::physics::impl::applyImpulse) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( applyRotation, UF_LUA_C_FUN(::binds::applyRotation) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( enableGravity, UF_LUA_C_FUN(::binds::enableGravity) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( rayCast, UF_LUA_C_FUN(::binds::rayCast) ),
	
	UF_LUA_REGISTER_USERTYPE_DEFINE( getMass, UF_LUA_C_FUN(::binds::getMass) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( setMass, UF_LUA_C_FUN(::binds::setMass) )
)

#endif