#include <uf/ext/lua/lua.h>
#if UF_USE_LUA
#include <uf/utils/math/physics.h>

namespace binds {
	namespace body {
		bool initialized( pod::PhysicsBody& self ) { return !!self.world; }
		uf::Object* getObject( pod::PhysicsBody& self ) { return self.object; }
		pod::Collider& getCollider( pod::PhysicsBody& self ) { return self.collider; }
		pod::Vector3f& getVelocity( pod::PhysicsBody& self ) { return self.velocity; }
		pod::Vector3f& getAngularVelocity( pod::PhysicsBody& self ) { return self.angularVelocity; }

		void setVelocity( pod::PhysicsBody& self, const pod::Vector3f& v ) { uf::physics::setVelocity( self, v ); }
		void setAngularVelocity( pod::PhysicsBody& self, const pod::Quaternion<>& q, float dt = 0 ) { uf::physics::setAngularVelocity( self, q, dt ); }

		void applyVelocity( pod::PhysicsBody& self, const pod::Vector3f& v ) { uf::physics::applyVelocity( self, v ); }
		void applyAngularVelocity( pod::PhysicsBody& self, const pod::Quaternion<>& q, float dt = 0 ) { uf::physics::applyAngularVelocity( self, q, dt ); }
		
		float getMass( const pod::PhysicsBody& self ) { return self.inverseMass == 0.0f ? 0.0f : 1.0f / self.inverseMass; }
		void setMass( pod::PhysicsBody& self, float mass ) { self.inverseMass = ( mass == 0.0f ? 0.0f : 1.0f / mass );  }

		void setGravity( pod::PhysicsBody& self, const pod::Vector3f& gravity ) {
			uf::physics::setGravity( self, gravity );
		}
		void enableGravity( pod::PhysicsBody& self, bool s ) {
			if ( s ) uf::physics::setGravity( self, pod::Vector3f{ 0, -9.81f, 0 } );
			else uf::physics::setGravity( self );
		}
		
		void applyRotation( pod::PhysicsBody& self, const pod::Quaternion<>& q ) {
			return uf::physics::applyRotation( self, q );
		}

		pod::Transform<> getTransform( pod::PhysicsBody& body ) {
			pod::Transform<> t;
			t.position = body.offsetPosition;
			t.orientation = body.offsetOrientation;
			t.reference = body.transform;
			return uf::transform::flatten( t );
		}

		auto getCollisionEvents( const pod::PhysicsBody& body ) {
			auto events = uf::physics::getCollisionEvents( body );
			return sol::as_table( events );
		}

		std::tuple<uf::Object*, float> rayCast( pod::PhysicsBody& self, const pod::Vector3f& center, const pod::Vector3f& direction ) {
			pod::RayQuery query = uf::physics::rayCast( pod::Ray{center, uf::vector::normalize( direction )}, self, uf::vector::norm( direction ) );
			uf::Object* object = query.hit ? query.body->object : NULL;
			float depth = query.hit ? query.contact.penetration : -1;		
			return std::make_tuple( object, depth );
		}

		pod::AABB bounds( const pod::PhysicsBody& self ) {
			return self.bounds;
		}

		pod::PhysicsBody& asAabb( pod::PhysicsBody& self, const pod::AABB& shape ) {
			return uf::physics::initialize( self, shape );
		}
		pod::PhysicsBody& asObb( pod::PhysicsBody& self, const pod::OBB& shape ) {
			return uf::physics::initialize( self, shape );
		}
		pod::PhysicsBody& asSphere( pod::PhysicsBody& self, const pod::Sphere& shape ) {
			return uf::physics::initialize( self, shape );
		}
		pod::PhysicsBody& asPlane( pod::PhysicsBody& self, const pod::Plane& shape ) {
			return uf::physics::initialize( self, shape );
		}
		pod::PhysicsBody& asCapsule( pod::PhysicsBody& self, const pod::Capsule& shape ) {
			return uf::physics::initialize( self, shape );
		}
		pod::PhysicsBody& asMesh( pod::PhysicsBody& self, const uf::Mesh& shape, sol::optional<bool> convex ) {
			return uf::physics::initialize( self, shape, convex.value_or(false) );
		}

		pod::Constraint& constrain( pod::PhysicsBody& a, pod::PhysicsBody& b ) {
			return uf::physics::constrain( a, b );
		}
		void unconstrain( pod::PhysicsBody& body ) {
			return uf::physics::unconstrain( body );
		}
	}
	namespace collider {
		const pod::AABB& asAabb( pod::Collider& self ) {
			UF_ASSERT( self.type == pod::ShapeType::AABB );
			return self.aabb;
		}
		const pod::OBB& asObb( pod::Collider& self ) {
			UF_ASSERT( self.type == pod::ShapeType::OBB );
			return self.obb;
		}
		const pod::Sphere& asSphere( pod::Collider& self ) {
			UF_ASSERT( self.type == pod::ShapeType::SPHERE );
			return self.sphere;
		}
		const pod::Plane& asPlane( pod::Collider& self ) {
			UF_ASSERT( self.type == pod::ShapeType::PLANE );
			return self.plane;
		}
		const pod::Capsule& asCapsule( pod::Collider& self ) {
			UF_ASSERT( self.type == pod::ShapeType::CAPSULE );
			return self.capsule;
		}
		const uf::Mesh& asMesh( pod::Collider& self ) {
			UF_ASSERT( self.type == pod::ShapeType::MESH );
			return *self.mesh.mesh;
		}
	}
	namespace constraint {
		void setLimits( pod::Constraint& self, float lower, float upper ) {
			return uf::physics::setConstraintLimits( self, lower, upper );
		}
		pod::Constraint& asBallSocket( pod::Constraint& self, const pod::Vector3f& joint ) {
			return uf::physics::constrainBallSocket( self, joint );
		}
		pod::Constraint& asConeTwist( pod::Constraint& self, const pod::Vector3f& joint, const pod::Vector3f& axis, sol::optional<float> swingLimit, sol::optional<float> twistLimit ) {
			return uf::physics::constrainConeTwist( self, joint, axis, swingLimit.value_or(M_PI / 4.0f), twistLimit.value_or(M_PI / 8.0f) );
		}
		pod::Constraint& asDistance( pod::Constraint& self, const pod::Vector3f& pA, const pod::Vector3f& pB, sol::optional<bool> isRope ) {
			return uf::physics::constrainDistance( self, pA, pB, isRope.value_or(false) );
		}
		pod::Constraint& asHinge( pod::Constraint& self, const pod::Vector3f& joint, const pod::Vector3f& axis ) {
			return uf::physics::constrainHinge( self, joint, axis );
		}
		pod::Constraint& asMotor( pod::Constraint& self, float targetVelocity, float maxForceOrTorque ) {
			return uf::physics::constrainMotor( self, targetVelocity, maxForceOrTorque );
		}
		pod::Constraint& asSlider( pod::Constraint& self, const pod::Vector3f& joint, const pod::Vector3f& axis, float lowerLimit, float upperLimit ) {
			return uf::physics::constrainSlider( self, joint, axis, lowerLimit, upperLimit );
		}
		pod::Constraint& asSpring( pod::Constraint& self, const pod::Vector3f& pA, const pod::Vector3f& pB, float stiffness, float damping ) {
			return uf::physics::constrainSpring( self, pA, pB, stiffness, damping );
		}
		pod::Constraint& asWeld( pod::Constraint& self, const pod::Vector3f& joint, const pod::Vector3f& axis ) {
			return uf::physics::constrainWeld( self, joint, axis );
		}
	}
}

#include <uf/ext/lua/component.h>

UF_LUA_REGISTER_ENUM(pod::ShapeType,
	"AABB",        pod::ShapeType::AABB,
	"OBB",         pod::ShapeType::OBB,
	"SPHERE",      pod::ShapeType::SPHERE,
	"PLANE",       pod::ShapeType::PLANE,
	"CAPSULE",     pod::ShapeType::CAPSULE,
	"TRIANGLE",    pod::ShapeType::TRIANGLE,
	"MESH",        pod::ShapeType::MESH,
	"CONVEX_HULL", pod::ShapeType::CONVEX_HULL
)

UF_LUA_REGISTER_USERTYPE(pod::Collider,
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Collider::type),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Collider::category),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Collider::mask),

	UF_LUA_REGISTER_USERTYPE_DEFINE( asAabb, UF_LUA_C_FUN(::binds::collider::asAabb) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( asObb, UF_LUA_C_FUN(::binds::collider::asObb) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( asSphere, UF_LUA_C_FUN(::binds::collider::asSphere) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( asPlane, UF_LUA_C_FUN(::binds::collider::asPlane) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( asCapsule, UF_LUA_C_FUN(::binds::collider::asCapsule) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( asMesh, UF_LUA_C_FUN(::binds::collider::asMesh) )
)
UF_LUA_REGISTER_USERTYPE(pod::CollisionEvent,
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::CollisionEvent::state),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::CollisionEvent::a),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::CollisionEvent::b),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::CollisionEvent::point),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::CollisionEvent::normal),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::CollisionEvent::impulse),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::CollisionEvent::featureA),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::CollisionEvent::featureB)
)
UF_LUA_REGISTER_USERTYPE_AND_COMPONENT(pod::AABB,
	sol::call_constructor, sol::initializers( 
		[]( pod::AABB& self ) {
			return self = {};
		},
		[]( pod::AABB& self, const pod::AABB& copy ) {
			return self = copy;
		},
		[]( pod::AABB& self, const pod::Vector3f& min, const pod::Vector3f& max ) {
			return self = pod::AABB{ min, max };
		}
	),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::AABB::min),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::AABB::max)
)
UF_LUA_REGISTER_USERTYPE_AND_COMPONENT(pod::OBB,
	sol::call_constructor, sol::initializers( 
		[]( pod::OBB& self ) {
			return self = {};
		},
		[]( pod::OBB& self, const pod::OBB& copy ) {
			return self = copy;
		},
	#if OBB_EXTENT_CENTER
		[]( pod::OBB& self, const pod::AABB& copy ) {
			return self = pod::OBB{ .extent = uf::vector::abs(copy.max - copy.min) * 0.5f, .center = (copy.max + copy.min) * 0.5f };
		},
		[]( pod::OBB& self, const pod::Vector3f& extent, const pod::Vector3f& center ) {
			return self = pod::OBB{ .extent = extent, .center = center };
		}
	#else
		[]( pod::OBB& self, const pod::AABB& copy ) {
			return self = pod::OBB{ .center = (copy.max + copy.min) * 0.5f, .extent = uf::vector::abs(copy.max - copy.min) * 0.5f };
		},
		[]( pod::OBB& self, const pod::Vector3f& center, const pod::Vector3f& extent ) {
			return self = pod::OBB{ .center = center, .extent = extent };
		}
	#endif
	),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::OBB::extent),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::OBB::center)
)
UF_LUA_REGISTER_USERTYPE(pod::Sphere,
	sol::call_constructor, sol::initializers( 
		[]( pod::Sphere& self ) {
			return self = {};
		},
		[]( pod::Sphere& self, const pod::Sphere& copy ) {
			return self = copy;
		},
		[]( pod::Sphere& self, float radius ) {
			return self = pod::Sphere{ radius };
		}
	),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Sphere::radius)
)
UF_LUA_REGISTER_USERTYPE(pod::Capsule,
	sol::call_constructor, sol::initializers( 
		[]( pod::Capsule& self ) {
			return self = {};
		},
		[]( pod::Capsule& self, const pod::Capsule& copy ) {
			return self = copy;
		},
		[]( pod::Capsule& self, float radius, pod::Vector3f up ) {
			//if ( up == pod::Vector3f{} ) up = pod::Vector3f{0,1,0};
			return self = pod::Capsule{ .radius = radius, .up = up };
		}
	),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Capsule::radius),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Capsule::up)
)
UF_LUA_REGISTER_USERTYPE(pod::Plane,
	sol::call_constructor, sol::initializers( 
		[]( pod::Plane& self ) {
			return self = {};
		},
		[]( pod::Plane& self, const pod::Plane& copy ) {
			return self = copy;
		},
		[]( pod::Plane& self, const pod::Vector3f& normal, float offset ) {
			return self = pod::Plane{ .normal = normal, .offset = offset };
		}
	),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Plane::normal),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Plane::offset)
)
UF_LUA_REGISTER_USERTYPE(pod::Ray,
	sol::call_constructor, sol::initializers( 
		[]( pod::Ray& self ) {
			return self = {};
		},
		[]( pod::Ray& self, const pod::Ray& copy ) {
			return self = copy;
		},
		[]( pod::Ray& self, const pod::Vector3f& origin, const pod::Vector3f& direction ) {
			return self = pod::Ray{ origin, direction };
		}
	),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Ray::origin),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Ray::direction)
)

UF_LUA_REGISTER_USERTYPE_AND_COMPONENT(pod::PhysicsBody,
	UF_LUA_REGISTER_USERTYPE_DEFINE( initialized, UF_LUA_C_FUN(::binds::body::initialized) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( getObject, UF_LUA_C_FUN(::binds::body::getObject) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( getCollider, UF_LUA_C_FUN(::binds::body::getCollider) ),
	
	UF_LUA_REGISTER_USERTYPE_DEFINE( getVelocity, UF_LUA_C_FUN(::binds::body::getVelocity) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( setVelocity, UF_LUA_C_FUN(::binds::body::setVelocity) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( applyVelocity, UF_LUA_C_FUN(::binds::body::applyVelocity) ),
	
	UF_LUA_REGISTER_USERTYPE_DEFINE( getAngularVelocity, UF_LUA_C_FUN(::binds::body::getAngularVelocity) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( setAngularVelocity, UF_LUA_C_FUN(::binds::body::setAngularVelocity) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( applyAngularVelocity, UF_LUA_C_FUN(::binds::body::applyAngularVelocity) ),

	UF_LUA_REGISTER_USERTYPE_DEFINE( applyImpulse, UF_LUA_C_FUN(uf::physics::applyImpulse) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( applyRotation, UF_LUA_C_FUN(::binds::body::applyRotation) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( getTransform, UF_LUA_C_FUN(::binds::body::getTransform) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( getCollisionEvents, UF_LUA_C_FUN(::binds::body::getCollisionEvents) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( setGravity, UF_LUA_C_FUN(::binds::body::setGravity) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( enableGravity, UF_LUA_C_FUN(::binds::body::enableGravity) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( rayCast, UF_LUA_C_FUN(::binds::body::rayCast) ),
	
	UF_LUA_REGISTER_USERTYPE_DEFINE( getMass, UF_LUA_C_FUN(::binds::body::getMass) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( setMass, UF_LUA_C_FUN(::binds::body::setMass) ),
	
	UF_LUA_REGISTER_USERTYPE_DEFINE( bounds, UF_LUA_C_FUN(::binds::body::bounds) ),

	UF_LUA_REGISTER_USERTYPE_DEFINE( asAabb, UF_LUA_C_FUN(::binds::body::asAabb) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( asObb, UF_LUA_C_FUN(::binds::body::asObb) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( asSphere, UF_LUA_C_FUN(::binds::body::asSphere) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( asPlane, UF_LUA_C_FUN(::binds::body::asPlane) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( asCapsule, UF_LUA_C_FUN(::binds::body::asCapsule) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( asMesh, UF_LUA_C_FUN(::binds::body::asMesh) ),

	UF_LUA_REGISTER_USERTYPE_DEFINE( constrain, UF_LUA_C_FUN(::binds::body::constrain) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( unconstrain, UF_LUA_C_FUN(::binds::body::unconstrain) )
)

UF_LUA_REGISTER_USERTYPE(pod::Constraint,
	UF_LUA_REGISTER_USERTYPE_DEFINE( setLimits, UF_LUA_C_FUN(::binds::constraint::setLimits) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( asBallSocket, UF_LUA_C_FUN(::binds::constraint::asBallSocket) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( asConeTwist, UF_LUA_C_FUN(::binds::constraint::asConeTwist) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( asDistance, UF_LUA_C_FUN(::binds::constraint::asDistance) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( asHinge, UF_LUA_C_FUN(::binds::constraint::asHinge) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( asMotor, UF_LUA_C_FUN(::binds::constraint::asMotor) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( asSlider, UF_LUA_C_FUN(::binds::constraint::asSlider) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( asSpring, UF_LUA_C_FUN(::binds::constraint::asSpring) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( asWeld, UF_LUA_C_FUN(::binds::constraint::asWeld) )
)

#endif