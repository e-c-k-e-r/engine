#pragma once

#include "../structs.h"

namespace impl {
	void solveSpringConstraint( pod::Constraint& constraint, float dt );
	void drawSpring( const pod::Constraint& constraint );
}

namespace uf {
	namespace physics {
		pod::Constraint& UF_API constrainSpring( pod::Constraint& constraint, const pod::Vector3f& pA, const pod::Vector3f& pB, float stiffness, float damping );
	}
}