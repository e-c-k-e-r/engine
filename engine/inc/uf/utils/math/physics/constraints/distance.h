#pragma once

#include "../structs.h"

namespace impl {
	void solveDistanceConstraint( pod::Constraint& constraint, float dt );
}

namespace uf {
	namespace physics {
		pod::Constraint& UF_API constrainDistance( pod::Constraint& constraint, const pod::Vector3f& pA, const pod::Vector3f& pB, bool isRope = false );
	}
}
