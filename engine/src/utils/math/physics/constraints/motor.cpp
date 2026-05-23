#include <uf/utils/math/physics/impl.h>
#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/integration.h>
#include <uf/utils/math/physics/constraints.h>

void impl::solve1DLinearMotor(
	pod::PhysicsBody& a, pod::PhysicsBody& b,
	const pod::Vector3f& rA, const pod::Vector3f& rB, const pod::Vector3f& axis,
	float targetVelocity, float maxForce, float& accumulatedImpulse, float dt
) {
	auto vA = (a.velocity + uf::vector::cross(a.angularVelocity, rA));
	auto vB = (b.velocity + uf::vector::cross(b.angularVelocity, rB));
	auto relVel = vB - vA;
	float currentSpeed = uf::vector::dot(relVel, axis);
	float invMassN = impl::computeEffectiveMass( a, b, rA, rB, axis );

	UF_ASSERT( invMassN > EPS );

	float j = (targetVelocity - currentSpeed) / invMassN;
	float maxImpulse = maxForce * dt;
	impl::applyImpulseTo(a, b, rA, rB, axis, j, accumulatedImpulse, -maxImpulse, maxImpulse);
}

void impl::solve1DAngularMotor(
	pod::PhysicsBody& a, pod::PhysicsBody& b,
	const pod::Vector3f& axis,
	float targetVelocity, float maxTorque,
	float& accumulatedImpulse, float dt
) {
	auto relAngularVel = b.angularVelocity - a.angularVelocity;
	float currentSpeed = uf::vector::dot(relAngularVel, axis);
	
	auto ctxA = impl::solverBodyContext( a );
	auto ctxB = impl::solverBodyContext( b );
	float invMassN = impl::computeAngularMassMatrixLine( ctxA, ctxB, axis ,axis );
	UF_ASSERT( invMassN > EPS );

	float j = (targetVelocity - currentSpeed) / invMassN;
	float maxImpulse = maxTorque * dt;
	auto jDelta = impl::accumulateImpulseTo( j, accumulatedImpulse, -maxImpulse, maxImpulse  );

	pod::Vector3f impulse = axis * jDelta;
	if ( a.inverseMass != 0.0f ) a.angularVelocity -= uf::matrix::multiply( ctxA.invI, impulse );
	if ( b.inverseMass != 0.0f ) b.angularVelocity += uf::matrix::multiply( ctxB.invI, impulse );
}

pod::Constraint& uf::physics::constrainMotor( pod::Constraint& constraint, float targetVelocity, float maxForceOrTorque ) {
	auto& motor = constraint.motor;

	motor.enabled = true;
	motor.targetVelocity = targetVelocity;
	motor.maxMotorTorque = maxForceOrTorque;
	motor.accumulatedMotorImpulse = 0.0f;

	return constraint;
}