#pragma once

#include "../structs.h"

namespace impl {
	void solveHingeConstraint( pod::Constraint& constraint, float dt );
	void drawHinge( const pod::Constraint& constraint );
}

namespace uf {
	namespace physics {
		pod::Constraint& UF_API constrainHinge( pod::Constraint& constraint, const pod::Vector3f& joint, const pod::Vector3f& axis );
	}
}