#include <uf/utils/math/physics/impl.h>
#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/integration.h>
#include <uf/utils/math/physics/constraints.h>

void impl::solveConeTwistConstraint( pod::Constraint& constraint, float dt ) {
	auto& joint = constraint.coneTwist;
	auto& a = *constraint.a;
	auto& b = *constraint.b;

	// solve linearly
	impl::solveBallSocketConstraint( constraint, dt );
	
	auto ctxA = impl::solverBodyContext( a );
	auto ctxB = impl::solverBodyContext( b );

	auto tA = impl::getTransform( a );
	auto tB = impl::getTransform( b );

	auto taA = uf::quaternion::rotate( tA.orientation, joint.localTwistAxisA );
	auto taB = uf::quaternion::rotate( tB.orientation, joint.localTwistAxisB );

	auto raA = uf::quaternion::rotate( tA.orientation, joint.localReferenceAxisA );
	auto raB = uf::quaternion::rotate( tB.orientation, joint.localReferenceAxisB );

	auto relAngularVel = b.angularVelocity - a.angularVelocity;

	// swing
	auto swingAxis = uf::vector::cross( taA, taB );
	float swingLength2 = uf::vector::dot( swingAxis, swingAxis );
	float swingAngle = 0.0f;
	float swingError = 0.0f;
	bool swingActive = false;

	if ( swingLength2 > EPS2 ) {
		swingAxis = swingAxis / std::sqrt( swingLength2 );
		float dot = uf::vector::dot( taA, taB );
		swingAngle = std::atan2( std::sqrt(swingLength2), dot );

		if ( swingAngle > joint.swingLimit ) {
			swingError = swingAngle - joint.swingLimit;
			swingActive = true;
		} else {
			joint.accumulatedAngularImpulse.x = 0.0f;
		}
	}

	// twist
	pod::Vector3f twistAxis = taA;
	float proj = uf::vector::dot( raB, twistAxis );
	pod::Vector3f refB_projected = raB - (twistAxis * proj);

	float refBLength2 = uf::vector::dot(refB_projected, refB_projected);
	float twistError = 0.0f;
	bool twistActive = false;

	if ( refBLength2 > EPS2 ) {
		refB_projected = refB_projected / std::sqrt(refBLength2);
		pod::Vector3f crossRef = uf::vector::cross( raA, refB_projected );
		float sinTheta = uf::vector::dot( crossRef, twistAxis );
		float cosTheta = uf::vector::dot( raA, refB_projected );
		float twistAngle = std::atan2( sinTheta, cosTheta );

		if ( twistAngle > joint.twistLimit ) {
			twistError = twistAngle - joint.twistLimit;
			twistActive = true;
		} else if ( twistAngle < -joint.twistLimit ) {
			twistError = twistAngle + joint.twistLimit;
			twistActive = true;
		} else {
			joint.accumulatedAngularImpulse.y = 0.0f;
		}
	}

	// block solve
	if ( swingActive && twistActive ) {
		pod::Vector3f axes[2] = { swingAxis, twistAxis };
		pod::Matrix2f K = {};
		for ( auto i = 0; i < 2; ++i ) {
			for ( auto j = 0; j < 2; ++j ) {
				K(i, j) = impl::computeAngularMassMatrixLine( ctxA, ctxB, axes[i], axes[j]);
			}
			K(i,i) += uf::physics::settings.jointCFM * ( 1.0f + ctxA.invM + ctxB.invM );
		}

		pod::Matrix2f Kinv = uf::matrix::inverse( K );
		pod::Vector2f rhs = {
			-(uf::vector::dot(relAngularVel, swingAxis) + ((uf::physics::settings.baumgarteCorrectionPercent / dt) * swingError)),
			-(uf::vector::dot(relAngularVel, twistAxis) + ((uf::physics::settings.baumgarteCorrectionPercent / dt) * twistError)),
		};
		pod::Vector2f impulse = uf::matrix::multiply( Kinv, rhs );

		float unconstrainedSwing = joint.accumulatedAngularImpulse.x + impulse[0];
		float unconstrainedTwist = joint.accumulatedAngularImpulse.y + impulse[1];

		bool swingValid = ( unconstrainedSwing <= 0.0f );
		bool twistValid = ( twistError > 0.0f ) ? ( unconstrainedTwist <= 0.0f ) : ( unconstrainedTwist >= 0.0f );

		if ( swingValid && twistValid ) {
			float swingDelta = unconstrainedSwing - joint.accumulatedAngularImpulse.x;
			float twistDelta = unconstrainedTwist - joint.accumulatedAngularImpulse.y;

			joint.accumulatedAngularImpulse.x = unconstrainedSwing;
			joint.accumulatedAngularImpulse.y = unconstrainedTwist;

			auto impulseVector = (swingAxis * swingDelta) + (twistAxis * twistDelta);
			if ( !a.isStatic ) a.angularVelocity -= uf::matrix::multiply( ctxA.invI, impulseVector );
			if ( !b.isStatic ) b.angularVelocity += uf::matrix::multiply( ctxB.invI, impulseVector );

			return;
		}
	}

	// swing fallback: solve 1D
	if ( swingActive ) {
		float invMassN = impl::computeAngularMassMatrixLine( ctxA, ctxB, swingAxis, swingAxis );
		UF_ASSERT( invMassN > EPS );

		float bias = (uf::physics::settings.baumgarteCorrectionPercent / dt) * swingError;
		float j = -(uf::vector::dot(relAngularVel, swingAxis) + bias) / invMassN;
		auto impulse = swingAxis * impl::accumulateImpulseTo( j, joint.accumulatedAngularImpulse.x, -FLT_MAX, 0.0f );
		if ( !a.isStatic ) a.angularVelocity -= uf::matrix::multiply( ctxA.invI, impulse );
		if ( !b.isStatic ) b.angularVelocity += uf::matrix::multiply( ctxB.invI, impulse );
	}

	// twist fallback: solve 1D
	if ( twistActive ) {
		float invMassN = impl::computeAngularMassMatrixLine( ctxA, ctxB, twistAxis, twistAxis );
		UF_ASSERT( invMassN > EPS );

		float bias = (uf::physics::settings.baumgarteCorrectionPercent / dt) * twistError;
		float j = -(uf::vector::dot(relAngularVel, twistAxis) + bias) / invMassN;
		float min = (twistError > 0.0f) ? -FLT_MAX : 0.0f;
		float max = (twistError > 0.0f) ? 0.0f : FLT_MAX;
		auto impulse = twistAxis * impl::accumulateImpulseTo( j, joint.accumulatedAngularImpulse.y, min, max );
		if ( !a.isStatic ) a.angularVelocity -= uf::matrix::multiply( ctxA.invI, impulse );
		if ( !b.isStatic ) b.angularVelocity += uf::matrix::multiply( ctxB.invI, impulse );
	}
}

pod::Constraint& uf::physics::constrain( pod::World& world, pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Vector3f& joint, const pod::Vector3f& axis, float swingLimit, float twistLimit ) {
	auto& constraint = uf::physics::constrain( world, a, b );
	constraint.type = pod::ConstraintType::CONE_TWIST;

	// transform joint into local space
	auto tA = impl::getTransform( a );
	auto tB = impl::getTransform( b );

	auto normAxis = uf::vector::normalize( axis );
	auto worldRefAxis = uf::vector::normalize( impl::computeTangent( normAxis ) );

	auto invOriA = uf::quaternion::inverse( tA.orientation );
	auto invOriB = uf::quaternion::inverse( tB.orientation );

	constraint.coneTwist.localAnchorA = uf::transform::applyInverse( tA, joint );
	constraint.coneTwist.localAnchorB = uf::transform::applyInverse( tB, joint );
	
	constraint.coneTwist.accumulatedImpulse = {};
	constraint.coneTwist.accumulatedAngularImpulse = {};

	constraint.coneTwist.localTwistAxisA = uf::quaternion::rotate( invOriA, normAxis );
	constraint.coneTwist.localTwistAxisB = uf::quaternion::rotate( invOriB, normAxis );

	constraint.coneTwist.localReferenceAxisA = uf::quaternion::rotate( invOriA, worldRefAxis );
	constraint.coneTwist.localReferenceAxisB = uf::quaternion::rotate( invOriB, worldRefAxis );

	constraint.coneTwist.swingLimit = swingLimit;
	constraint.coneTwist.twistLimit = twistLimit;

	return constraint;
}
pod::Constraint& uf::physics::constrain( pod::World& world, uf::Object& a, uf::Object& b, const pod::Vector3f& joint, const pod::Vector3f& axis, float swingLimit, float twistLimit ) {
	return constrain( world, a.getComponent<pod::PhysicsBody>(), b.getComponent<pod::PhysicsBody>(), joint, axis, swingLimit, twistLimit );
}
pod::Constraint& uf::physics::constrain( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Vector3f& joint, const pod::Vector3f& axis, float swingLimit, float twistLimit ) {
	return constrain( uf::physics::getWorld(), a, b, joint, axis, swingLimit, twistLimit );
}
pod::Constraint& uf::physics::constrain( uf::Object& a, uf::Object& b, const pod::Vector3f& joint, const pod::Vector3f& axis, float swingLimit, float twistLimit ) {
	return constrain( uf::physics::getWorld(), a.getComponent<pod::PhysicsBody>(), b.getComponent<pod::PhysicsBody>(), joint, axis, swingLimit, twistLimit );
}