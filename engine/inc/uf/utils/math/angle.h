#pragma once

#include <uf/config.h>

#include <cmath>
#include <stdint.h>

namespace pod {
	// 	Simple angle
	struct UF_API Angle {
		// 	Enums to store unit information
		enum Unit {
			RADIANS,
			DEGREES,
			GRADIANS,
			UNKNOWN,
			DEFAULT = RADIANS,
		};
		// 	Type to store angle measures in
		typedef double type_t;

		Angle::type_t angle; 	// Angle measure
		Angle::Unit unit; 		// Unit angle is stored as
	};
}