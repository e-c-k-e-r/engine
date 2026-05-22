#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/integration.h>
#include <uf/utils/math/physics/constraints.h>

void impl::solveBallSocketConstraint( pod::Constraint& constraint, float dt ) {
	auto& a = *constraint.a;
	auto& b = *constraint.b;
	auto& joint = constraint.ballSocket;

	auto tA = impl::getTransform( a );
	auto tB = impl::getTransform( b );

	auto rA = uf::quaternion::rotate( tA.orientation, joint.localAnchorA );
	auto rB = uf::quaternion::rotate( tB.orientation, joint.localAnchorB );

	auto worldAnchorA = tA.position + rA;
	auto worldAnchorB = tB.position + rB;

	auto positionError = worldAnchorB - worldAnchorA;

	auto vA = a.velocity + uf::vector::cross(a.angularVelocity, rA);
	auto vB = b.velocity + uf::vector::cross(b.angularVelocity, rB);
	auto relativeVelocity = vB - vA;

	float invMassA = a.isStatic ? 0.0f : a.inverseMass;
	float invMassB = b.isStatic ? 0.0f : b.inverseMass;
	float sumInvMass = invMassA + invMassB;

	pod::Matrix3f invIa = impl::computeWorldInverseInertia( a );
	pod::Matrix3f invIb = impl::computeWorldInverseInertia( b );

	// effective mass matrix
	pod::Matrix3f K = {};
	pod::Vector3f axes[3] = { {1,0,0}, {0,1,0}, {0,0,1} };

	for ( auto i = 0; i < 3; ++i ) {
		for ( auto j = 0; j < 3; ++j ) {
			float termLinear = (i == j) ? sumInvMass : 0.0f;

			pod::Vector3f raXnj = uf::vector::cross(rA, axes[j]);
			pod::Vector3f rbXnj = uf::vector::cross(rB, axes[j]);

			pod::Vector3f Ia_raXnj = uf::matrix::multiply(invIa, raXnj);
			pod::Vector3f Ib_rbXnj = uf::matrix::multiply(invIb, rbXnj);

			pod::Vector3f crossA = uf::vector::cross(Ia_raXnj, rA);
			pod::Vector3f crossB = uf::vector::cross(Ib_rbXnj, rB);

			float termAngular = uf::vector::dot(axes[i], crossA + crossB);

			K(i, j) = termLinear + termAngular;
		}
	}

	// impulse matrix
	pod::Matrix3f Kinv = uf::matrix::inverse( K );
	// bias
	pod::Vector3f bias = positionError * (uf::physics::settings.baumgarteCorrectionPercent / dt);
	pod::Vector3f rhs = -(relativeVelocity + bias);
	// solve and apply
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