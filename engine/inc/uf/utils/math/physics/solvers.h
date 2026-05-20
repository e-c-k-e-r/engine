#pragma once

#include "impl.h"

#include "solvers/block.h"
#include "solvers/iterativeImpulse.h"

namespace impl {
	void resolveManifold( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt );
	void solveContacts( uf::stl::vector<pod::Manifold>& manifolds, float dt );
	void solvePositions( uf::stl::vector<pod::Manifold>& manifolds, float dt, uint32_t iterations = 2 );
}