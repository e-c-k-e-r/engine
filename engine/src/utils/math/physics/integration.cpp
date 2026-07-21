#include <uf/utils/math/physics/impl.h>
#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/integration.h>
#include <uf/utils/math/physics/narrowphase.h>

pod::SolverBodyContext impl::solverBodyContext( const pod::PhysicsBody& body ) {
	return { body.inverseMass == 0.0f ? 0.0f : body.inverseMass, impl::computeWorldInverseInertia( body ) };
}

float impl::computeEffectiveMass( const pod::SolverBodyContext& a, const pod::SolverBodyContext& b, const pod::JacobianRow& row ) {
	float inverseMass = a.invM + b.invM;
	float angularTerm = 0.0f;

	if ( a.invM > 0.0f ) {
		auto crossA = uf::vector::cross(row.rA, row.n);
		auto I_crossA = uf::matrix::multiply(a.invI, crossA);
		angularTerm += uf::vector::dot(uf::vector::cross(I_crossA, row.rA), row.n);
	}
	if ( b.invM > 0.0f ) {
		auto crossB = uf::vector::cross(row.rB, row.n);
		auto I_crossB = uf::matrix::multiply(b.invI, crossB);
		angularTerm += uf::vector::dot(uf::vector::cross(I_crossB, row.rB), row.n);
	}

	return inverseMass + angularTerm;
}
// to-do: convert below to just use the above
float impl::computeEffectiveMass( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Vector3f& rA, const pod::Vector3f& rB, const pod::Vector3f& n ) {
	auto ctxA = impl::solverBodyContext( a );
	auto ctxB = impl::solverBodyContext( b );
	auto row = pod::JacobianRow{ rA, rB, n };
	return impl::computeEffectiveMass( ctxA, ctxB, row );
}

float impl::computeMassMatrixLine( const pod::SolverBodyContext& a, const pod::SolverBodyContext& b, const pod::JacobianRow& rowI, const pod::JacobianRow& rowJ ) {
	float termLinear = (a.invM + b.invM) * uf::vector::dot(rowI.n, rowJ.n);
	float angularTerm = 0.0f;

	if ( a.invM > 0.0f ) {
		pod::Vector3f raXnj = uf::vector::cross(rowJ.rA, rowJ.n);
		pod::Vector3f Ia_raXnj = uf::matrix::multiply( a.invI, raXnj );
		pod::Vector3f crossA = uf::vector::cross(Ia_raXnj, rowI.rA);
		angularTerm += uf::vector::dot(rowI.n, crossA);
	}

	if ( b.invM > 0.0f ) {
		pod::Vector3f rbXnj = uf::vector::cross(rowJ.rB, rowJ.n);
		pod::Vector3f Ib_rbXnj = uf::matrix::multiply( b.invI, rbXnj );
		pod::Vector3f crossB = uf::vector::cross(Ib_rbXnj, rowI.rB);
		angularTerm += uf::vector::dot(rowI.n, crossB);
	}

	return termLinear + angularTerm;
}
float impl::computeAngularMassMatrixLine( const pod::SolverBodyContext& a, const pod::SolverBodyContext& b, const pod::Vector3f& n_i, const pod::Vector3f& n_j ) {
	float kVal = 0.0f;
	if ( a.invM > 0.0f ) kVal += uf::vector::dot(n_i, uf::matrix::multiply(a.invI, n_j));
	if ( b.invM > 0.0f ) kVal += uf::vector::dot(n_i, uf::matrix::multiply(b.invI, n_j));
	return kVal;
}

void impl::applyImpulseTo( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Vector3f& rA, const pod::Vector3f& rB, const pod::Vector3f& impulse ) {
	if ( a.inverseMass != 0.0f ) {
		a.velocity -= impulse * a.inverseMass;
		pod::Matrix3f invIa = impl::computeWorldInverseInertia( a );
		a.angularVelocity -= uf::matrix::multiply( invIa, uf::vector::cross(rA, impulse) );
	}
	if ( b.inverseMass != 0.0f ) {
		b.velocity += impulse * b.inverseMass;
		pod::Matrix3f invIb = impl::computeWorldInverseInertia( b );
		b.angularVelocity += uf::matrix::multiply( invIb, uf::vector::cross(rB, impulse) );
	}
}
void impl::applyPseudoImpulseTo( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Vector3f& rA, const pod::Vector3f& rB, const pod::Vector3f& impulse ) {
	if ( a.inverseMass != 0.0f ) {
		a.pseudoVelocity -= impulse * a.inverseMass;
		pod::Matrix3f invIa = impl::computeWorldInverseInertia( a );
		a.pseudoAngularVelocity -= uf::matrix::multiply( invIa, uf::vector::cross(rA, impulse) );
	}
	if ( b.inverseMass != 0.0f ) {
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
	if ( body.inverseMass == 0.0f ) return;

	float rollingFriction = 0.02f; // to-do: derive from material
	float angularSpeed2 = uf::vector::magnitude( body.angularVelocity );
	if ( angularSpeed2 < EPS2 ) return;

	body.angularVelocity *= std::max(0.0f, 1.0f - rollingFriction * dt);
}

// snap velocity for grounded bodies
void impl::snapVelocity( pod::PhysicsBody& body, float dt, float threshold ) {
	if ( !body.activity.grounded || !body.activity.awake || body.inverseMass == 0.0f ) return;

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

void impl::integrateKinematic( pod::PhysicsBody& body, float dt ) {
	if ( !body.activity.awake || body.inverseMass != 0.0f ) return;

	auto& transform = *body.transform;

	transform.position += body.velocity * dt;

	float angularSpeed2 = uf::vector::magnitude( body.angularVelocity );
	if ( angularSpeed2 > EPS2 ) {
		float angularSpeed = std::sqrt( angularSpeed2 );
		pod::Quaternion<> dq = uf::quaternion::axisAngle( body.angularVelocity / angularSpeed, angularSpeed * dt );
		uf::transform::rotate( transform, dq );
	}
}

void impl::integrate( pod::PhysicsBody& body, float dt ) {
	// only integrate awake and dynamic bodies
	if ( !body.activity.awake || body.inverseMass == 0.0f ) return;

	auto& world = *body.world;
	auto& transform = *body.transform;
	auto fT = uf::transform::flatten( transform );
	auto gravity = uf::physics::getGravity( body );

	if ( body.activity.referenceFrame ) {
		auto ref = body.activity.referenceFrame->velocity;
		if ( body.velocity.y > ref.y + 1.0f ) {
        	body.activity.referenceFrame = NULL;
    	} else if ( body.velocity.y > ref.y ) body.velocity.y = ref.y;
	}

	// linear integration
	pod::Vector3f acceleration = (body.forceAccumulator * body.inverseMass);
	acceleration += gravity; // apply gravity

	acceleration = acceleration * body.linearFactor;
	body.velocity += acceleration * dt;
	body.velocity = body.velocity * body.linearFactor;

	// angular integration
	{
		pod::Matrix3f R = uf::quaternion::matrix3( fT.orientation );
		pod::Vector3f localTorque = uf::matrix::multiply( uf::matrix::transpose(R), body.torqueAccumulator );

		pod::Vector3f localAngAccel = (localTorque * body.inverseInertiaTensor) * body.angularFactor;
		body.angularVelocity += uf::matrix::multiply( R, localAngAccel ) * dt;
		body.angularVelocity = body.angularVelocity * body.angularFactor;
	}

	// update position
	transform.position += body.velocity * dt;

	// update orientation
	float angularSpeed2 = uf::vector::magnitude( body.angularVelocity );
	if ( angularSpeed2 > EPS2 ) {
		float angularSpeed = std::sqrt( angularSpeed2 );
		pod::Quaternion<> dq = uf::quaternion::axisAngle( body.angularVelocity / angularSpeed, angularSpeed * dt);
		uf::transform::rotate( transform, dq );
	}

	// pseudo-impulse position correction
	if ( !uf::physics::settings.ngsPositionSolver ) {
		transform.position += body.pseudoVelocity * dt;

		float pseudoAngularSpeed2 = uf::vector::magnitude( body.pseudoAngularVelocity );
		if ( pseudoAngularSpeed2 > EPS ) {
			float pseudoAngularSpeed = std::sqrt( pseudoAngularSpeed2 );
			pod::Vector3f axis = body.pseudoAngularVelocity / pseudoAngularSpeed;

			float clampedSpeed = std::min(pseudoAngularSpeed, (2.0f * M_PI / 180.0f) / dt);
			pod::Quaternion<> dq = uf::quaternion::axisAngle( axis, clampedSpeed * dt );
			uf::transform::rotate( transform, dq );
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