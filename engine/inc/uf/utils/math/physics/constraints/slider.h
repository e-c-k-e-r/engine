#pragma once

#include "../structs.h"

namespace impl {
	void solveSliderConstraint( pod::Constraint& constraint, float dt );
	void drawSlider( const pod::Constraint& constraint );
}

namespace uf {
	namespace physics {
		pod::Constraint& UF_API constrainSlider( pod::Constraint& constraint, const pod::Vector3f& joint, const pod::Vector3f& axis, float lowerLimit, float upperLimit );
	}
}