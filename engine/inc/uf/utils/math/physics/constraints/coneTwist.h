#pragma once

#include "../structs.h"

namespace impl {
	void solveConeTwistConstraint( pod::Constraint& constraint, float dt );
	void drawConeTwist( const pod::Constraint& constraint );
}

namespace uf {
	namespace physics {
		pod::Constraint& UF_API constrainConeTwist( pod::Constraint& constraint, const pod::Vector3f& joint, const pod::Vector3f& axis, float swingLimit = M_PI / 4.0f, float twistLimit = M_PI / 8.0f );
	}
}