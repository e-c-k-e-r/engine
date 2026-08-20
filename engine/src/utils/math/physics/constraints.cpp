#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/constraints.h>

void impl::solveConstraints( uf::stl::vector<pod::Constraint*>& constraints, float dt ) {
	// accumulated impulses are step-local, reset them before solving
	// (limit/motor impulses are clamped against the accumulated value, so a stale value saturates them after the first step)
	for ( auto* constraint : constraints ) {
		switch ( constraint->type ) {
			case pod::ConstraintType::BALL_AND_SOCKET: {
				constraint->ballSocket.accumulatedImpulse = {};
			} break;
			case pod::ConstraintType::HINGE: {
				constraint->hinge.accumulatedImpulse = {};
				constraint->hinge.accumulatedAngularImpulse = {};
			} break;
			case pod::ConstraintType::CONE_TWIST: {
				constraint->coneTwist.accumulatedImpulse = {};
				constraint->coneTwist.accumulatedAngularImpulse = {};
			} break;
			case pod::ConstraintType::SLIDER: {
				constraint->slider.accumulatedLinearImpulse = {};
				constraint->slider.accumulatedAngularImpulse = {};
				constraint->slider.accumulatedLimitImpulse = 0.0f;
			} break;
			case pod::ConstraintType::DISTANCE: {
				constraint->distance.accumulatedImpulse = 0.0f;
			} break;
			case pod::ConstraintType::WELD: {
				constraint->weld.accumulatedLinearImpulse = {};
				constraint->weld.accumulatedAngularImpulse = {};
			} break;
			case pod::ConstraintType::SPRING: {
				constraint->spring.accumulatedImpulse = 0.0f;
			} break;
		}
		constraint->motor.accumulatedMotorImpulse = 0.0f;
	}
	for ( uint32_t i = 0; i < uf::physics::settings.solverIterations; ++i ) {
		for ( auto* constraint : constraints ) impl::solveConstraint( *constraint, dt );
	}
}
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