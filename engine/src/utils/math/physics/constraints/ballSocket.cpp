#include <uf/utils/math/physics/impl.h>
#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/integration.h>
#include <uf/utils/math/physics/constraints.h>

void impl::solveBallSocketConstraint( pod::Constraint& constraint, float dt ) {
	auto& joint = constraint.ballSocket;
	auto& a = *constraint.a;
	auto& b = *constraint.b;

	auto ctxA = impl::solverBodyContext( a );
	auto ctxB = impl::solverBodyContext( b );

	auto tA = impl::getTransform( a );
	auto tB = impl::getTransform( b );

	auto anchorA = joint.localAnchorA * tA.scale;
	auto anchorB = joint.localAnchorB * tB.scale;

	auto rA = uf::quaternion::rotate( tA.orientation, anchorA );
	auto rB = uf::quaternion::rotate( tB.orientation, anchorB );

	pod::Matrix3f K = {};
	pod::Vector3f axes[3] = { {1,0,0}, {0,1,0}, {0,0,1} };
	for ( auto i = 0; i < 3; ++i ) {
		auto rowI = pod::JacobianRow{ rA, rB, axes[i] };
		for ( auto j = 0; j < 3; ++j ) {
			auto rowJ = pod::JacobianRow{ rA, rB, axes[j] };
			K(i, j) = impl::computeMassMatrixLine( ctxA, ctxB, rowI, rowJ );
		}

		K(i,i) += uf::physics::settings.jointCFM * ( 1.0f + ctxA.invM + ctxB.invM );
	}

	auto pA = tA.position + rA;
	auto pB = tB.position + rB;

	auto vA = a.velocity + uf::vector::cross(a.angularVelocity, rA);
	auto vB = b.velocity + uf::vector::cross(b.angularVelocity, rB);
	
	auto relativeVelocity = vB - vA;
	auto positionError = pB - pA;

	pod::Matrix3f Kinv = uf::matrix::inverse( K );
	pod::Vector3f bias = positionError * (uf::physics::settings.baumgarteCorrectionPercent / dt);
	pod::Vector3f rhs = -(relativeVelocity + bias);

	pod::Vector3f impulse = uf::matrix::multiply( Kinv, rhs );
	impl::applyImpulseTo( a, b, rA, rB, impulse, joint.accumulatedImpulse );
}

pod::Constraint& uf::physics::constrainBallSocket( pod::Constraint& constraint, const pod::Vector3f& p ) {
	auto& joint = constraint.ballSocket;

	auto tA = impl::getTransform( *constraint.a );
	auto tB = impl::getTransform( *constraint.b );

	constraint.type = pod::ConstraintType::BALL_AND_SOCKET;
	joint.localAnchorA = uf::transform::applyInverse( tA, p );
	joint.localAnchorB = uf::transform::applyInverse( tB, p );
	joint.accumulatedImpulse = {};

	return constraint;
}

void impl::drawBallSocket( const pod::Constraint& constraint ) {
	if ( !constraint.a || !constraint.b ) return;

	const auto& joint = constraint.ballSocket;
	auto tA = impl::getTransform( *constraint.a );
	auto tB = impl::getTransform( *constraint.b );

	auto pA = uf::transform::apply( tA, joint.localAnchorA );
	auto pB = uf::transform::apply( tB, joint.localAnchorB );

	uf::debug::drawLine( tA.position, pA );
	uf::debug::drawLine( tB.position, pB );

	uf::debug::drawLine( pA, pB );

	// crosshair
	float size = 0.1f;
	uf::debug::drawLine( pA - pod::Vector3f{size, 0.0f, 0.0f}, pA + pod::Vector3f{size, 0.0f, 0.0f} );
	uf::debug::drawLine( pA - pod::Vector3f{0.0f, size, 0.0f}, pA + pod::Vector3f{0.0f, size, 0.0f} );
	uf::debug::drawLine( pA - pod::Vector3f{0.0f, 0.0f, size}, pA + pod::Vector3f{0.0f, 0.0f, size} );
}