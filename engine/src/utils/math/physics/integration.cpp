#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/integration.h>
#include <uf/utils/math/physics/narrowphase.h>

float impl::computeEffectiveMass( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Vector3f& rA, const pod::Vector3f& rB, const pod::Vector3f& n ) {
	float inverseMass = 0.0f;
	if ( !a.isStatic ) inverseMass += a.inverseMass;
	if ( !b.isStatic ) inverseMass += b.inverseMass;

	float angularTermA = 0.0f;
	float angularTermB = 0.0f;

	if ( !a.isStatic ) {
		auto invIa = impl::computeWorldInverseInertia(a);
		auto crossA = uf::vector::cross(rA, n);
		auto I_crossA = uf::matrix::multiply(invIa, crossA);
		angularTermA = uf::vector::dot(uf::vector::cross(I_crossA, rA), n);
	}
	if ( !b.isStatic ) {
		auto invIb = impl::computeWorldInverseInertia(b);
		auto crossB = uf::vector::cross(rB, n);
		auto I_crossB = uf::matrix::multiply(invIb, crossB);
		angularTermB = uf::vector::dot(uf::vector::cross(I_crossB, rB), n);
	}

	// to-do: assert / handle result == 0 to avoid division by zero (this probably only would happen with two static bodies colliding, which never should happen)
	return inverseMass + angularTermA + angularTermB;
}

void impl::applyImpulseTo( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Vector3f& rA, const pod::Vector3f& rB, const pod::Vector3f& impulse ) {
	if ( !a.isStatic ) {
		a.velocity -= impulse * a.inverseMass;
		pod::Matrix3f invIa = impl::computeWorldInverseInertia( a );
		a.angularVelocity -= uf::matrix::multiply( invIa, uf::vector::cross(rA, impulse) );
	}
	if ( !b.isStatic ) {
		b.velocity += impulse * b.inverseMass;
		pod::Matrix3f invIb = impl::computeWorldInverseInertia( b );
		b.angularVelocity += uf::matrix::multiply( invIb, uf::vector::cross(rB, impulse) );
	}
}
void impl::applyPseudoImpulseTo( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Vector3f& rA, const pod::Vector3f& rB, const pod::Vector3f& impulse ) {
	if ( !a.isStatic ) {
		a.pseudoVelocity -= impulse * a.inverseMass;
		pod::Matrix3f invIa = impl::computeWorldInverseInertia( a );
		a.pseudoAngularVelocity -= uf::matrix::multiply( invIa, uf::vector::cross(rA, impulse) );
	}
	if ( !b.isStatic ) {
		b.pseudoVelocity += impulse * b.inverseMass;
		pod::Matrix3f invIb = impl::computeWorldInverseInertia( b );
		b.pseudoAngularVelocity += uf::matrix::multiply( invIb, uf::vector::cross(rB, impulse) );
	}
}


// accumulates then applies the change in impulse given an impulse direction and magnitude
void impl::applyImpulseTo(
	pod::PhysicsBody& a, pod::PhysicsBody& b,
	const pod::Vector3f& rA, const pod::Vector3f& rB,
	const pod::Vector3f& direction, float magnitude,
	float& accumulatedImpulse,
	float minLimit, float maxLimit
) {
	auto j = accumulateImpulseTo( magnitude, accumulatedImpulse, minLimit, maxLimit );
	impl::applyImpulseTo( a, b, rA, rB, direction * j );
}

// accumulates then applies the change in impulse given an impulse vector
void impl::applyImpulseTo(
	pod::PhysicsBody& a, pod::PhysicsBody& b,
	const pod::Vector3f& rA, const pod::Vector3f& rB,
	const pod::Vector3f& impulse, pod::Vector3f& accumulatedImpulse,
	float minLimit, float maxLimit
) {
	auto j = accumulateImpulseTo( impulse, accumulatedImpulse, minLimit, maxLimit );
	impl::applyImpulseTo( a, b, rA, rB, j );
}

void impl::applyPseudoImpulseTo(
	pod::PhysicsBody& a, pod::PhysicsBody& b,
	const pod::Vector3f& rA, const pod::Vector3f& rB,
	const pod::Vector3f& direction, float magnitude,
	float& accumulatedImpulse,
	float minLimit, float maxLimit
) {
	auto j = accumulateImpulseTo( magnitude, accumulatedImpulse, minLimit, maxLimit );
	impl::applyPseudoImpulseTo( a, b, rA, rB, direction * j );
}

void impl::applyPseudoImpulseTo(
	pod::PhysicsBody& a, pod::PhysicsBody& b,
	const pod::Vector3f& rA, const pod::Vector3f& rB,
	const pod::Vector3f& impulse, pod::Vector3f& accumulatedImpulse,
	float minLimit, float maxLimit
) {
	auto j = accumulateImpulseTo( impulse, accumulatedImpulse, minLimit, maxLimit );
	impl::applyPseudoImpulseTo( a, b, rA, rB, j );
}

// accumulates an scalar impulse
float impl::accumulateImpulseTo(
	float magnitude, float& accumulatedImpulse,
	float minLimit, float maxLimit
) {
	auto jOld = accumulatedImpulse;
	auto jNew = std::clamp( jOld + magnitude, minLimit, maxLimit );
	auto jDelta = jNew - jOld;
	accumulatedImpulse = jNew;

	return jDelta;
}
// accumulates an impulse vector
pod::Vector3f impl::accumulateImpulseTo(
	const pod::Vector3f& impulse, pod::Vector3f& accumulatedImpulse,
	float minLimit, float maxLimit
) {
	auto jOld = accumulatedImpulse;
	auto jNew = uf::vector::clamp( jOld + impulse, minLimit, maxLimit );
	auto jDelta = jNew - jOld;
	accumulatedImpulse = jNew;

	return jDelta;
}

void impl::applyRollingResistance( pod::PhysicsBody& body, float dt ) {
	if ( body.isStatic ) return;

	float rollingFriction = 0.02f; // to-do: derive from material
	float angularSpeed2 = uf::vector::magnitude( body.angularVelocity );
	if ( angularSpeed2 < EPS2 ) return;

	body.angularVelocity *= std::max(0.0f, 1.0f - rollingFriction * dt);
}

// snap velocity for grounded bodies
void impl::snapVelocity( pod::PhysicsBody& body, float dt, float threshold ) {
	if ( !body.activity.grounded || !body.activity.awake ) return;

	float threshold2 = threshold * threshold;
	// snap velocity if body is grounded and nearly still
	float linSpeed2 = uf::vector::magnitude( body.velocity );
	float angSpeed2 = uf::vector::magnitude( body.angularVelocity );

	// cancel out vertical component
	if ( fabs(body.velocity.y) < threshold2 ) body.velocity.y = 0.0f;
	// cancel out velocity entirely
	if ( linSpeed2 < threshold2 ) body.velocity = {};
	// cancel out rotational velocity entirely
	if ( angSpeed2 < threshold2 ) body.angularVelocity = {};
}

void impl::integrate( pod::PhysicsBody& body, float dt ) {
	// only integrate awake and dynamic bodies
	if ( !body.activity.awake || body.isStatic || body.mass == 0 ) return;

	auto& world = *body.world;

	// linear integration
	pod::Vector3f acceleration = body.forceAccumulator * body.inverseMass;
	acceleration += uf::physics::getGravity( body ); // apply gravity
	body.velocity += acceleration * dt;

	// angular integration
	{
		pod::Matrix3f R = uf::quaternion::matrix3(body.transform->orientation);
		pod::Vector3f localTorque = uf::matrix::multiply( uf::matrix::transpose(R), body.torqueAccumulator );
		pod::Vector3f localAngAccel = localTorque * body.inverseInertiaTensor; // element-wise
		body.angularVelocity += uf::matrix::multiply( R, localAngAccel ) * dt;
	}

	// update position
	body.transform/*.reference*/->position += body.velocity * dt;

	// update orientation
	float angularSpeed2 = uf::vector::magnitude( body.angularVelocity );
	if ( angularSpeed2 > EPS2 ) {
		float angularSpeed = std::sqrt( angularSpeed2 );
		pod::Quaternion<> dq = uf::quaternion::axisAngle( body.angularVelocity / angularSpeed, angularSpeed * dt);
		uf::transform::rotate( *body.transform/*.reference*/, dq );
	}

	// pseudo-impulse position correction
	if ( !uf::physics::settings.ngsPositionSolver ) {
		body.transform->position += body.pseudoVelocity * dt;

		float pseudoAngularSpeed2 = uf::vector::magnitude( body.pseudoAngularVelocity );
		if ( pseudoAngularSpeed2 > EPS ) {
			float pseudoAngularSpeed = std::sqrt( pseudoAngularSpeed2 );
			pod::Vector3f axis = body.pseudoAngularVelocity / pseudoAngularSpeed;

			float clampedSpeed = std::min(pseudoAngularSpeed, (2.0f * M_PI / 180.0f) / dt);
			pod::Quaternion<> dq = uf::quaternion::axisAngle( axis, clampedSpeed * dt );
			uf::transform::rotate( *body.transform, dq );
		}
		
		// reset
		body.pseudoAngularVelocity = {};
		body.pseudoVelocity = {};
	}

	// reset accumulators
	body.forceAccumulator = {};
	body.torqueAccumulator = {};

	// apply rolling resistance
	impl::applyRollingResistance( body, dt );

	// update activity state
	impl::updateActivity( body, dt );
}