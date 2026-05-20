#pragma once

#include "../impl.h"

namespace impl {
	void blockPGSSolver( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt );
}