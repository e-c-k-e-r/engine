#include <uf/utils/math/physics/impl.h>
#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/integration.h>
#include <uf/utils/math/physics/constraints.h>

void impl::solveWeldConstraint( pod::Constraint& constraint, float dt ) {
	auto& a = *constraint.a;
	auto& b = *constraint.b;
	auto& joint = constraint.weld;

	// solve linearly
	impl::solveBallSocketConstraint( constraint, dt );

	// solve angularly
	auto tA = impl::getTransform( a );
	auto tB = impl::getTransform( b );

	auto ctxA = impl::solverBodyContext( a );
	auto ctxB = impl::solverBodyContext( b );

	auto waA = uf::quaternion::rotate( tA.orientation, joint.localAxisA );
	auto waB = uf::quaternion::rotate( tB.orientation, joint.localAxisB );

	auto wrA = uf::quaternion::rotate( tA.orientation, joint.localReferenceAxisA );
	auto wrB = uf::quaternion::rotate( tB.orientation, joint.localReferenceAxisB );

	auto relAngularVel = b.angularVelocity - a.angularVelocity;
	auto angularError = uf::vector::cross(waA, waB) + uf::vector::cross(wrA, wrB);

	pod::Matrix3f K = {};
	pod::Vector3f axes[3] = { {1,0,0}, {0,1,0}, {0,0,1} };
	for ( auto i = 0; i < 3; ++i ) {
		for ( auto j = 0; j < 3; ++j ) {
			K(i, j) = impl::computeAngularMassMatrixLine( ctxA, ctxB, axes[i], axes[j]);
		}
		K(i,i) += uf::physics::settings.jointCFM * ( 1.0f + ctxA.invM + ctxB.invM );
	}

	auto bias = angularError * (uf::physics::settings.baumgarteCorrectionPercent / dt);
	auto rhs = -(relAngularVel + bias);
	auto impulse = uf::matrix::multiply( uf::matrix::inverse(K), rhs );
	
	joint.accumulatedAngularImpulse += impulse;

	if ( !a.isStatic ) a.angularVelocity -= uf::matrix::multiply( ctxA.invI, impulse );
	if ( !b.isStatic ) b.angularVelocity += uf::matrix::multiply( ctxB.invI, impulse );
}