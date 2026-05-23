#pragma once

#include "../structs.h"

namespace impl {
	void solveWeldConstraint( pod::Constraint& constraint, float dt );
}

namespace uf {
	namespace physics {
		pod::Constraint& UF_API constrainWeld( pod::Constraint& constraint, const pod::Vector3f& joint, const pod::Vector3f& axis );
	}
}