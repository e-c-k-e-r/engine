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

void uf::physics::setConstraintLimits( pod::Constraint& constraint, float lower, float upper ) {
	switch ( constraint.type ) {
		case pod::ConstraintType::SLIDER:
			constraint.slider.lowerLimit = lower;
			constraint.slider.upperLimit = upper;
		break;
		case pod::ConstraintType::CONE_TWIST:
			constraint.coneTwist.swingLimit = lower;
			constraint.coneTwist.twistLimit = upper;
		break;
	}
}

pod::Constraint& uf::physics::constrain( pod::PhysicsBody& a, pod::PhysicsBody& b ) {
	auto& world = *a.world;
	// allocate constraint struct (pointer cringe because the vector WILL resize)
	auto* pointer = world.constraints.emplace_back(new pod::Constraint);
	auto& constraint = *pointer;
	constraint.a = &a;
	constraint.b = &b;
	return constraint;
}

void uf::physics::unconstrain( pod::PhysicsBody& body ) {
	auto& world = *body.world;
	auto& constraints = world.constraints;
	// remove all constraints that reference this body
	for ( auto it = constraints.begin(); it != constraints.end(); ) {
		auto* constraint = *it;
		if ( constraint->a == &body || constraint->b == &body ) {
			it = constraints.erase(it);
			delete constraint;
		} else {
			++it;
		}
	}
}