#include <uf/utils/math/physics/impl.h>
#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/integration.h>
#include <uf/utils/math/physics/constraints.h>

void impl::solveHingeConstraint( pod::Constraint& constraint, float dt ) {
	auto& a = *constraint.a;
	auto& b = *constraint.b;
	auto& joint = constraint.hinge;
	auto& motor = constraint.motor;

	auto tA = impl::getTransform( a );
	auto tB = impl::getTransform( b );

	// solve linearly first
	impl::solveBallSocketConstraint( constraint, dt );

	auto ctxA = impl::solverBodyContext( a );
	auto ctxB = impl::solverBodyContext( b );

	auto waA = uf::quaternion::rotate( tA.orientation, joint.localAxisA );
	auto waB = uf::quaternion::rotate( tB.orientation, joint.localAxisB );
	auto angularError = uf::vector::cross( waA, waB );

	pod::Vector3f tangents[2];
	tangents[0] = impl::computeTangent( waA );
	tangents[1] = uf::vector::cross( waA, tangents[0] );

	auto relAngularVel = b.angularVelocity - a.angularVelocity;

	pod::Matrix2f K = {};
	for ( auto i = 0; i < 2; ++i ) {
		for ( auto j = 0; j < 2; ++j ) {
			K(i, j) = impl::computeAngularMassMatrixLine( ctxA, ctxB, tangents[i], tangents[j]);
		}
		K(i,i) += uf::physics::settings.jointCFM * ( 1.0f + ctxA.invM + ctxB.invM );
	}

	pod::Matrix2f Kinv = uf::matrix::inverse(K);
	pod::Vector2f rhs = {};
	FOR_EACH( 2, {
		rhs[i] = -(uf::vector::dot(relAngularVel, tangents[i]) + (uf::physics::settings.baumgarteCorrectionPercent / dt) * uf::vector::dot(angularError, tangents[i]));
	});

	pod::Vector2f impulse = uf::matrix::multiply(Kinv, rhs);

	for ( auto i = 0; i < 2; ++i ) {
		auto jDelta = impl::accumulateImpulseTo( impulse[i], joint.accumulatedAngularImpulse[i] );
		auto j = tangents[i] * jDelta;
		if ( !a.isStatic ) a.angularVelocity -= uf::matrix::multiply( ctxA.invI, j );
		if ( !b.isStatic ) b.angularVelocity += uf::matrix::multiply( ctxB.invI, j );
	}

	if ( motor.enabled ) {
		impl::solve1DAngularMotor( a, b, waA, motor.targetVelocity, motor.maxMotorTorque, motor.accumulatedMotorImpulse, dt );
	}
}

pod::Constraint& uf::physics::constrain( pod::World& world, pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Vector3f& joint, const pod::Vector3f& axis ) {
	auto& constraint = uf::physics::constrain( world, a, b );
	constraint.type = pod::ConstraintType::HINGE;
	// transform joint into local space 
	auto tA = impl::getTransform( a );
	auto tB = impl::getTransform( b );
	auto normAxis = uf::vector::normalize( axis );

	constraint.hinge.localAnchorA = uf::transform::applyInverse( tA, joint );
	constraint.hinge.localAnchorB = uf::transform::applyInverse( tB, joint );
	constraint.hinge.accumulatedImpulse = {};
	constraint.hinge.localAxisA = uf::quaternion::rotate( uf::quaternion::inverse(tA.orientation), normAxis );
	constraint.hinge.localAxisB = uf::quaternion::rotate( uf::quaternion::inverse(tB.orientation), normAxis );
	constraint.hinge.accumulatedAngularImpulse = {};

	return constraint;
}
pod::Constraint& uf::physics::constrain( pod::World& world, uf::Object& a, uf::Object& b, const pod::Vector3f& joint, const pod::Vector3f& axis ) {
	return constrain( world, a.getComponent<pod::PhysicsBody>(), b.getComponent<pod::PhysicsBody>(), joint, axis );
}
pod::Constraint& uf::physics::constrain( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Vector3f& joint, const pod::Vector3f& axis ) {
	return constrain( uf::physics::getWorld(), a, b, joint, axis );
}
pod::Constraint& uf::physics::constrain( uf::Object& a, uf::Object& b, const pod::Vector3f& joint, const pod::Vector3f& axis ) {
	return constrain( uf::physics::getWorld(), a.getComponent<pod::PhysicsBody>(), b.getComponent<pod::PhysicsBody>(), joint, axis );
}