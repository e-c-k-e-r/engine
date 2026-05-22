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
		case pod::ConstraintType::SLIDER: {
			return impl::solveSliderConstraint( constraint, dt );
		} break;
		case pod::ConstraintType::DISTANCE: {
			return impl::solveDistanceConstraint( constraint, dt );
		} break;
		case pod::ConstraintType::WELD: {
			return impl::solveWeldConstraint( constraint, dt );
		} break;
		case pod::ConstraintType::SPRING: {
			return impl::solveSpringConstraint( constraint, dt );
		} break;
	}
}