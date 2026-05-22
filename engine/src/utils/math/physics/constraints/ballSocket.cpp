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

	auto rA = uf::quaternion::rotate( tA.orientation, joint.localAnchorA );
	auto rB = uf::quaternion::rotate( tB.orientation, joint.localAnchorB );

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

pod::Constraint& uf::physics::constrain( pod::World& world, pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Vector3f& joint ) {
	auto& constraint = uf::physics::constrain( world, a, b );
	constraint.type = pod::ConstraintType::BALL_AND_SOCKET;
	// transform joint into local space 
	auto tA = impl::getTransform( a );
	auto tB = impl::getTransform( b );

	constraint.ballSocket.localAnchorA = uf::transform::applyInverse( tA, joint );
	constraint.ballSocket.localAnchorB = uf::transform::applyInverse( tB, joint );
	constraint.ballSocket.accumulatedImpulse = {};

	return constraint;
}
pod::Constraint& uf::physics::constrain( pod::World& world, uf::Object& a, uf::Object& b, const pod::Vector3f& joint ) {
	return constrain( world, a.getComponent<pod::PhysicsBody>(), b.getComponent<pod::PhysicsBody>(), joint );
}
pod::Constraint& uf::physics::constrain( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Vector3f& joint ) {
	return constrain( uf::physics::getWorld(), a, b, joint );
}
pod::Constraint& uf::physics::constrain( uf::Object& a, uf::Object& b, const pod::Vector3f& joint ) {
	return constrain( uf::physics::getWorld(), a.getComponent<pod::PhysicsBody>(), b.getComponent<pod::PhysicsBody>(), joint );
}