#include <uf/utils/math/physics/impl.h>
#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/integration.h>
#include <uf/utils/math/physics/constraints.h>

void impl::solveSliderConstraint( pod::Constraint& constraint, float dt ) {
	auto& a = *constraint.a;
	auto& b = *constraint.b;
	auto& joint = constraint.slider;
	auto& motor = constraint.motor;

	auto tA = impl::getTransform( a );
	auto tB = impl::getTransform( b );

	auto rA = uf::quaternion::rotate( tA.orientation, joint.localAnchorA );
	auto rB = uf::quaternion::rotate( tB.orientation, joint.localAnchorB );

	auto pA = tA.position + rA;
	auto pB = tB.position + rB;

	auto waA = uf::quaternion::rotate( tA.orientation, joint.localAxisA );
	auto waB = uf::quaternion::rotate( tB.orientation, joint.localAxisB );
	
	auto wrA = uf::quaternion::rotate( tA.orientation, joint.localReferenceAxisA );
	auto wrB = uf::quaternion::rotate( tB.orientation, joint.localReferenceAxisB );

	auto ctxA = impl::solverBodyContext( a );
	auto ctxB = impl::solverBodyContext( b );

	auto relAngularVel = b.angularVelocity - a.angularVelocity;

	auto vA = a.velocity + uf::vector::cross(a.angularVelocity, rA);
	auto vB = b.velocity + uf::vector::cross(b.angularVelocity, rB);
	
	auto relVel = vB - vA;
	auto angularError = uf::vector::cross(waA, waB) + uf::vector::cross(wrA, wrB);
	auto positionError = pB - pA;

	{
		pod::Matrix3f K = {};
		pod::Vector3f axes[3] = { {1,0,0}, {0,1,0}, {0,0,1} };
		for ( auto i = 0; i < 3; ++i ) {
			for ( auto j = 0; j < 3; ++j ) {
				K(i, j) = impl::computeAngularMassMatrixLine( ctxA, ctxB, axes[i], axes[j]);
			}
			K(i,i) += uf::physics::settings.jointCFM * ( 1.0f + ctxA.invM + ctxB.invM );
		}

		pod::Matrix3f Kinv = uf::matrix::inverse( K );
		pod::Vector3f bias = angularError * (uf::physics::settings.baumgarteCorrectionPercent / dt);
		pod::Vector3f rhs = -(relAngularVel + bias);

		pod::Vector3f impulse = uf::matrix::multiply( Kinv, rhs );
		joint.accumulatedAngularImpulse += impulse;

		if ( a.inverseMass != 0.0f ) a.angularVelocity -= uf::matrix::multiply( ctxA.invI, impulse );
		if ( b.inverseMass != 0.0f ) b.angularVelocity += uf::matrix::multiply( ctxB.invI, impulse );
	}
	{
		pod::Vector3f t1 = wrA;
		pod::Vector3f t2 = uf::vector::cross( waA, wrA );

		pod::Vector3f axes[2] = { t1, t2 };
		pod::Matrix2f K = {};
		for ( auto i = 0; i < 2; ++i ) {
			auto rowI = pod::JacobianRow{ rA, rB, axes[i] };
			for ( auto j = 0; j < 2; ++j ) {
				auto rowJ = pod::JacobianRow{ rA, rB, axes[j] };
				K(i, j) = impl::computeMassMatrixLine( ctxA, ctxB, rowI, rowJ );
			}
		}

		pod::Matrix2f Kinv = uf::matrix::inverse( K );
		pod::Vector2f rhs = {
			-(uf::vector::dot(relVel, t1) + (uf::vector::dot(positionError, t1) * (uf::physics::settings.baumgarteCorrectionPercent / dt))),
			-(uf::vector::dot(relVel, t2) + (uf::vector::dot(positionError, t2) * (uf::physics::settings.baumgarteCorrectionPercent / dt)))
		};

		pod::Vector2f impulse = uf::matrix::multiply( Kinv, rhs );
		joint.accumulatedLinearImpulse += impulse;

		for( int i = 0; i < 2; i++ ) {
			impl::applyImpulseTo( a, b, rA, rB, axes[i], impulse[i], joint.accumulatedLinearImpulse[i] );
		}
	}

	if ( motor.enabled ) {
		impl::solve1DLinearMotor( a, b, rA, rB, waA, motor.targetVelocity, motor.maxMotorForce, motor.accumulatedMotorImpulse, dt );
	}

	float currentDist = uf::vector::dot( positionError, waA );
	if ( currentDist < joint.lowerLimit || currentDist > joint.upperLimit ) {
		float invMassN = impl::computeEffectiveMass( a, b, rA, rB, waA );
		UF_ASSERT( invMassN > EPS );

		float limitError = (currentDist < joint.lowerLimit) ? (currentDist - joint.lowerLimit) : (currentDist - joint.upperLimit);
		float bias = (uf::physics::settings.baumgarteCorrectionPercent / dt) * limitError;
		float j = -(uf::vector::dot(relVel, waA) + bias) / invMassN;

		float min = ( currentDist < joint.lowerLimit ) ? 0.0f : -FLT_MAX;
		float max = ( currentDist < joint.lowerLimit ) ? FLT_MAX : 0.0f;
		impl::applyImpulseTo( a, b, rA, rB, waA, j, joint.accumulatedLimitImpulse, min, max );
	}
}

pod::Constraint& uf::physics::constrainSlider( pod::Constraint& constraint, const pod::Vector3f& p, const pod::Vector3f& a, float lowerLimit, float upperLimit ) {
	auto tA = impl::getTransform( *constraint.a );
	auto tB = impl::getTransform( *constraint.b );

	auto axis = uf::vector::normalize( a);
	auto tangent = uf::vector::normalize( impl::computeTangent(axis) );

	auto invqA = uf::quaternion::inverse( tA.orientation );
	auto invqB = uf::quaternion::inverse( tB.orientation );

	auto& joint = constraint.slider;
	constraint.type = pod::ConstraintType::SLIDER;
	joint.localAnchorA = uf::transform::applyInverse( tA, p );
	joint.localAnchorB = uf::transform::applyInverse( tB, p );

	joint.localAxisA = uf::quaternion::rotate( invqA, axis );
	joint.localAxisB = uf::quaternion::rotate( invqB, axis );

	joint.localReferenceAxisA = uf::quaternion::rotate( invqA, tangent );
	joint.localReferenceAxisB = uf::quaternion::rotate( invqB, tangent );

	joint.lowerLimit = lowerLimit;
	joint.upperLimit = upperLimit;

	joint.accumulatedLinearImpulse = {};
	joint.accumulatedAngularImpulse = {};
	joint.accumulatedLimitImpulse = 0.0f;

	return constraint;
}

void impl::drawSlider( const pod::Constraint& constraint ) {
	if ( !constraint.a || !constraint.b ) return;

}