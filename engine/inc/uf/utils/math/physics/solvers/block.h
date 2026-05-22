#pragma once

#include "../structs.h"

namespace impl {
	/*FORCE_INLINE*/ bool block2x2Solver( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt );
	/*FORCE_INLINE*/ bool block3x3Solver( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt );
	/*FORCE_INLINE*/ bool block4x4Solver( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt );
	/*FORCE_INLINE*/ bool blockSolver( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt );
}