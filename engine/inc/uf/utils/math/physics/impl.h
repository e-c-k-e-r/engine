#pragma once

#include <uf/config.h>
#include <uf/utils/time/time.h>
#include <uf/utils/math/quant.h>

#include "structs.h"
#include "narrowphase.h"
#include "constraints.h"

namespace uf {
	namespace physics {
		void UF_API initialize();
		void UF_API initialize( uf::Object& );
		void UF_API initialize( pod::World& );
		
		void UF_API tick();
		void UF_API tick( uf::Object& );

		void UF_API tick( float );
		void UF_API tick( uf::Object&, float );
		void UF_API tick( pod::World&, float );

		void UF_API terminate();
		void UF_API terminate( uf::Object& );
		void UF_API terminate( pod::World& );

		void UF_API step( pod::World&, float dt );
		void UF_API substep( pod::World&, float dt, int substeps );

		void UF_API setMass( pod::PhysicsBody& body, float mass = 0.0f );
		void UF_API setColliderCategory( pod::PhysicsBody&, uint32_t category = pod::Collider::CATEGORY_ALL );
		void UF_API setColliderCategory( pod::PhysicsBody&, const uf::stl::string& );
		void UF_API setColliderMask( pod::PhysicsBody&, uint32_t mask = pod::Collider::MASK_ALL );
		void UF_API setColliderMask( pod::PhysicsBody&, const uf::stl::string& );
		void UF_API setGravity( pod::PhysicsBody&, const pod::Vector3f& = { NAN, NAN, NAN } );
		pod::Vector3f UF_API getGravity( pod::PhysicsBody& );

		void UF_API updateInertia( pod::PhysicsBody& body );

		void UF_API applyForce( pod::PhysicsBody& body, const pod::Vector3f& force );
		void UF_API applyForceAtPoint( pod::PhysicsBody& body, const pod::Vector3f& force, const pod::Vector3f& point );
		void UF_API applyImpulse( pod::PhysicsBody& body, const pod::Vector3f& impulse );
		void UF_API applyTorque( pod::PhysicsBody& body, const pod::Vector3f& torque );

		void UF_API setVelocity( pod::PhysicsBody& body, const pod::Vector3f& velocity );
		void UF_API applyVelocity( pod::PhysicsBody& body, const pod::Vector3f& velocity );
		
		void UF_API setAngularVelocity( pod::PhysicsBody& body, const pod::Vector3f& velocity );
		void UF_API setAngularVelocity( pod::PhysicsBody& body, const pod::Quaternion<>& q, float dt = 0 );
		void UF_API applyAngularVelocity( pod::PhysicsBody& body, const pod::Vector3f& velocity );
		void UF_API applyAngularVelocity( pod::PhysicsBody& body, const pod::Quaternion<>& q, float dt = 0 );

		void UF_API applyRotation( pod::PhysicsBody& body, const pod::Quaternion<>& q );
		void UF_API applyRotation( pod::PhysicsBody& body, const pod::Vector3f& axis, float angle );

		pod::World& UF_API getWorld();
		
		pod::PhysicsBody& UF_API create( uf::Object&, float mass = 0.0f, const pod::Vector3f& = {} );
		pod::PhysicsBody& UF_API create( pod::World&, uf::Object&, float mass = 0.0f, const pod::Vector3f& = {} );
		void UF_API destroy( uf::Object& );
		void UF_API destroy( pod::PhysicsBody& );

		pod::Constraint& UF_API constrain( uf::Object&, uf::Object& );
		pod::Constraint& UF_API constrain( pod::PhysicsBody&, pod::PhysicsBody& );
		pod::Constraint& UF_API constrain( pod::World&, uf::Object&, uf::Object& );
		pod::Constraint& UF_API constrain( pod::World&, pod::PhysicsBody&, pod::PhysicsBody& );
		
		void UF_API unconstrain( pod::PhysicsBody& );
		void UF_API unconstrain( uf::Object& );
	}
}