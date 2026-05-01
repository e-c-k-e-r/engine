#pragma once

#include "../impl.h"

namespace impl {
	void iterativeImpulseSolver( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Contact& contact, float dt );
}