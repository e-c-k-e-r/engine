#pragma once

#include "../impl.h"

namespace impl {
	/*FORCE_INLINE*/ void block2x2Solver( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt );
	/*FORCE_INLINE*/ void block3x3Solver( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt );
	/*FORCE_INLINE*/ void block4x4Solver( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt );
}