#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/constraints.h>

void impl::solveConstraint( pod::Constraint& constraint, float dt ) {
	switch ( constraint.type ) {
		case pod::ConstraintType::BALL_AND_SOCKET: {
			return impl::solveBallSocketConstraint( constraint, dt );
		} break;
		case pod::ConstraintType::HINGE: {
			return impl::solveHingeConstraint( constraint, dt );

		} break;
		case pod::ConstraintType::CONE_TWIST: {
			return impl::solveConeTwistConstraint( constraint, dt );
		} break;
	}
}