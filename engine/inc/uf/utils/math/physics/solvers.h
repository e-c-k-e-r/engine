#pragma once

#include "structs.h"

#include "constraints/contact.h"

#include "solvers/block.h"
#include "solvers/iterativeImpulse.h"

namespace impl {
	void solvePositions( uf::stl::vector<pod::Manifold>& manifolds, float dt, uint32_t iterations = 4 );
}