#pragma once

#include "../structs.h"

namespace impl {
	void solveBallSocketConstraint( pod::Constraint& constraint, float dt );
	void drawBallSocket( const pod::Constraint& constraint );
}

namespace uf {
	namespace physics {
		pod::Constraint& UF_API constrainBallSocket( pod::Constraint& constraint, const pod::Vector3f& joint );
	}
}