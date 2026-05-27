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
		if ( a.inverseMass != 0.0f ) a.angularVelocity -= uf::matrix::multiply( ctxA.invI, j );
		if ( b.inverseMass != 0.0f ) b.angularVelocity += uf::matrix::multiply( ctxB.invI, j );
	}

	if ( motor.enabled ) {
		impl::solve1DAngularMotor( a, b, waA, motor.targetVelocity, motor.maxMotorTorque, motor.accumulatedMotorImpulse, dt );
	}
}

pod::Constraint& uf::physics::constrainHinge( pod::Constraint& constraint, const pod::Vector3f& p, const pod::Vector3f& a ) {
	auto tA = impl::getTransform( *constraint.a );
	auto tB = impl::getTransform( *constraint.b );
	auto axis = uf::vector::normalize( a );

	auto& joint = constraint.hinge;
	constraint.type = pod::ConstraintType::HINGE;
	joint.localAnchorA = uf::transform::applyInverse( tA, p );
	joint.localAnchorB = uf::transform::applyInverse( tB, p );
	joint.accumulatedImpulse = {};
	joint.localAxisA = uf::quaternion::rotate( uf::quaternion::inverse( tA.orientation ), axis );
	joint.localAxisB = uf::quaternion::rotate( uf::quaternion::inverse( tB.orientation ), axis );
	joint.accumulatedAngularImpulse = {};

	return constraint;
}

void impl::drawHinge( const pod::Constraint& constraint ) {
	if ( !constraint.a || !constraint.b ) return;

	auto tA = impl::getTransform(*constraint.a);
	auto tB = impl::getTransform(*constraint.b);

	const auto& joint = constraint.hinge;

	auto pA = tA.position + uf::quaternion::rotate(tA.orientation, joint.localAnchorA);
	auto pB = tB.position + uf::quaternion::rotate(tB.orientation, joint.localAnchorB);

	auto aA = uf::quaternion::rotate(tA.orientation, joint.localAxisA);
	auto aB = uf::quaternion::rotate(tB.orientation, joint.localAxisB);

	uf::debug::drawLine( pA, pB );

	float pin = 0.5f;
	uf::debug::drawLine( pA - pod::Vector3f{aA.x * pin, aA.y * pin, aA.z * pin}, pA + pod::Vector3f{aA.x * pin, aA.y * pin, aA.z * pin} );
	uf::debug::drawLine( pB - pod::Vector3f{aB.x * pin, aB.y * pin, aB.z * pin}, pB + pod::Vector3f{aB.x * pin, aB.y * pin, aB.z * pin} );
}